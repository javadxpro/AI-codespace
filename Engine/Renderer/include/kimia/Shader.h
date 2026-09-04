#pragma once

#include <kimia/GLFunctions.h>
#include <kimia/Mat4.h>
#include <kimia/Vec.h>

#include <string>

namespace kimia {

// A compiled+linked GLSL program. Safe when GL is unavailable: compile()
// returns false with a message and everything else is a no-op.
class Shader {
public:
  Shader() = default;
  ~Shader();
  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;
  Shader(Shader&& other) noexcept;
  Shader& operator=(Shader&& other) noexcept;

  bool compile(const char* vertexSource, const char* fragmentSource, std::string& error);
  void destroy();
  bool valid() const { return program_ != 0; }
  void use() const;

  void setMat4(const char* name, const Mat4& value) const;
  void setVec3(const char* name, const Vec3& value) const;
  void setFloat(const char* name, f64 value) const;
  void setInt(const char* name, i32 value) const;

private:
  GLuint program_ = 0;
};

}  // namespace kimia
