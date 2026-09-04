#include <kimia/Image.h>

// Vendored third-party code: keep strict warnings scoped to our own code.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#pragma GCC diagnostic pop

#include <stdexcept>

namespace kimia {

namespace {

struct PngWriteContext {
  std::vector<u8>* buffer;
};

void pngWriteCallback(void* context, void* data, int size) {
  auto* ctx = static_cast<PngWriteContext*>(context);
  const u8* bytes = static_cast<const u8*>(data);
  ctx->buffer->insert(ctx->buffer->end(), bytes, bytes + size);
}

}  // namespace

std::optional<Image> Image::load(const std::string& path, std::string& error) {
  int width = 0;
  int height = 0;
  int channels = 0;
  u8* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
  if (data == nullptr) {
    const char* reason = stbi_failure_reason();
    error = "cannot load image '" + path + "'";
    if (reason != nullptr && reason[0] != '\0') error += ": " + std::string(reason);
    return std::nullopt;
  }
  Image image;
  image.width = width;
  image.height = height;
  image.channels = channels;
  const usize byteCount = static_cast<usize>(width) * static_cast<usize>(height) * static_cast<usize>(channels);
  image.pixels.assign(data, data + byteCount);
  stbi_image_free(data);
  return image;
}

Image Image::loadOrThrow(const std::string& path) {
  std::string error;
  auto image = load(path, error);
  if (!image.has_value()) throw std::runtime_error(error);
  return *image;
}

std::vector<u8> Image::encodePNG() const {
  std::vector<u8> result;
  if (isEmpty()) return result;
  PngWriteContext context{&result};
  const int stride = width * channels;
  if (stbi_write_png_to_func(pngWriteCallback, &context, width, height, channels, pixels.data(), stride) == 0) {
    result.clear();
  }
  return result;
}

bool Image::writePNG(const std::string& path) const {
  if (isEmpty()) return false;
  const int stride = width * channels;
  return stbi_write_png(path.c_str(), width, height, channels, pixels.data(), stride) != 0;
}

bool Image::writeJPG(const std::string& path, i32 quality) const {
  if (isEmpty()) return false;
  return stbi_write_jpg(path.c_str(), width, height, channels, pixels.data(), quality) != 0;
}

}  // namespace kimia
