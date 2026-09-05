#include <kimia/WebViewer.h>
#include <kimia_test.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

using kimia::u8;
using kimia::u16;
using kimia::usize;

struct HttpResponse {
  int status = 0;
  std::map<std::string, std::string> headers;
  std::string body;
};

// Minimal raw-socket HTTP client (no browser involved, per the engine spec).
HttpResponse request(u16 port, const std::string& method, const std::string& target) {
  HttpResponse response;
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return response;
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  ::inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
  if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
    ::close(fd);
    return response;
  }
  const std::string requestText = method + " " + target + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
  usize sent = 0;
  while (sent < requestText.size()) {
    const ssize_t n = ::send(fd, requestText.data() + sent, requestText.size() - sent, 0);
    if (n <= 0) {
      ::close(fd);
      return response;
    }
    sent += static_cast<usize>(n);
  }
  std::string raw;
  char buffer[4096];
  for (;;) {
    const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
    if (n <= 0) break;
    raw.append(buffer, static_cast<usize>(n));
  }
  ::close(fd);

  const usize headerEnd = raw.find("\r\n\r\n");
  if (headerEnd == std::string::npos) return response;
  const usize firstLineEnd = raw.find("\r\n");
  if (firstLineEnd == std::string::npos) return response;
  response.status = std::atoi(raw.substr(9, 3).c_str());
  usize lineStart = firstLineEnd + 2U;
  while (lineStart < headerEnd) {
    const usize lineEnd = raw.find("\r\n", lineStart);
    if (lineEnd == std::string::npos || lineEnd >= headerEnd) break;
    const std::string line = raw.substr(lineStart, lineEnd - lineStart);
    const usize colon = line.find(':');
    if (colon != std::string::npos) {
      std::string name = line.substr(0, colon);
      for (char& c : name) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
      }
      response.headers[name] = line.substr(colon + 2U);
    }
    lineStart = lineEnd + 2U;
  }
  response.body = raw.substr(headerEnd + 4U);
  return response;
}

std::string makeTestPage() {
  return std::string("<html><head><title>KIMIA WEB TEST</title></head><body>page</body></html>");
}

}  // namespace

KIMIA_TEST(web_get_root_returns_html_page) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  KIMIA_REQUIRE(server.port() > 0);
  KIMIA_REQUIRE(server.running());
  const HttpResponse response = request(server.port(), "GET", "/");
  KIMIA_REQUIRE(response.status == 200);
  KIMIA_REQUIRE(response.headers.find("content-type") != response.headers.end());
  KIMIA_REQUIRE(response.headers.at("content-type").find("text/html") != std::string::npos);
  KIMIA_REQUIRE(response.body.find("KIMIA WEB TEST") != std::string::npos);
  server.stop();
}

KIMIA_TEST(web_frame_png_503_before_first_publish) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  const HttpResponse before = request(server.port(), "GET", "/frame.png");
  KIMIA_REQUIRE(before.status == 503);
  server.stop();
}

KIMIA_TEST(web_frame_png_200_after_publish_with_exact_bytes) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  std::vector<u8> png = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 1, 2, 3, 4};
  server.publishFrame(png, "frame 1");
  const HttpResponse response = request(server.port(), "GET", "/frame.png");
  KIMIA_REQUIRE(response.status == 200);
  KIMIA_REQUIRE(response.headers.at("content-type").find("image/png") != std::string::npos);
  KIMIA_REQUIRE(response.body.size() == png.size());
  KIMIA_REQUIRE(std::memcmp(response.body.data(), png.data(), png.size()) == 0);
  server.stop();
}

KIMIA_TEST(web_stats_returns_last_stats_line) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  server.publishFrame({1, 2, 3}, "KIMIA GOLF | EDIT | stroke 2");
  const HttpResponse response = request(server.port(), "GET", "/stats");
  KIMIA_REQUIRE(response.status == 200);
  KIMIA_REQUIRE(response.body.find("KIMIA GOLF | EDIT | stroke 2") != std::string::npos);
  server.stop();
}

KIMIA_TEST(web_held_keys_drain_as_level) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  KIMIA_REQUIRE(request(server.port(), "POST", "/input?key=a&down=1").status == 200);
  kimia::web::DrainedInput first = server.drain();
  KIMIA_REQUIRE(first.held.at("a") == true);
  kimia::web::DrainedInput second = server.drain();  // level: still held
  KIMIA_REQUIRE(second.held.at("a") == true);
  KIMIA_REQUIRE(request(server.port(), "POST", "/input?key=a&down=0").status == 200);
  kimia::web::DrainedInput third = server.drain();
  KIMIA_REQUIRE(third.held.at("a") == false);
  server.stop();
}

