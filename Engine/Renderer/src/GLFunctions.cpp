#include <kimia/GLFunctions.h>

#include <dlfcn.h>

namespace kimia {

GLFunctions& GLFunctions::instance() {
  static GLFunctions functions;
  return functions;
}

namespace {
// The dlopen handle used by resolve() (load() runs once at startup, before
// any threads touch GL, so a plain file-scope pointer is fine here).
void* gLibraryHandle = nullptr;

void* resolve(const char* name) {
  return gLibraryHandle != nullptr ? dlsym(gLibraryHandle, name) : nullptr;
}

#define LOAD(name) name##Fn = reinterpret_cast<decltype(name##Fn)>(resolver("gl" #name))
}  // namespace

bool GLFunctions::load(GLGetProcFn proc) {
  if (loaded_) return true;

  GLGetProcFn resolver = proc;
  if (resolver == nullptr) {
    handle_ = dlopen("libGL.so.1", RTLD_NOW | RTLD_LOCAL);
    if (handle_ == nullptr) handle_ = dlopen("libGL.so", RTLD_NOW | RTLD_LOCAL);
    if (handle_ == nullptr) return false;
    gLibraryHandle = handle_;
    resolver = resolve;
  }

  LOAD(createShader);
  LOAD(shaderSource);
  LOAD(compileShader);
  LOAD(getShaderiv);
  LOAD(getShaderInfoLog);
  LOAD(createProgram);
  LOAD(attachShader);
  LOAD(linkProgram);
  LOAD(getProgramiv);
  LOAD(getProgramInfoLog);
  LOAD(useProgram);
  LOAD(deleteShader);
  LOAD(deleteProgram);
  LOAD(genVertexArrays);
  LOAD(bindVertexArray);
  LOAD(deleteVertexArrays);
  LOAD(genBuffers);
  LOAD(bindBuffer);
  LOAD(bufferData);
  LOAD(deleteBuffers);
  LOAD(enableVertexAttribArray);
  LOAD(vertexAttribPointer);
  LOAD(getUniformLocation);
  LOAD(uniformMatrix4fv);
  LOAD(uniform3f);
  LOAD(uniform1f);
  LOAD(uniform1i);
  LOAD(activeTexture);
  LOAD(genTextures);
  LOAD(bindTexture);
  LOAD(texImage2D);
  LOAD(texParameteri);
  LOAD(generateMipmap);
  LOAD(deleteTextures);
  LOAD(viewport);
  LOAD(clear);
  LOAD(clearColor);
  LOAD(clearDepth);
  LOAD(enable);
  LOAD(disable);
  LOAD(depthFunc);
  LOAD(cullFace);
  LOAD(frontFace);
  LOAD(drawElements);
  LOAD(readPixels);
  LOAD(getIntegerv);
  LOAD(getString);
  LOAD(getError);
  LOAD(genFramebuffers);
  LOAD(bindFramebuffer);
  LOAD(framebufferTexture2D);
  LOAD(checkFramebufferStatus);
  LOAD(deleteFramebuffers);
  LOAD(pixelStorei);
  LOAD(depthMask);

  gLibraryHandle = nullptr;
  if (createShaderFn == nullptr || createProgramFn == nullptr || drawElementsFn == nullptr ||
      useProgramFn == nullptr || clearFn == nullptr) {
    unload();
    return false;
  }
  loaded_ = true;
  return true;
}

void GLFunctions::unload() {
  if (handle_ != nullptr) {
    dlclose(handle_);
    handle_ = nullptr;
  }
  loaded_ = false;
  createShaderFn = nullptr;
  shaderSourceFn = nullptr;
  compileShaderFn = nullptr;
  getShaderivFn = nullptr;
  getShaderInfoLogFn = nullptr;
  createProgramFn = nullptr;
  attachShaderFn = nullptr;
  linkProgramFn = nullptr;
  getProgramivFn = nullptr;
  getProgramInfoLogFn = nullptr;
  useProgramFn = nullptr;
  deleteShaderFn = nullptr;
  deleteProgramFn = nullptr;
  genVertexArraysFn = nullptr;
  bindVertexArrayFn = nullptr;
  deleteVertexArraysFn = nullptr;
  genBuffersFn = nullptr;
  bindBufferFn = nullptr;
  bufferDataFn = nullptr;
  deleteBuffersFn = nullptr;
  enableVertexAttribArrayFn = nullptr;
  vertexAttribPointerFn = nullptr;
  getUniformLocationFn = nullptr;
  uniformMatrix4fvFn = nullptr;
  uniform3fFn = nullptr;
  uniform1fFn = nullptr;
  uniform1iFn = nullptr;
  activeTextureFn = nullptr;
  genTexturesFn = nullptr;
  bindTextureFn = nullptr;
  texImage2DFn = nullptr;
  texParameteriFn = nullptr;
  generateMipmapFn = nullptr;
  deleteTexturesFn = nullptr;
  viewportFn = nullptr;
  clearFn = nullptr;
  clearColorFn = nullptr;
  clearDepthFn = nullptr;
  enableFn = nullptr;
  disableFn = nullptr;
  depthFuncFn = nullptr;
  cullFaceFn = nullptr;
  frontFaceFn = nullptr;
  drawElementsFn = nullptr;
  readPixelsFn = nullptr;
  getIntegervFn = nullptr;
  getStringFn = nullptr;
  getErrorFn = nullptr;
  genFramebuffersFn = nullptr;
  bindFramebufferFn = nullptr;
  framebufferTexture2DFn = nullptr;
  checkFramebufferStatusFn = nullptr;
  deleteFramebuffersFn = nullptr;
  pixelStoreiFn = nullptr;
  depthMaskFn = nullptr;
}

}  // namespace kimia
