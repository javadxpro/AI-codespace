#pragma once

#include <cstddef>
#include <string>

namespace kimia {
namespace embedded {

// One asset bundled into the binary at build time (Tools/make_embedded_assets.py).
// `path` is the runtime-relative path ("profiles/golf.kimiaprofile", …).
struct Asset {
  const char* path;
  const unsigned char* data;
  std::size_t size;
};

// Null-terminated list of every embedded asset. Returns a pointer to a static
// array whose last entry has path == nullptr.
const Asset* assetList();

// The asset whose `path` matches exactly, or nullptr.
const Asset* find(const char* path);

// Writes every embedded asset under `rootDir` (creating subdirectories).
// Returns false on any I/O error; the caller can simply retry later.
bool extractAll(const std::string& rootDir);

}  // namespace embedded
}  // namespace kimia
