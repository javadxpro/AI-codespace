#include <kimia/WebViewer.h>
#include <kimia_test.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <map>
#include <fstream>
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

// Same as request(), plus a Range header — exactly what a browser sends
// when it streams a video.
HttpResponse rangeRequest(u16 port, const std::string& target, const std::string& range) {
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
  const std::string requestText = "GET " + target + " HTTP/1.1\r\nHost: 127.0.0.1\r\nRange: " + range +
                                  "\r\nConnection: close\r\n\r\n";
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

// --- Branding: the intro film ---

KIMIA_TEST(web_intro_film_is_served_byte_for_byte_or_404) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  // Nothing set: both branding routes are a clean 404, and the engine says so.
  KIMIA_REQUIRE(!server.hasIntro());
  KIMIA_REQUIRE(request(server.port(), "GET", "/intro.mp4").status == 404);
  KIMIA_REQUIRE(request(server.port(), "GET", "/logo.png").status == 404);

  // A tiny stand-in film and poster: the server must hand back exactly the
  // bytes it was given, with the right content types.
  const std::vector<kimia::u8> film{0x00, 0x00, 0x00, 0x18, 'f', 't', 'y', 'p', 0x00, 0xFF, 0x7F, 0x10};
  const std::vector<kimia::u8> logo{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0x00};
  server.setIntro(film, logo);
  KIMIA_REQUIRE(server.hasIntro());

  const HttpResponse movie = request(server.port(), "GET", "/intro.mp4");
  KIMIA_REQUIRE(movie.status == 200);
  KIMIA_REQUIRE(movie.body.size() == 12U);
  for (kimia::usize i = 0; i < film.size(); ++i) {
    KIMIA_REQUIRE(static_cast<kimia::u8>(movie.body[i]) == film[i]);
  }
  const HttpResponse poster = request(server.port(), "GET", "/logo.png");
  KIMIA_REQUIRE(poster.status == 200);
  KIMIA_REQUIRE(poster.body.size() == 9U);
  KIMIA_REQUIRE(static_cast<kimia::u8>(poster.body[0]) == 0x89);
  server.stop();
}

KIMIA_TEST(web_page_carries_the_skippable_splash) {
  const std::string page = kimia::web::makePageHtml("KIMIA World", {}, "", "");
  KIMIA_REQUIRE(page.find("id=\"splash\"") != std::string::npos);
  KIMIA_REQUIRE(page.find("id=\"introfilm\"") != std::string::npos);
  KIMIA_REQUIRE(page.find("/intro.mp4") != std::string::npos);
  // Deliberately NO poster image: a still poster is impossible to tell
  // apart from a video that failed to decode, which is exactly how the
  // «it is only a picture» bug looked to the user.
  KIMIA_REQUIRE(page.find("poster=") == std::string::npos);
  // And a watchdog gives up when the film never actually advances.
  KIMIA_REQUIRE(page.find("currentTime") != std::string::npos);
  KIMIA_REQUIRE(page.find("stalls") != std::string::npos);
  // The splash starts hidden and is only revealed when /intro.mp4 answers,
  // so a build with no branding never shows a black box.
  KIMIA_REQUIRE(page.find("#splash{position:fixed;inset:0;background:#000;display:none") != std::string::npos);
  KIMIA_REQUIRE(page.find("'HEAD'") != std::string::npos || page.find("method:'HEAD'") != std::string::npos);
  // It must always be escapable and never replay in the same tab.
  KIMIA_REQUIRE(page.find("id=\"skip\"") != std::string::npos);
  KIMIA_REQUIRE(page.find("kimiaIntroSeen") != std::string::npos);
}

KIMIA_TEST(web_load_intro_from_folder_finds_the_shipped_film) {
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  // A named folder is authoritative: no film there means no intro, even
  // though a Branding folder exists next to the build.
  KIMIA_REQUIRE(!kimia::web::loadIntroFrom(server, "/tmp/kimia-no-such-branding"));
  KIMIA_REQUIRE(!server.hasIntro());
  // "-" is the explicit opt out (what --no-intro passes).
  KIMIA_REQUIRE(!kimia::web::loadIntroFrom(server, "-"));
  KIMIA_REQUIRE(!server.hasIntro());
  server.stop();
}