KIMIA_TEST(web_taps_drain_as_edges_once) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  KIMIA_REQUIRE(request(server.port(), "POST", "/input?tap=return&down=1").status == 200);
  kimia::web::DrainedInput first = server.drain();
  KIMIA_REQUIRE(first.taps.size() == 1U);
  KIMIA_REQUIRE(first.taps[0] == "return");
  kimia::web::DrainedInput second = server.drain();  // edge: cleared
  KIMIA_REQUIRE(second.taps.empty());
  server.stop();
}

KIMIA_TEST(web_look_and_zoom_accumulate_as_edges) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  KIMIA_REQUIRE(request(server.port(), "POST", "/input?lookX=1.5&lookY=-2.25").status == 200);
  KIMIA_REQUIRE(request(server.port(), "POST", "/input?zoom=0.5").status == 200);
  kimia::web::DrainedInput first = server.drain();
  KIMIA_REQUIRE(first.lookX == 1.5);
  KIMIA_REQUIRE(first.lookY == -2.25);
  KIMIA_REQUIRE(first.zoom == 0.5);
  kimia::web::DrainedInput second = server.drain();  // edges: cleared
  KIMIA_REQUIRE(second.lookX == 0.0 && second.lookY == 0.0 && second.zoom == 0.0);
  server.stop();
}

KIMIA_TEST(web_sound_cues_are_sequenced_and_served_as_wav) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  // Nothing registered: sequence 0, no name, unknown sfx is 404.
  KIMIA_REQUIRE(server.soundSequence() == 0U);
  HttpResponse sound = request(server.port(), "GET", "/sound");
  KIMIA_REQUIRE(sound.status == 200);
  KIMIA_REQUIRE(sound.body == "0 ");
  KIMIA_REQUIRE(request(server.port(), "GET", "/sfx/holed").status == 404);
  // Cueing an unregistered name is ignored (no sequence bump).
  server.playSound("holed");
  KIMIA_REQUIRE(server.soundSequence() == 0U);
  // Register two cues; the bytes come back exactly with audio/wav.
  const std::vector<u8> holed = {'R', 'I', 'F', 'F', 1, 2, 3, 4};
  const std::vector<u8> shot = {'R', 'I', 'F', 'F', 9, 9};
  server.registerSound("holed", holed);
  server.registerSound("shot", shot);
  const HttpResponse wav = request(server.port(), "GET", "/sfx/holed");
  KIMIA_REQUIRE(wav.status == 200);
  KIMIA_REQUIRE(wav.headers.at("content-type").find("audio/wav") != std::string::npos);
  KIMIA_REQUIRE(wav.body.size() == holed.size());
  KIMIA_REQUIRE(std::memcmp(wav.body.data(), holed.data(), holed.size()) == 0);
  // Each cue bumps the sequence and names the latest sound.
  server.playSound("shot");
  KIMIA_REQUIRE(server.soundSequence() == 1U);
  KIMIA_REQUIRE(request(server.port(), "GET", "/sound").body == "1 shot");
  server.playSound("shot");
  server.playSound("holed");
  KIMIA_REQUIRE(server.soundSequence() == 3U);
  KIMIA_REQUIRE(request(server.port(), "GET", "/sound").body == "3 holed");
  // Re-registering replaces the bytes.
  server.registerSound("shot", {7});
  KIMIA_REQUIRE(request(server.port(), "GET", "/sfx/shot").body == std::string(1, '\x07'));
  server.stop();
}

KIMIA_TEST(web_unknown_route_is_404) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  KIMIA_REQUIRE(request(server.port(), "GET", "/nope").status == 404);
  server.stop();
}

KIMIA_TEST(web_make_page_contains_title_buttons_and_keymap) {
  const std::vector<kimia::web::PadButton> buttons = {{"Shoot", "space", true}, {"Place", "return", false}};
  const std::string page = kimia::web::makePageHtml("KIMIA Golf", buttons, "var keymap = {'a':'h:a'};", "hint text");
  KIMIA_REQUIRE(page.find("KIMIA Golf") != std::string::npos);
  KIMIA_REQUIRE(page.find("Shoot") != std::string::npos);
  KIMIA_REQUIRE(page.find("Place") != std::string::npos);
  KIMIA_REQUIRE(page.find("keymap") != std::string::npos);
  KIMIA_REQUIRE(page.find("hint text") != std::string::npos);
  KIMIA_REQUIRE(page.find("/frame.png") != std::string::npos);
  KIMIA_REQUIRE(page.find("/stats") != std::string::npos);
  // The sound poller and the gesture unlock are part of every page.
  KIMIA_REQUIRE(page.find("/sound") != std::string::npos);
  KIMIA_REQUIRE(page.find("'/sfx/'") != std::string::npos);
  KIMIA_REQUIRE(page.find("pointerdown") != std::string::npos);
}

