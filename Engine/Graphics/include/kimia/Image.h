#pragma once

#include <kimia/Types.h>

#include <optional>
#include <string>
#include <vector>

namespace kimia {

// CPU-side image (u8 per channel). Row-major, row 0 = top.
struct Image {
  i32 width = 0;
  i32 height = 0;
  i32 channels = 0;
  std::vector<u8> pixels;  // width * height * channels

  bool isEmpty() const { return width <= 0 || height <= 0 || channels <= 0 || pixels.empty(); }

  u8* at(i32 x, i32 y) { return pixels.data() + (static_cast<usize>(y) * static_cast<usize>(width) + static_cast<usize>(x)) * static_cast<usize>(channels); }
  const u8* at(i32 x, i32 y) const { return pixels.data() + (static_cast<usize>(y) * static_cast<usize>(width) + static_cast<usize>(x)) * static_cast<usize>(channels); }

  // Loads PNG / JPG / JPEG (and BMP/TGA/GIF via stb_image). Sets `error` and
  // returns nullopt on failure.
  static std::optional<Image> load(const std::string& path, std::string& error);
  static Image loadOrThrow(const std::string& path);

  std::vector<u8> encodePNG() const;
  bool writePNG(const std::string& path) const;
  bool writeJPG(const std::string& path, i32 quality = 90) const;
};

}  // namespace kimia
