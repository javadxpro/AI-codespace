#pragma once

#include <kimia/Types.h>

namespace kimia {

// Minimal GL types (kept local so nothing outside depends on GL headers).
using GLenum = u32;
using GLuint = u32;
using GLint = i32;
using GLsizei = i32;
using GLfloat = f32;
using GLbitfield = u32;
using GLboolean = u8;
using GLchar = char;
using GLsizeiptr = i64;
using GLintptr = i64;
using GLubyte = u8;

enum : GLenum {
  GL_VERTEX_SHADER = 0x8B31,
  GL_FRAGMENT_SHADER = 0x8B30,
  GL_COMPILE_STATUS = 0x8B81,
  GL_LINK_STATUS = 0x8B82,
  GL_ARRAY_BUFFER = 0x8892,
  GL_ELEMENT_ARRAY_BUFFER = 0x8893,
  GL_STATIC_DRAW = 0x88E4,
  GL_FLOAT = 0x1406,
  GL_UNSIGNED_INT = 0x1405,
  GL_TRIANGLES = 0x0004,
  GL_DEPTH_BUFFER_BIT = 0x00000100,
  GL_COLOR_BUFFER_BIT = 0x00004000,
  GL_DEPTH_TEST = 0x0B71,
  GL_CULL_FACE = 0x0B44,
  GL_BACK = 0x0405,
  GL_CCW = 0x0901,
  GL_LEQUAL = 0x0203,
  GL_TEXTURE_2D = 0x0DE1,
  GL_TEXTURE0 = 0x84C0,
  GL_RGBA = 0x1908,
  GL_RGB = 0x1907,
  GL_UNSIGNED_BYTE = 0x1401,
  GL_TEXTURE_MIN_FILTER = 0x2801,
  GL_TEXTURE_MAG_FILTER = 0x2800,
  GL_TEXTURE_WRAP_S = 0x2802,
  GL_TEXTURE_WRAP_T = 0x2803,
  GL_LINEAR = 0x2601,
  GL_LINEAR_MIPMAP_LINEAR = 0x2703,
  GL_REPEAT = 0x2901,
  GL_FRAMEBUFFER = 0x8D40,
  GL_DEPTH_ATTACHMENT = 0x8D00,
  GL_FRAMEBUFFER_COMPLETE = 0x8CD5,
  GL_DEPTH_COMPONENT = 0x1902,
  GL_DEPTH_COMPONENT24 = 0x81A6,
  GL_TEXTURE_COMPARE_MODE = 0x884C,
  GL_TEXTURE_COMPARE_FUNC = 0x884D,
  GL_COMPARE_REF_TO_TEXTURE = 0x884E,
  GL_PACK_ALIGNMENT = 0x0D05,
  GL_NO_ERROR = 0,
  GL_VERSION = 0x1F02,
  GL_RENDERER = 0x1F01,
};

using GLGetProcFn = void* (*)(const char* name);

// Runtime loader for the OpenGL 3.3 core subset used by the renderer.
// dlopen-based (this is the future Vulkan swap point): the renderer never
// links against a GL library at build time. Wrappers are safe no-ops when
// nothing was loaded, so the software fallback path needs no guards.
class GLFunctions {
public:
  static GLFunctions& instance();

  // Loads from `proc` (e.g. SDL_GL_GetProcAddress) or, when proc is null,
  // from dlopen("libGL.so.1" / "libGL.so"). Returns false if any required
  // entry point is missing.
  bool load(GLGetProcFn proc = nullptr);
  void unload();
  bool loaded() const { return loaded_; }

