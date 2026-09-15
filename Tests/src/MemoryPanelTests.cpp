#include <kimia_test.h>
#include <kimia/MemoryPanel.h>

KIMIA_TEST(MemoryPanel_DrawEmptyDoesNotCrash) {
  kimia::ui::drawMemoryPanel({0, 0, 240, 200}, {});
}

KIMIA_TEST(MemoryPanel_DrawOneBucket) {
  std::vector<kimia::ui::MemoryBucket> v(1);
  v[0].name = "Textures";
  v[0].bytes = 16ULL * 1024 * 1024;
  v[0].peakBytes = 32ULL * 1024 * 1024;
  v[0].allocCount = 42;
  kimia::ui::drawMemoryPanel({0, 0, 240, 200}, v);
}

KIMIA_TEST(MemoryPanel_DrawManyBuckets) {
  std::vector<kimia::ui::MemoryBucket> v;
  v.push_back({"Textures",  16ULL * 1024 * 1024, 32ULL * 1024 * 1024, 50});
  v.push_back({"Meshes",    8ULL * 1024 * 1024,  16ULL * 1024 * 1024, 120});
  v.push_back({"Sounds",    4ULL * 1024 * 1024,  4ULL * 1024 * 1024,  12});
  v.push_back({"Scripts",   1ULL * 1024 * 1024,  2ULL * 1024 * 1024,   8});
  v.push_back({"Scenes",    512 * 1024,          1ULL * 1024 * 1024,   3});
  v.push_back({"UI",        128 * 1024,          256 * 1024,           20});
  kimia::ui::drawMemoryPanel({0, 0, 280, 280}, v);
}

KIMIA_TEST(MemoryPanel_DrawWithZeroBytes) {
  std::vector<kimia::ui::MemoryBucket> v;
  v.push_back({"Empty", 0, 0, 0});
  v.push_back({"Tiny",  64, 64, 1});
  kimia::ui::drawMemoryPanel({0, 0, 240, 200}, v);
}

KIMIA_TEST(MemoryPanel_DrawAtPhonePortrait) {
  std::vector<kimia::ui::MemoryBucket> v;
  for (int i = 0; i < 4; ++i) {
    kimia::ui::MemoryBucket b;
    b.name = "B" + std::to_string(i);
    b.bytes = static_cast<kimia::u64>(i) * 1024 * 1024;
    b.allocCount = i;
    v.push_back(b);
  }
  kimia::ui::drawMemoryPanel({0, 0, 240, 320}, v);
}

KIMIA_TEST(MemoryPanel_DrawAtTabletLandscape) {
  std::vector<kimia::ui::MemoryBucket> v;
  for (int i = 0; i < 10; ++i) {
    kimia::ui::MemoryBucket b;
    b.name = "Bucket_" + std::to_string(i);
    b.bytes = static_cast<kimia::u64>(i) * 1024 * 1024 * 4;
    b.peakBytes = b.bytes * 2;
    b.allocCount = i * 10;
    v.push_back(b);
  }
  kimia::ui::drawMemoryPanel({0, 0, 480, 320}, v);
}
