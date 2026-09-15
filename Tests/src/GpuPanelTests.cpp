#include <kimia_test.h>
#include <kimia/GpuPanel.h>

KIMIA_TEST(Gpu_DrawDefaultDoesNotCrash) {
  kimia::ui::GpuInfo i;
  i.vendor = "Qualcomm";
  i.renderer = "Adreno 640";
  i.version = "OpenGL ES 3.2";
  kimia::ui::drawGpuPanel({0, 0, 280, 280}, i);
}

KIMIA_TEST(Gpu_DrawWithFullInfo) {
  kimia::ui::GpuInfo i;
  i.vendor = "Qualcomm";
  i.renderer = "Adreno 640";
  i.version = "OpenGL ES 3.2 V@415.0";
  i.maxTextureSize = 16384;
  i.maxVertexAttribs = 32;
  i.maxUniformVectors = 4096;
  i.supportsCompute = true;
  i.supportsGeometry = true;
  kimia::ui::drawGpuPanel({0, 0, 320, 320}, i);
}

KIMIA_TEST(Gpu_DrawWithEmptyFields) {
  kimia::ui::GpuInfo i;
  i.vendor = "";
  i.renderer = "";
  i.version = "";
  kimia::ui::drawGpuPanel({0, 0, 280, 280}, i);
}

KIMIA_TEST(Gpu_DrawWithoutComputeOrGeometry) {
  kimia::ui::GpuInfo i;
  i.vendor = "Mali";
  i.renderer = "Mali-G610";
  i.version = "OpenGL ES 3.1";
  i.supportsCompute = false;
  i.supportsGeometry = false;
  kimia::ui::drawGpuPanel({0, 0, 280, 280}, i);
}

KIMIA_TEST(Gpu_DrawAtPhonePortrait) {
  kimia::ui::GpuInfo i;
  i.vendor = "Q";
  i.renderer = "Adreno";
  kimia::ui::drawGpuPanel({0, 0, 240, 320}, i);
}

KIMIA_TEST(Gpu_DrawAtTabletLandscape) {
  kimia::ui::GpuInfo i;
  i.vendor = "NVIDIA";
  i.renderer = "GeForce RTX 3060";
  i.version = "OpenGL 4.6";
  i.maxTextureSize = 32768;
  kimia::ui::drawGpuPanel({0, 0, 480, 320}, i);
}