  GLuint createShader(GLenum type) const { return createShaderFn ? createShaderFn(type) : 0; }
  void shaderSource(GLuint shader, const GLchar* source) const {
    if (shaderSourceFn != nullptr) shaderSourceFn(shader, 1, &source, nullptr);
  }
  void compileShader(GLuint shader) const {
    if (compileShaderFn != nullptr) compileShaderFn(shader);
  }
  void getShaderiv(GLuint shader, GLenum name, GLint* value) const {
    if (getShaderivFn != nullptr) getShaderivFn(shader, name, value);
  }
  void getShaderInfoLog(GLuint shader, GLsizei size, GLsizei* length, GLchar* log) const {
    if (getShaderInfoLogFn != nullptr) getShaderInfoLogFn(shader, size, length, log);
  }
  GLuint createProgram() const { return createProgramFn ? createProgramFn() : 0; }
  void attachShader(GLuint program, GLuint shader) const {
    if (attachShaderFn != nullptr) attachShaderFn(program, shader);
  }
  void linkProgram(GLuint program) const {
    if (linkProgramFn != nullptr) linkProgramFn(program);
  }
  void getProgramiv(GLuint program, GLenum name, GLint* value) const {
    if (getProgramivFn != nullptr) getProgramivFn(program, name, value);
  }
  void getProgramInfoLog(GLuint program, GLsizei size, GLsizei* length, GLchar* log) const {
    if (getProgramInfoLogFn != nullptr) getProgramInfoLogFn(program, size, length, log);
  }
  void useProgram(GLuint program) const {
    if (useProgramFn != nullptr) useProgramFn(program);
  }
  void deleteShader(GLuint shader) const {
    if (deleteShaderFn != nullptr) deleteShaderFn(shader);
  }
  void deleteProgram(GLuint program) const {
    if (deleteProgramFn != nullptr) deleteProgramFn(program);
  }
  void genVertexArrays(GLsizei n, GLuint* arrays) const {
    if (genVertexArraysFn != nullptr) genVertexArraysFn(n, arrays);
  }
  void bindVertexArray(GLuint array) const {
    if (bindVertexArrayFn != nullptr) bindVertexArrayFn(array);
  }
  void deleteVertexArrays(GLsizei n, const GLuint* arrays) const {
    if (deleteVertexArraysFn != nullptr) deleteVertexArraysFn(n, arrays);
  }
  void genBuffers(GLsizei n, GLuint* buffers) const {
    if (genBuffersFn != nullptr) genBuffersFn(n, buffers);
  }
  void bindBuffer(GLenum target, GLuint buffer) const {
    if (bindBufferFn != nullptr) bindBufferFn(target, buffer);
  }
  void bufferData(GLenum target, i64 size, const void* data, GLenum usage) const {
    if (bufferDataFn != nullptr) bufferDataFn(target, size, data, usage);
  }
  void deleteBuffers(GLsizei n, const GLuint* buffers) const {
    if (deleteBuffersFn != nullptr) deleteBuffersFn(n, buffers);
  }
  void enableVertexAttribArray(GLuint index) const {
    if (enableVertexAttribArrayFn != nullptr) enableVertexAttribArrayFn(index);
  }
  void vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
                           i64 offset) const {
    if (vertexAttribPointerFn != nullptr) {
      vertexAttribPointerFn(index, size, type, normalized, stride, reinterpret_cast<const void*>(offset));
    }
  }
  GLint getUniformLocation(GLuint program, const GLchar* name) const {
    return getUniformLocationFn ? getUniformLocationFn(program, name) : -1;
  }
  void uniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) const {
    if (uniformMatrix4fvFn != nullptr) uniformMatrix4fvFn(location, count, transpose, value);
  }
  void uniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z) const {
    if (uniform3fFn != nullptr) uniform3fFn(location, x, y, z);
  }
  void uniform1f(GLint location, GLfloat x) const {
    if (uniform1fFn != nullptr) uniform1fFn(location, x);
  }
  void uniform1i(GLint location, GLint x) const {
    if (uniform1iFn != nullptr) uniform1iFn(location, x);
  }
  void activeTexture(GLenum texture) const {
    if (activeTextureFn != nullptr) activeTextureFn(texture);
  }
  void genTextures(GLsizei n, GLuint* textures) const {
    if (genTexturesFn != nullptr) genTexturesFn(n, textures);
  }
  void bindTexture(GLenum target, GLuint texture) const {
    if (bindTextureFn != nullptr) bindTextureFn(target, texture);
  }
  void texImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const void* pixels) const {
    if (texImage2DFn != nullptr) {
      texImage2DFn(target, level, internalFormat, width, height, border, format, type, pixels);
    }
  }
  void texParameteri(GLenum target, GLenum name, GLint value) const {
    if (texParameteriFn != nullptr) texParameteriFn(target, name, value);
  }
  void generateMipmap(GLenum target) const {
    if (generateMipmapFn != nullptr) generateMipmapFn(target);
  }
  void deleteTextures(GLsizei n, const GLuint* textures) const {
    if (deleteTexturesFn != nullptr) deleteTexturesFn(n, textures);
  }
  void viewport(GLint x, GLint y, GLsizei width, GLsizei height) const {
    if (viewportFn != nullptr) viewportFn(x, y, width, height);
  }
  void clear(GLbitfield mask) const {
    if (clearFn != nullptr) clearFn(mask);
  }
  void clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) const {
    if (clearColorFn != nullptr) clearColorFn(r, g, b, a);
  }
  void clearDepth(f64 depth) const {
    if (clearDepthFn != nullptr) clearDepthFn(static_cast<f64>(depth));
  }
  void enable(GLenum cap) const {
    if (enableFn != nullptr) enableFn(cap);
  }
  void disable(GLenum cap) const {
    if (disableFn != nullptr) disableFn(cap);
  }
  void depthFunc(GLenum func) const {
    if (depthFuncFn != nullptr) depthFuncFn(func);
  }
  void cullFace(GLenum mode) const {
    if (cullFaceFn != nullptr) cullFaceFn(mode);
  }
  void frontFace(GLenum mode) const {
    if (frontFaceFn != nullptr) frontFaceFn(mode);
  }
  void drawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) const {
    if (drawElementsFn != nullptr) drawElementsFn(mode, count, type, indices);
  }
  void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels) const {
    if (readPixelsFn != nullptr) readPixelsFn(x, y, width, height, format, type, pixels);
  }
  void getIntegerv(GLenum name, GLint* value) const {
    if (getIntegervFn != nullptr) getIntegervFn(name, value);
  }
  const GLchar* getString(GLenum name) const {
    return getStringFn ? reinterpret_cast<const GLchar*>(getStringFn(name)) : nullptr;
  }
  GLenum getError() const { return getErrorFn ? getErrorFn() : GL_NO_ERROR; }
  void genFramebuffers(GLsizei n, GLuint* framebuffers) const {
    if (genFramebuffersFn != nullptr) genFramebuffersFn(n, framebuffers);
  }
  void bindFramebuffer(GLenum target, GLuint framebuffer) const {
    if (bindFramebufferFn != nullptr) bindFramebufferFn(target, framebuffer);
  }
  void framebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) const {
    if (framebufferTexture2DFn != nullptr) framebufferTexture2DFn(target, attachment, textarget, texture, level);
  }
  GLenum checkFramebufferStatus(GLenum target) const {
    return checkFramebufferStatusFn ? checkFramebufferStatusFn(target) : 0;
  }
  void deleteFramebuffers(GLsizei n, const GLuint* framebuffers) const {
    if (deleteFramebuffersFn != nullptr) deleteFramebuffersFn(n, framebuffers);
  }
  void pixelStorei(GLenum name, GLint value) const {
    if (pixelStoreiFn != nullptr) pixelStoreiFn(name, value);
  }
  void depthMask(GLboolean flag) const {
    if (depthMaskFn != nullptr) depthMaskFn(flag);
  }