KIMIA_TEST(web_menu_default_empty) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  const HttpResponse response = request(server.port(), "GET", "/menu");
  KIMIA_REQUIRE(response.status == 200);
  const auto contentType = response.headers.find("content-type");
  KIMIA_REQUIRE(contentType != response.headers.end());
  KIMIA_REQUIRE(contentType->second.find("application/json") != std::string::npos);
  KIMIA_REQUIRE(response.body.find("\"title\":\"\"") != std::string::npos);
  KIMIA_REQUIRE(response.body.find("\"holds\":[]") != std::string::npos);
  KIMIA_REQUIRE(response.body.find("\"taps\":[]") != std::string::npos);
  server.stop();
}

KIMIA_TEST(web_menu_roundtrip_with_persian_labels) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  kimia::web::Menu menu;
  menu.title = "\xD8\xAA\xD9\x88\xD9\xBE\x3A \xD8\xAF\xD9\x82\xDB\x8C\xD9\x82 "  // "توپ: دقیق "
              "\xD8\xA8\xD8\xA7\xD8\xB4\xD9\x87 \xDB\x8C\xD8\xA7 "
              "\xD9\x81\xD8\xA7\xD9\x86\xD8\xAA\xD8\xB2\xDB\x8C\x3F";              // "باشه یا فانتزی؟"
  menu.taps.push_back({"\xD8\xAF\xD9\x82\xDB\x8C\xD9\x82", "num1"});   // دقیق
  menu.taps.push_back({"\xD9\x81\xD8\xA7\xD9\x86\xD8\xAA\xD8\xB2\xDB\x8C", "num2"});  // فانتزی
  menu.holds.push_back({"\xD8\xA8\xD8\xA7\xD9\x84\xD8\xA7", "up"});   // بالا
  server.setMenu(menu);
  const HttpResponse response = request(server.port(), "GET", "/menu");
  KIMIA_REQUIRE(response.status == 200);
  // UTF-8 labels pass through verbatim inside the JSON strings.
  KIMIA_REQUIRE(response.body.find("\xD8\xAF\xD9\x82\xDB\x8C\xD9\x82") != std::string::npos);
  KIMIA_REQUIRE(response.body.find("\xD9\x81\xD8\xA7\xD9\x86\xD8\xAA\xD8\xB2\xDB\x8C") != std::string::npos);
  KIMIA_REQUIRE(response.body.find("\"num1\"") != std::string::npos);
  KIMIA_REQUIRE(response.body.find("\"num2\"") != std::string::npos);
  KIMIA_REQUIRE(response.body.find("\"up\"") != std::string::npos);
  KIMIA_REQUIRE(response.body.find("\"holds\":[") != std::string::npos);
  KIMIA_REQUIRE(response.body.find("\"taps\":[") != std::string::npos);
  // Clearing the menu hides it again.
  server.setMenu(kimia::web::Menu{});
  const HttpResponse cleared = request(server.port(), "GET", "/menu");
  KIMIA_REQUIRE(cleared.body.find("\"title\":\"\"") != std::string::npos);
  server.stop();
}

KIMIA_TEST(web_make_page_includes_menu_machinery) {
  const std::string page = kimia::web::makePageHtml("KIMIA World", {}, "", "");
  KIMIA_REQUIRE(page.find("id=\"menutitle\"") != std::string::npos);
  KIMIA_REQUIRE(page.find("id=\"pad\"") != std::string::npos);
  KIMIA_REQUIRE(page.find("id=\"staticpad\"") != std::string::npos);
  KIMIA_REQUIRE(page.find("/menu") != std::string::npos);
  KIMIA_REQUIRE(page.find("showMenu") != std::string::npos);
  KIMIA_REQUIRE(page.find("bindPad") != std::string::npos);
}

KIMIA_TEST(web_restart_and_ephemeral_port) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  const u16 firstPort = server.port();
  KIMIA_REQUIRE(firstPort > 0);
  server.stop();
  KIMIA_REQUIRE(!server.running());
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  KIMIA_REQUIRE(server.running());
  server.stop();
}
