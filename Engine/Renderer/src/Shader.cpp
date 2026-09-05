#include <kimia/Shader.h>

#include <cstring>
#include <utility>
#include <vector>

namespace kimia {

namespace {

std::vector<GLfloat> toFloatArray(const Mat4& value) {
  std::vector<GLfloat> data(16);
  for (i32 i = 0; i < 16; ++i) {
    data[static_cast<usize>(i)] = static_cast<GLfloat>(value.m_[static_cast<usize>(i)]);
  }
  return data;
}

}  // namespace

Shader::~Shader() { destroy(); }

Shader::Shader(Shader&& other) noexcept : program_(other.program_) { other.program_ = 0; }

Shader& Shader::operator=(Shader&& other) noexcept {
  if (this != &other) {
    destroy();
    program_ = other.program_;
    other.program_ = 0;
  }
  return *this;
}

bool Shader::compile(const char* vertexSource, const char* fragmentSource, std::string& error) {
  destroy();
  GLFunctions& gl = GLFunctions::instance();
  if (!gl.loaded()) {
    error = "no OpenGL available";
    return false;
  }

  const GLuint vertexShader = gl.createShader(GL_VERTEX_SHADER);
  const GLuint fragmentShader = gl.createShader(GL_FRAGMENT_SHADER);
  if (vertexShader == 0 || fragmentShader == 0) {
    error = "glCreateShader failed";
    if (vertexShader != 0) gl.deleteShader(vertexShader);
    if (fragmentShader != 0) gl.deleteShader(fragmentShader);
    return false;
  }

  gl.shaderSource(vertexShader, vertexSource);
  gl.compileShader(vertexShader);
  GLint status = 0;
  gl.getShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
  if (status == 0) {
    char log[2048];
    gl.getShaderInfoLog(vertexShader, sizeof(log) - 1, nullptr, log);
    error = std::string("vertex shader compile failed: ") + log;
    gl.deleteShader(vertexShader);
    gl.deleteShader(fragmentShader);
    return false;
  }

  gl.shaderSource(fragmentShader, fragmentSource);
  gl.compileShader(fragmentShader);
  gl.getShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
  if (status == 0) {
    char log[2048];
    gl.getShaderInfoLog(fragmentShader, sizeof(log) - 1, nullptr, log);
    error = std::string("fragment shader compile failed: ") + log;
    gl.deleteShader(vertexShader);
    gl.deleteShader(fragmentShader);
    return false;
  }

  program_ = gl.createProgram();
  gl.attachShader(program_, vertexShader);
  gl.attachShader(program_, fragmentShader);
  gl.linkProgram(program_);
  gl.getProgramiv(program_, GL_LINK_STATUS, &status);
  gl.deleteShader(vertexShader);
  gl.deleteShader(fragmentShader);
  if (status == 0) {
    char log[2048];
    gl.getProgramInfoLog(program_, sizeof(log) - 1, nullptr, log);
    error = std::string("program link failed: ") + log;
    gl.deleteProgram(program_);
    program_ = 0;
    return false;
  }
  return true;
}

void Shader::destroy() {
  if (program_ != 0) {
    GLFunctions::instance().deleteProgram(program_);
    program_ = 0;
  }
}

void Shader::use() const { GLFunctions::instance().useProgram(program_); }

void Shader::setMat4(const char* name, const Mat4& value) const {
  if (program_ == 0) return;
  GLFunctions& gl = GLFunctions::instance();
  const GLint location = gl.getUniformLocation(program_, name);
  if (location < 0) return;
  const std::vector<GLfloat> data = toFloatArray(value);
  gl.uniformMatrix4fv(location, 1, 0, data.data());
}

void Shader::setVec3(const char* name, const Vec3& value) const {
  if (program_ == 0) return;
  GLFunctions& gl = GLFunctions::instance();
  const GLint location = gl.getUniformLocation(program_, name);
  if (location < 0) return;
  gl.uniform3f(location, static_cast<GLfloat>(value.x), static_cast<GLfloat>(value.y), static_cast<GLfloat>(value.z));
}

void Shader::setFloat(const char* name, f64 value) const {
  if (program_ == 0) return;
  GLFunctions& gl = GLFunctions::instance();
  const GLint location = gl.getUniformLocation(program_, name);
  if (location < 0) return;
  gl.uniform1f(location, static_cast<GLfloat>(value));
}

void Shader::setInt(const char* name, i32 value) const {
  if (program_ == 0) return;
  GLFunctions& gl = GLFunctions::instance();
  const GLint location = gl.getUniformLocation(program_, name);
  if (location < 0) return;
  gl.uniform1i(location, value);
}

}  // namespace kimia