private:
  void* handle_ = nullptr;
  bool loaded_ = false;
  GLuint (*createShaderFn)(GLenum) = nullptr;
  void (*shaderSourceFn)(GLuint, GLsizei, const GLchar* const*, const GLint*) = nullptr;
  void (*compileShaderFn)(GLuint) = nullptr;
  void (*getShaderivFn)(GLuint, GLenum, GLint*) = nullptr;
  void (*getShaderInfoLogFn)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
  GLuint (*createProgramFn)() = nullptr;
  void (*attachShaderFn)(GLuint, GLuint) = nullptr;
  void (*linkProgramFn)(GLuint) = nullptr;
  void (*getProgramivFn)(GLuint, GLenum, GLint*) = nullptr;
  void (*getProgramInfoLogFn)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
  void (*useProgramFn)(GLuint) = nullptr;
  void (*deleteShaderFn)(GLuint) = nullptr;
  void (*deleteProgramFn)(GLuint) = nullptr;
  void (*genVertexArraysFn)(GLsizei, GLuint*) = nullptr;
  void (*bindVertexArrayFn)(GLuint) = nullptr;
  void (*deleteVertexArraysFn)(GLsizei, const GLuint*) = nullptr;
  void (*genBuffersFn)(GLsizei, GLuint*) = nullptr;
  void (*bindBufferFn)(GLenum, GLuint) = nullptr;
  void (*bufferDataFn)(GLenum, GLsizeiptr, const void*, GLenum) = nullptr;
  void (*deleteBuffersFn)(GLsizei, const GLuint*) = nullptr;
  void (*enableVertexAttribArrayFn)(GLuint) = nullptr;
  void (*vertexAttribPointerFn)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) = nullptr;
  GLint (*getUniformLocationFn)(GLuint, const GLchar*) = nullptr;
  void (*uniformMatrix4fvFn)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
  void (*uniform3fFn)(GLint, GLfloat, GLfloat, GLfloat) = nullptr;
  void (*uniform1fFn)(GLint, GLfloat) = nullptr;
  void (*uniform1iFn)(GLint, GLint) = nullptr;
  void (*activeTextureFn)(GLenum) = nullptr;
  void (*genTexturesFn)(GLsizei, GLuint*) = nullptr;
  void (*bindTextureFn)(GLenum, GLuint) = nullptr;
  void (*texImage2DFn)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) = nullptr;
  void (*texParameteriFn)(GLenum, GLenum, GLint) = nullptr;
  void (*generateMipmapFn)(GLenum) = nullptr;
  void (*deleteTexturesFn)(GLsizei, const GLuint*) = nullptr;
  void (*viewportFn)(GLint, GLint, GLsizei, GLsizei) = nullptr;
  void (*clearFn)(GLbitfield) = nullptr;
  void (*clearColorFn)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
  void (*clearDepthFn)(f64) = nullptr;
  void (*enableFn)(GLenum) = nullptr;
  void (*disableFn)(GLenum) = nullptr;
  void (*depthFuncFn)(GLenum) = nullptr;
  void (*cullFaceFn)(GLenum) = nullptr;
  void (*frontFaceFn)(GLenum) = nullptr;
  void (*drawElementsFn)(GLenum, GLsizei, GLenum, const void*) = nullptr;
  void (*readPixelsFn)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*) = nullptr;
  void (*getIntegervFn)(GLenum, GLint*) = nullptr;
  const GLubyte* (*getStringFn)(GLenum) = nullptr;
  GLenum (*getErrorFn)() = nullptr;
  void (*genFramebuffersFn)(GLsizei, GLuint*) = nullptr;
  void (*bindFramebufferFn)(GLenum, GLuint) = nullptr;
  void (*framebufferTexture2DFn)(GLenum, GLenum, GLenum, GLuint, GLint) = nullptr;
  GLenum (*checkFramebufferStatusFn)(GLenum) = nullptr;
  void (*deleteFramebuffersFn)(GLsizei, const GLuint*) = nullptr;
  void (*pixelStoreiFn)(GLenum, GLint) = nullptr;
  void (*depthMaskFn)(GLboolean) = nullptr;
};

}  // namespace kimia
