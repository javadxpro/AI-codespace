#include <kimia/WebViewer.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <sstream>
#include <thread>

namespace kimia {
namespace web {

namespace {

std::string htmlEscape(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  for (const char c : text) {
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

std::string urlDecode(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  for (usize i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (c == '+') {
      out += ' ';
    } else if (c == '%' && i + 2U < text.size()) {
      const int hi = hexValue(text[i + 1U]);
      const int lo = hexValue(text[i + 2U]);
      if (hi >= 0 && lo >= 0) {
        out += static_cast<char>((hi << 4) | lo);
        i += 2U;
      } else {
        out += c;
      }
    } else {
      out += c;
    }
  }
  return out;
}

bool parseF64Token(const std::string& token, f64& out) {
  if (token.empty()) return false;
  try {
    usize consumed = 0;
    out = std::stod(token, &consumed);
    return consumed == token.size();
  } catch (...) {
    return false;
  }
}

// Parses "key=value&key=value..." into a map.
std::map<std::string, std::string> parseQuery(const std::string& query) {
  std::map<std::string, std::string> params;
  usize begin = 0;
  while (begin <= query.size()) {
    const usize amp = query.find('&', begin);
    const std::string pair = query.substr(begin, amp == std::string::npos ? std::string::npos : amp - begin);
    const usize eq = pair.find('=');
    if (eq != std::string::npos) {
      params[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1U));
    } else if (!pair.empty()) {
      params[urlDecode(pair)] = "";
    }
    if (amp == std::string::npos) break;
    begin = amp + 1U;
  }
  return params;
}

bool sendAll(int fd, const std::string& data) {
  usize sent = 0;
  while (sent < data.size()) {
    const ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, 0);
    if (n <= 0) return false;
    sent += static_cast<usize>(n);
  }
  return true;
}

std::string httpResponse(const std::string& status, const std::string& contentType, const std::string& body) {
  std::ostringstream out;
  out << "HTTP/1.1 " << status << "\r\n";
  out << "Content-Type: " << contentType << "\r\n";
  out << "Content-Length: " << body.size() << "\r\n";
  out << "Cache-Control: no-store\r\n";
  out << "Connection: close\r\n\r\n";
  out << body;
  return out.str();
}

std::string statusLine(int code) {
  switch (code) {
    case 200:
      return "200 OK";
    case 404:
      return "404 Not Found";
    case 503:
      return "503 Service Unavailable";
    default:
      return "500 Internal Server Error";
  }
}

}  // namespace

struct Server::Impl {
  std::mutex mutex;
  std::vector<u8> frame;
  bool hasFrame = false;
  std::string stats;
  std::map<std::string, bool> held;
  std::vector<std::string> taps;
  f64 lookX = 0.0;
  f64 lookY = 0.0;
  f64 zoom = 0.0;
  std::string page;
  int listenFd = -1;
  u16 boundPort = 0;
  std::thread acceptThread;
  std::atomic<bool> stopFlag{false};
};

namespace {

void applyInputParams(Server::Impl* impl, const std::map<std::string, std::string>& params) {
  std::lock_guard<std::mutex> lock(impl->mutex);
  const auto key = params.find("key");
  if (key != params.end() && !key->second.empty()) {
    const auto down = params.find("down");
    const bool isDown = down != params.end() && down->second == "1";
    impl->held[key->second] = isDown;
  }
  const auto tap = params.find("tap");
  if (tap != params.end() && !tap->second.empty()) impl->taps.push_back(tap->second);
  const auto lookX = params.find("lookX");
  if (lookX != params.end()) {
    f64 value = 0.0;
    if (parseF64Token(lookX->second, value)) impl->lookX += value;
  }
  const auto lookY = params.find("lookY");
  if (lookY != params.end()) {
    f64 value = 0.0;
    if (parseF64Token(lookY->second, value)) impl->lookY += value;
  }
  const auto zoom = params.find("zoom");
  if (zoom != params.end()) {
    f64 value = 0.0;
    if (parseF64Token(zoom->second, value)) impl->zoom += value;
  }
}

void handleConnection(int fd, Server::Impl* impl) {
  timeval timeout{};
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  std::string request;
  char buffer[2048];
  for (;;) {
    const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
    if (n <= 0) break;
    request.append(buffer, static_cast<usize>(n));
    if (request.find("\r\n\r\n") != std::string::npos) break;
    if (request.size() > 16384U) break;
  }

  std::string method;
  std::string target;
  {
    std::istringstream stream(request);
    stream >> method >> target;
  }
  if (method.empty() || target.empty()) {
    ::close(fd);
    return;
  }

  std::string path = target;
  std::string query;
  const usize question = target.find('?');
  if (question != std::string::npos) {
    path = target.substr(0, question);
    query = target.substr(question + 1U);
  }

  std::string response;
  if (path == "/") {
    response = httpResponse(statusLine(200), "text/html; charset=utf-8", impl->page);
  } else if (path == "/frame.png") {
    std::lock_guard<std::mutex> lock(impl->mutex);
    if (impl->hasFrame) {
      const std::string body(reinterpret_cast<const char*>(impl->frame.data()), impl->frame.size());
      response = httpResponse(statusLine(200), "image/png", body);
    } else {
      response = httpResponse(statusLine(503), "text/plain; charset=utf-8", "no frame yet");
    }
  } else if (path == "/stats") {
    std::lock_guard<std::mutex> lock(impl->mutex);
    response = httpResponse(statusLine(200), "text/plain; charset=utf-8", impl->stats);
  } else if (path == "/input" && method == "POST") {
    applyInputParams(impl, parseQuery(query));
    response = httpResponse(statusLine(200), "text/plain; charset=utf-8", "ok");
  } else {
    response = httpResponse(statusLine(404), "text/plain; charset=utf-8", "not found");
  }
  sendAll(fd, response);
  ::close(fd);
}

void acceptLoop(Server::Impl* impl) {
  while (!impl->stopFlag.load()) {
    sockaddr_in address{};
    socklen_t addressLength = sizeof(address);
    const int client = ::accept(impl->listenFd, reinterpret_cast<sockaddr*>(&address), &addressLength);
    if (client < 0) {
      if (impl->stopFlag.load()) break;
      continue;
    }
    std::thread(handleConnection, client, impl).detach();
  }
}

}  // namespace

Server::Server() : impl_(new Impl()) {}

Server::~Server() { stop(); }

bool Server::start(u16 port, const std::string& pageHtml) {
  stop();
  impl_->page = pageHtml;
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return false;
  int reuse = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);
  if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
    ::close(fd);
    return false;
  }
  if (::listen(fd, 16) != 0) {
    ::close(fd);
    return false;
  }
  sockaddr_in bound{};
  socklen_t boundLength = sizeof(bound);
  u16 actualPort = port;
  if (::getsockname(fd, reinterpret_cast<sockaddr*>(&bound), &boundLength) == 0) {
    actualPort = ntohs(bound.sin_port);
  }
  impl_->listenFd = fd;
  impl_->boundPort = actualPort;
  impl_->stopFlag.store(false);
  impl_->acceptThread = std::thread(acceptLoop, impl_.get());
  return true;
}

