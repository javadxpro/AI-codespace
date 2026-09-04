#pragma once

#include <kimia/GLFunctions.h>
#include <kimia/GraphicsTypes.h>

namespace kimia {

// GPU upload of a MeshData: VAO + VBO (interleaved pos/normal/uv) + EBO.
class GpuMesh {
public:
  GpuMesh() = default;
  ~GpuMesh();
  GpuMesh(const GpuMesh&) = delete;
  GpuMesh& operator=(const GpuMesh&) = delete;
  GpuMesh(GpuMesh&& other) noexcept;
  GpuMesh& operator=(GpuMesh&& other) noexcept;

  bool upload(const MeshData& mesh);
  void destroy();
  bool valid() const { return vao_ != 0; }
  void draw() const;

private:
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint ebo_ = 0;
  GLsizei indexCount_ = 0;
};

}  // namespace kimia
