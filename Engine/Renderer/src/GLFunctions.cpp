#include <kimia/GLFunctions.h>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <type_traits>
#endif

namespace kimia {

GLFunctions& GLFunctions::instance() {
  static GLFunctions functions;
  return functions;
}

namespace {

#ifndef __EMSCRIPTEN__
// The dlopen handle used by resolve() (load() runs once at startup, before
// any threads touch GL, so a plain file-scope pointer is fine here).
void* gLibraryHandle = nullptr;

void* resolve(const char* name) {
#ifdef _WIN32
  return gLibraryHandle != nullptr
             ? reinterpret_cast<void*>(::GetProcAddress(reinterpret_cast<HMODULE>(gLibraryHandle), name))
             : nullptr;
#else
  return gLibraryHandle != nullptr ? dlsym(gLibraryHandle, name) : nullptr;
#endif
}

#ifdef __ANDROID__
// Android GLES3: glBufferData's GLsizeiptr is pointer-sized (32-bit on
// armeabi-v7a, 64-bit on arm64/x86_64) while the engine always passes an
// i64, and glClearDepth (double) does not exist — it is glClearDepthf.
// Shim both through the dlopen'd library so the ABI matches on every ABI.
using NativeGLSize = std::conditional<sizeof(void*) == 8, kimia::i64, kimia::i32>::type;
void bufferDataShim(kimia::GLenum target, kimia::GLsizeiptr size, const void* data, kimia::GLenum usage) {
  const auto fn = reinterpret_cast<void (*)(kimia::GLenum, NativeGLSize, const void*, kimia::GLenum)>(
      resolve("glBufferData"));
  if (fn != nullptr) fn(target, static_cast<NativeGLSize>(size), data, usage);
}
void clearDepthShim(kimia::f64 depth) {
  const auto fn = reinterpret_cast<void (*)(kimia::GLfloat)>(resolve("glClearDepthf"));
  if (fn != nullptr) fn(static_cast<kimia::GLfloat>(depth));
}
#endif

#define LOAD(name) name##Fn = reinterpret_cast<decltype(name##Fn)>(resolver("gl" #name))
#endif  // !__EMSCRIPTEN__

#ifdef __EMSCRIPTEN__
// Emscripten links the WebGL2/GLES3 entry points directly (no dlopen), so
// wire the function pointers to the real symbols. Every entry point has an
// identical signature in desktop GL and GLES3 except the two shimmed below.
#define WIRE(member, glname) member##Fn = reinterpret_cast<decltype(member##Fn)>(::glname)

// On wasm32 the pointer-sized GL size types are 32-bit while the engine's
// local GLsizeiptr is i64, so forwarding byte sizes directly would break the
// ABI. glClearDepth (double) is glClearDepthf (float) in GLES too.
void bufferDataShim(kimia::GLenum target, kimia::GLsizeiptr size, const void* data, kimia::GLenum usage) {
  ::glBufferData(target, static_cast<::GLsizeiptr>(size), data, usage);
}
void clearDepthShim(kimia::f64 depth) { ::glClearDepthf(static_cast<GLfloat>(depth)); }
#endif
}  // namespace

bool GLFunctions::load(GLGetProcFn proc) {
  if (loaded_) return true;

#ifdef __EMSCRIPTEN__
  (void)proc;
  WIRE(createShader, glCreateShader);
  WIRE(shaderSource, glShaderSource);
  WIRE(compileShader, glCompileShader);
  WIRE(getShaderiv, glGetShaderiv);
  WIRE(getShaderInfoLog, glGetShaderInfoLog);
  WIRE(createProgram, glCreateProgram);
  WIRE(attachShader, glAttachShader);
  WIRE(linkProgram, glLinkProgram);
  WIRE(getProgramiv, glGetProgramiv);
  WIRE(getProgramInfoLog, glGetProgramInfoLog);
  WIRE(useProgram, glUseProgram);
  WIRE(deleteShader, glDeleteShader);
  WIRE(deleteProgram, glDeleteProgram);
  WIRE(genVertexArrays, glGenVertexArrays);
  WIRE(bindVertexArray, glBindVertexArray);
  WIRE(deleteVertexArrays, glDeleteVertexArrays);
  WIRE(genBuffers, glGenBuffers);
  WIRE(bindBuffer, glBindBuffer);
  bufferDataFn = bufferDataShim;
  WIRE(deleteBuffers, glDeleteBuffers);
  WIRE(enableVertexAttribArray, glEnableVertexAttribArray);
  WIRE(vertexAttribPointer, glVertexAttribPointer);
  WIRE(getUniformLocation, glGetUniformLocation);
  WIRE(uniformMatrix4fv, glUniformMatrix4fv);
  WIRE(uniform3f, glUniform3f);
  WIRE(uniform1f, glUniform1f);
  WIRE(uniform1i, glUniform1i);
  WIRE(activeTexture, glActiveTexture);
  WIRE(genTextures, glGenTextures);
  WIRE(bindTexture, glBindTexture);
  WIRE(texImage2D, glTexImage2D);
  WIRE(texParameteri, glTexParameteri);
  WIRE(generateMipmap, glGenerateMipmap);
  WIRE(deleteTextures, glDeleteTextures);
  WIRE(viewport, glViewport);
  WIRE(clear, glClear);
  WIRE(clearColor, glClearColor);
  clearDepthFn = clearDepthShim;
  WIRE(enable, glEnable);
  WIRE(disable, glDisable);
  WIRE(depthFunc, glDepthFunc);
  WIRE(cullFace, glCullFace);
  WIRE(frontFace, glFrontFace);
  WIRE(drawElements, glDrawElements);
  WIRE(drawArrays, glDrawArrays);
  WIRE(readPixels, glReadPixels);
  WIRE(getIntegerv, glGetIntegerv);
  WIRE(getString, glGetString);
  WIRE(getError, glGetError);
  WIRE(genFramebuffers, glGenFramebuffers);
  WIRE(bindFramebuffer, glBindFramebuffer);
  WIRE(framebufferTexture2D, glFramebufferTexture2D);
  WIRE(checkFramebufferStatus, glCheckFramebufferStatus);
  WIRE(deleteFramebuffers, glDeleteFramebuffers);
  WIRE(pixelStorei, glPixelStorei);
  WIRE(depthMask, glDepthMask);
  WIRE(blendFunc, glBlendFunc);
#undef WIRE
  loaded_ = true;
  return true;
#endif

  GLGetProcFn resolver = proc;
  if (resolver == nullptr) {
#ifdef _WIN32
    handle_ = reinterpret_cast<void*>(::LoadLibraryA("opengl32.dll"));
#elif defined(__ANDROID__)
    // Android exposes GLES3 (and ES2) directly; there is no desktop libGL.
    handle_ = dlopen("libGLESv3.so", RTLD_NOW | RTLD_LOCAL);
    if (handle_ == nullptr) handle_ = dlopen("libGLESv2.so", RTLD_NOW | RTLD_LOCAL);
#else
    handle_ = dlopen("libGL.so.1", RTLD_NOW | RTLD_LOCAL);
    if (handle_ == nullptr) handle_ = dlopen("libGL.so", RTLD_NOW | RTLD_LOCAL);
#endif
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
#ifdef __ANDROID__
  bufferDataFn = bufferDataShim;
#else
  LOAD(bufferData);
#endif
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
#ifdef __ANDROID__
  clearDepthFn = clearDepthShim;
#else
  LOAD(clearDepth);
#endif
  LOAD(enable);
  LOAD(disable);
  LOAD(depthFunc);
  LOAD(cullFace);
  LOAD(frontFace);
  LOAD(drawElements);
  LOAD(drawArrays);
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
  LOAD(blendFunc);

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
#ifndef __EMSCRIPTEN__
  if (handle_ != nullptr) {
#ifdef _WIN32
    ::FreeLibrary(reinterpret_cast<HMODULE>(handle_));
#else
    dlclose(handle_);
#endif
    handle_ = nullptr;
  }
#endif
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
  drawArraysFn = nullptr;
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
  blendFuncFn = nullptr;
}

}  // namespace kimia