KIMIA_TEST(web_intro_film_answers_byte_range_requests) {
  // A phone streams video with Range requests. Without 206 support it gets
  // the sound but a frozen picture, which is exactly the bug this guards.
  kimia::web::Server server;
  KIMIA_REQUIRE(server.start(0, makeTestPage()));
  std::vector<kimia::u8> film(1000U);
  for (kimia::usize i = 0; i < film.size(); ++i) {
    film[i] = static_cast<kimia::u8>(i % 251U);  // a pattern we can verify
  }
  server.setIntro(film, std::vector<kimia::u8>{});

  // A whole-file GET still works and advertises that ranges are welcome.
  const HttpResponse whole = request(server.port(), "GET", "/intro.mp4");
  KIMIA_REQUIRE(whole.status == 200);
  KIMIA_REQUIRE(whole.body.size() == 1000U);
  KIMIA_REQUIRE(whole.headers.count("accept-ranges") == 1U);
  KIMIA_REQUIRE(whole.headers.at("accept-ranges") == "bytes");

  // The opening chunk: 206, the exact bytes, and an honest Content-Range.
  const HttpResponse head = rangeRequest(server.port(), "/intro.mp4", "bytes=0-99");
  KIMIA_REQUIRE(head.status == 206);
  KIMIA_REQUIRE(head.body.size() == 100U);
  KIMIA_REQUIRE(head.headers.at("content-range") == "bytes 0-99/1000");
  for (kimia::usize i = 0; i < 100U; ++i) {
    KIMIA_REQUIRE(static_cast<kimia::u8>(head.body[i]) == film[i]);
  }

  // A chunk from the middle lands on the right offset.
  const HttpResponse middle = rangeRequest(server.port(), "/intro.mp4", "bytes=500-509");
  KIMIA_REQUIRE(middle.status == 206);
  KIMIA_REQUIRE(middle.body.size() == 10U);
  KIMIA_REQUIRE(middle.headers.at("content-range") == "bytes 500-509/1000");
  KIMIA_REQUIRE(static_cast<kimia::u8>(middle.body[0]) == film[500]);
  KIMIA_REQUIRE(static_cast<kimia::u8>(middle.body[9]) == film[509]);

  // Open-ended "from here to the end".
  const HttpResponse tail = rangeRequest(server.port(), "/intro.mp4", "bytes=990-");
  KIMIA_REQUIRE(tail.status == 206);
  KIMIA_REQUIRE(tail.body.size() == 10U);
  KIMIA_REQUIRE(tail.headers.at("content-range") == "bytes 990-999/1000");
  KIMIA_REQUIRE(static_cast<kimia::u8>(tail.body[9]) == film[999]);

  // A range running past the end is clamped, not an error.
  const HttpResponse over = rangeRequest(server.port(), "/intro.mp4", "bytes=995-99999");
  KIMIA_REQUIRE(over.status == 206);
  KIMIA_REQUIRE(over.headers.at("content-range") == "bytes 995-999/1000");
  KIMIA_REQUIRE(over.body.size() == 5U);

  // Nonsense falls back to the whole file rather than failing.
  const HttpResponse junk = rangeRequest(server.port(), "/intro.mp4", "kilograms=3");
  KIMIA_REQUIRE(junk.status == 200);
  KIMIA_REQUIRE(junk.body.size() == 1000U);
  server.stop();
}

KIMIA_TEST(web_shipped_intro_film_is_streamable_from_the_first_byte) {
  // An mp4 whose index (moov) sits at the END cannot start playing until the
  // whole file has arrived — the phone shows a frozen frame with sound. The
  // shipped film must be «faststart»: moov before mdat.
  std::ifstream file("Branding/kimia-intro.mp4", std::ios::binary);
  if (!file) {
    std::printf("SKIP: Branding/kimia-intro.mp4 not next to the test runner\n");
    return;
  }
  const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  KIMIA_REQUIRE(bytes.size() > 64U);
  // Walk the top-level boxes and record where moov and mdat begin.
  kimia::usize moovAt = 0U;
  kimia::usize mdatAt = 0U;
  kimia::usize offset = 0U;
  while (offset + 8U <= bytes.size()) {
    const auto byteAt = [&bytes](kimia::usize i) { return static_cast<kimia::u64>(static_cast<kimia::u8>(bytes[i])); };
    kimia::u64 size = (byteAt(offset) << 24) | (byteAt(offset + 1U) << 16) | (byteAt(offset + 2U) << 8) |
                      byteAt(offset + 3U);
    const std::string type(bytes.data() + offset + 4U, 4U);
    if (type == "moov" && moovAt == 0U) moovAt = offset + 1U;  // +1: 0 means «not seen»
    if (type == "mdat" && mdatAt == 0U) mdatAt = offset + 1U;
    if (size == 0U) break;
    offset += static_cast<kimia::usize>(size);
  }
  KIMIA_REQUIRE(moovAt != 0U);
  KIMIA_REQUIRE(mdatAt != 0U);
  // The index must come first, and right near the front of the file.
  KIMIA_REQUIRE(moovAt < mdatAt);
  KIMIA_REQUIRE(moovAt < 100000U);
}

