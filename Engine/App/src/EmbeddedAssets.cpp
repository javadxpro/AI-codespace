#include <kimia/EmbeddedAssets.h>

#include <cstring>
#include <filesystem>
#include <fstream>

namespace kimia {
namespace embedded {

const Asset* find(const char* path) {
  for (const Asset* a = assetList(); a != nullptr && a->path != nullptr; ++a) {
    if (std::strcmp(a->path, path) == 0) return a;
  }
  return nullptr;
}

bool extractAll(const std::string& rootDir) {
  namespace fs = std::filesystem;
  std::error_code ec;
  for (const Asset* a = assetList(); a != nullptr && a->path != nullptr; ++a) {
    const fs::path target = fs::path(rootDir) / a->path;
    fs::create_directories(target.parent_path(), ec);
    if (ec) return false;
    std::ofstream out(target, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(a->data), static_cast<std::streamsize>(a->size));
    if (!out) return false;
  }
  return true;
}

}  // namespace embedded
}  // namespace kimia
