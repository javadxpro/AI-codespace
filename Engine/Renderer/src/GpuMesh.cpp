#include <kimia/GpuMesh.h>

#include <vector>

namespace kimia {

GpuMesh::~GpuMesh() { destroy(); }

GpuMesh::GpuMesh(GpuMesh&& other) noexcept
    : vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_), indexCount_(other.indexCount_) {
  other.vao_ = 0;
  other.vbo_ = 0;
  other.ebo_ = 0;
  other.indexCount_ = 0;
}

GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept {
  if (this != &other) {
    destroy();
    vao_ = other.vao_;
    vbo_ = other.vbo_;
    ebo_ = other.ebo_;
    indexCount_ = other.indexCount_;
    other.vao_ = 0;
    other.vbo_ = 0;
    other.ebo_ = 0;
    other.indexCount_ = 0;
  }
  return *this;
}

bool GpuMesh::upload(const MeshData& mesh) {
  destroy();
  GLFunctions& gl = GLFunctions::instance();
  if (!gl.loaded() || !mesh.isValid()) return false;

  // Interleaved layout: pos(3f) + normal(3f) + uv(2f).
  std::vector<GLfloat> vertices;
  vertices.reserve(mesh.positions.size() * 8U);
  for (usize i = 0; i < mesh.positions.size(); ++i) {
    const Vec3& p = mesh.positions[i];
    const Vec3& n = mesh.normals[i];
    const Vec2 uv = mesh.uvs.empty() ? Vec2{} : mesh.uvs[i];
    vertices.push_back(static_cast<GLfloat>(p.x));
    vertices.push_back(static_cast<GLfloat>(p.y));
    vertices.push_back(static_cast<GLfloat>(p.z));
    vertices.push_back(static_cast<GLfloat>(n.x));
    vertices.push_back(static_cast<GLfloat>(n.y));
    vertices.push_back(static_cast<GLfloat>(n.z));
    vertices.push_back(static_cast<GLfloat>(uv.x));
    vertices.push_back(static_cast<GLfloat>(uv.y));
  }

  gl.genVertexArrays(1, &vao_);
  gl.genBuffers(1, &vbo_);
  gl.genBuffers(1, &ebo_);
  if (vao_ == 0 || vbo_ == 0 || ebo_ == 0) {
    destroy();
    return false;
  }
  gl.bindVertexArray(vao_);
  gl.bindBuffer(GL_ARRAY_BUFFER, vbo_);
  gl.bufferData(GL_ARRAY_BUFFER, static_cast<i64>(vertices.size() * sizeof(GLfloat)), vertices.data(), GL_STATIC_DRAW);
  gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<i64>(mesh.indices.size() * sizeof(u32)), mesh.indices.data(),
                GL_STATIC_DRAW);

  const GLsizei stride = static_cast<GLsizei>(8 * sizeof(GLfloat));
  gl.enableVertexAttribArray(0);
  gl.vertexAttribPointer(0, 3, GL_FLOAT, 0, stride, 0);
  gl.enableVertexAttribArray(1);
  gl.vertexAttribPointer(1, 3, GL_FLOAT, 0, stride, static_cast<i64>(3 * sizeof(GLfloat)));
  gl.enableVertexAttribArray(2);
  gl.vertexAttribPointer(2, 2, GL_FLOAT, 0, stride, static_cast<i64>(6 * sizeof(GLfloat)));
  gl.bindVertexArray(0);
  gl.bindBuffer(GL_ARRAY_BUFFER, 0);
  gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  indexCount_ = static_cast<GLsizei>(mesh.indices.size());
  return true;
}

void GpuMesh::destroy() {
  GLFunctions& gl = GLFunctions::instance();
  if (ebo_ != 0) {
    gl.deleteBuffers(1, &ebo_);
    ebo_ = 0;
  }
  if (vbo_ != 0) {
    gl.deleteBuffers(1, &vbo_);
    vbo_ = 0;
  }
  if (vao_ != 0) {
    gl.deleteVertexArrays(1, &vao_);
    vao_ = 0;
  }
  indexCount_ = 0;
}

void GpuMesh::draw() const {
  GLFunctions& gl = GLFunctions::instance();
  if (vao_ == 0 || indexCount_ == 0) return;
  gl.bindVertexArray(vao_);
  gl.drawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
  gl.bindVertexArray(0);
}

}  // namespace kimia