KIMIA_TEST(web_shipped_intro_film_is_decodable_on_a_phone) {
  // The film once played sound with a frozen picture on a real phone: the
  // encode was High profile at 6.8 Mb/s in a 1270x726 frame, which mobile
  // hardware decoders refuse. This pins the safe shape of the shipped file.
  std::ifstream file("Branding/kimia-intro.mp4", std::ios::binary);
  if (!file) {
    std::printf("SKIP: Branding/kimia-intro.mp4 not next to the test runner\n");
    return;
  }
  const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  KIMIA_REQUIRE(bytes.size() > 1024U);
  const auto byteAt = [&bytes](kimia::usize i) { return static_cast<kimia::u8>(bytes[i]); };

  // Find the avcC box: it carries the H.264 profile the decoder must accept.
  kimia::usize avcc = 0U;
  for (kimia::usize i = 0; i + 12U < bytes.size(); ++i) {
    if (bytes[i] == 'a' && bytes[i + 1U] == 'v' && bytes[i + 2U] == 'c' && bytes[i + 3U] == 'C') {
      avcc = i;
      break;
    }
  }
  KIMIA_REQUIRE(avcc != 0U);
  // avcC layout: [configurationVersion][AVCProfileIndication][compat][level]
  const kimia::u8 profile = byteAt(avcc + 5U);
  const kimia::u8 level = byteAt(avcc + 7U);
  // 66 = Baseline. Anything higher (77 Main, 100 High) is what broke.
  KIMIA_REQUIRE(profile == 66U);
  KIMIA_REQUIRE(level <= 41U);  // level 4.1 or below: every phone handles it

  // The frame size lives in the avc1 SAMPLE ENTRY. Note "avc1" also appears
  // as a compatible-brand string inside ftyp at the very top of the file,
  // so skip anything before the moov box we are really interested in.
  kimia::usize avc1 = 0U;
  for (kimia::usize i = 64U; i + 40U < bytes.size(); ++i) {
    if (bytes[i] == 'a' && bytes[i + 1U] == 'v' && bytes[i + 2U] == 'c' && bytes[i + 3U] == '1') {
      avc1 = i;
      break;
    }
  }
  KIMIA_REQUIRE(avc1 != 0U);
  // Sample entry: 8 reserved, 2 data_ref_idx, 16 pre_defined/reserved, then
  // width and height as u16 — 28 bytes past the type tag.
  const kimia::u32 width = static_cast<kimia::u32>((byteAt(avc1 + 28U) << 8) | byteAt(avc1 + 29U));
  const kimia::u32 height = static_cast<kimia::u32>((byteAt(avc1 + 30U) << 8) | byteAt(avc1 + 31U));
  KIMIA_REQUIRE(width == 1280U);
  KIMIA_REQUIRE(height == 720U);
  // Macroblock-aligned: the odd 1270x726 was part of the original problem.
  KIMIA_REQUIRE(width % 16U == 0U);
  KIMIA_REQUIRE(height % 8U == 0U);

  // And it must stay small enough to stream off a phone in a moment.
  KIMIA_REQUIRE(bytes.size() < 5000000U);
}

// --- The editor has to be REACHABLE ---

KIMIA_TEST(web_game_page_links_to_the_workbench) {
  // The Workbench existed for a whole release with no route to it from the
  // game page: the only way in was to already know the address, which is
  // no use to anybody. The player ran the engine, saw the same old page
  // and reported it as unchanged — quite rightly.
  const std::string page = kimia::web::makePageHtml("KIMIA", {}, "", "");
  KIMIA_REQUIRE(page.find("/bench") != std::string::npos);
  KIMIA_REQUIRE(page.find("Workbench") != std::string::npos);
}