u16 Server::port() const { return impl_->boundPort; }

bool Server::running() const { return impl_->listenFd >= 0; }

void Server::stop() {
  impl_->stopFlag.store(true);
  if (impl_->listenFd >= 0) {
    ::shutdown(impl_->listenFd, SHUT_RDWR);
    ::close(impl_->listenFd);
    impl_->listenFd = -1;
  }
  if (impl_->acceptThread.joinable()) impl_->acceptThread.join();
  // Let in-flight handlers finish (per the engine convention).
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void Server::publishFrame(std::vector<u8> pngBytes, const std::string& statsLine) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->frame = std::move(pngBytes);
  impl_->hasFrame = true;
  impl_->stats = statsLine;
}

DrainedInput Server::drain() {
  DrainedInput out;
  std::lock_guard<std::mutex> lock(impl_->mutex);
  out.held = impl_->held;
  out.taps = std::move(impl_->taps);
  impl_->taps.clear();
  out.lookX = impl_->lookX;
  out.lookY = impl_->lookY;
  out.zoom = impl_->zoom;
  impl_->lookX = 0.0;
  impl_->lookY = 0.0;
  impl_->zoom = 0.0;
  return out;
}

std::string makePageHtml(const std::string& title, const std::vector<PadButton>& padButtons,
                         const std::string& keymapJs, const std::string& hint) {
  std::ostringstream out;
  out << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n";
  out << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\">\n";
  out << "<title>" << htmlEscape(title) << "</title>\n<style>\n";
  out << "body{background:#101014;color:#e8e8ec;font-family:system-ui,sans-serif;margin:0;padding:12px;"
         "touch-action:manipulation;user-select:none;-webkit-user-select:none}\n";
  out << "h1{font-size:20px;margin:4px 0}\n.hint{color:#9a9aa5;font-size:13px;margin:4px 0 10px}\n";
  out << "#frame{width:100%;max-width:720px;display:block;background:#000;border-radius:8px}\n";
  out << "#stats{font-family:monospace;font-size:12px;color:#7fd47f;margin:8px 0;white-space:pre-wrap}\n";
  out << "#pad{display:flex;flex-wrap:wrap;gap:8px;margin:10px 0;max-width:720px}\n";
  out << ".btn{min-width:52px;min-height:44px;padding:8px 12px;font-size:16px;border:1px solid #3a3a44;"
         "border-radius:10px;background:#23232c;color:#e8e8ec;text-align:center;cursor:pointer}\n";
  out << ".btn:active{background:#3d4a7a}\n";
  out << "#look{width:100%;max-width:720px;height:120px;border:1px dashed #3a3a44;border-radius:10px;"
         "margin:10px 0;display:flex;align-items:center;justify-content:center;color:#7a7a88;font-size:14px;"
         "touch-action:none}\n";
  out << "</style>\n</head>\n<body>\n";
  out << "<h1>" << htmlEscape(title) << "</h1>\n";
  if (!hint.empty()) out << "<div class=\"hint\">" << htmlEscape(hint) << "</div>\n";
  out << "<img id=\"frame\" alt=\"engine frame\">\n<div id=\"stats\">waiting for first frame...</div>\n";
  out << "<div id=\"pad\">\n";
  for (const PadButton& button : padButtons) {
    out << "<div class=\"btn\" data-key=\"" << htmlEscape(button.key) << "\" data-hold=\""
        << (button.hold ? "1" : "0") << "\">" << htmlEscape(button.label) << "</div>\n";
  }
  out << "</div>\n";
  out << "<div id=\"look\">drag here to look around</div>\n";
  out << "<script>\n";
  out << "function post(q){fetch('/input?'+q,{method:'POST'}).catch(function(){});}\n";
  out << "var pad=document.getElementById('pad');\n";
  out << "pad.addEventListener('pointerdown',function(e){\n";
  out << "  var b=e.target.closest('.btn');if(!b)return;\n";
  out << "  e.preventDefault();\n";
  out << "  if(b.getAttribute('data-hold')==='1'){post('key='+encodeURIComponent(b.getAttribute('data-key'))+'&down=1');}\n";
  out << "});\n";
  out << "pad.addEventListener('pointerup',function(e){\n";
  out << "  var b=e.target.closest('.btn');if(!b)return;\n";
  out << "  var k=encodeURIComponent(b.getAttribute('data-key'));\n";
  out << "  if(b.getAttribute('data-hold')==='1'){post('key='+k+'&down=0');}\n";
  out << "  else{post('tap='+k);}\n";
  out << "});\n";
  out << "var look=document.getElementById('look');\n";
  out << "var dragging=false,lastX=0,lastY=0;\n";
  out << "look.addEventListener('pointerdown',function(e){dragging=true;lastX=e.clientX;lastY=e.clientY;"
         "look.setPointerCapture(e.pointerId);});\n";
  out << "look.addEventListener('pointermove',function(e){if(!dragging)return;\n";
  out << "  var dx=e.clientX-lastX,dy=e.clientY-lastY;lastX=e.clientX;lastY=e.clientY;\n";
  out << "  post('lookX='+dx+'&lookY='+dy);\n";
  out << "});\n";
  out << "look.addEventListener('pointerup',function(){dragging=false;});\n";
  out << "var img=document.getElementById('frame');\n";
  out << "setInterval(function(){img.src='/frame.png?t='+Date.now();},100);\n";
  out << "setInterval(function(){fetch('/stats').then(function(r){return r.text();}).then(function(t){"
         "document.getElementById('stats').textContent=t;}).catch(function(){});},500);\n";
  if (!keymapJs.empty()) out << keymapJs << '\n';
  out << "</script>\n</body>\n</html>\n";
  return out.str();
}

}  // namespace web
}  // namespace kimia
