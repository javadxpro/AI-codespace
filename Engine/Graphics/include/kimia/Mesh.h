#pragma once

#include <kimia/GraphicsTypes.h>

#include <string>

namespace kimia {

// --- Primitive meshes (all CCW from outside, per-face normals) ---

// Cube centered at origin. 24 vertices / 36 indices (4 verts per face, no
// sharing, correct per-face normals). `size` is the full edge length.
MeshData makeCube(f64 size = 1.0);

// Quad in the XZ plane (facing +Y), centered at origin.
// 4 vertices / 6 indices.
MeshData makePlane(f64 width = 1.0, f64 depth = 1.0);

// UV sphere built as a latitude grid with a duplicated seam column (pole rows
// are rings of points so every vertex carries its own normal/uv).
// verts = (rings + 1) * (segments + 1), indices = rings * segments * 6.
// The reference sphere makeSphere(16, 8) = 153 vertices / 768 indices.
MeshData makeSphere(u32 rings = 16, u32 segments = 8);

// --- Wavefront OBJ (subset) ---
// Supports `v`, `vn`, `vt`, `f` (triangles and n-gon fans, negative indices,
// v/vt/vn triplets), `o`/`g` (name), `#` comments and blank lines. Faces are
// kept as authored (OBJ is CCW). UVs are stored with the OBJ convention
// v-flipped (0,0 at bottom) to match image row order (0,0 at top).
// `dedupe` merges identical vertex tuples (position+normal+uv).
bool loadFromOBJText(const std::string& text, MeshData& out, std::string& error, bool dedupe = false);
bool loadFromOBJFile(const std::string& path, MeshData& out, std::string& error, bool dedupe = false);

// Merges vertices with identical position+normal+uv tuples (lossless, used by
// the FBX importer and available for any mesh).
void dedupeVertices(MeshData& mesh);

// --- KIMIA mesh text format v1 (round-trip safe) ---
//
//   # KIMIA mesh v1
//   name Cube
//   positions 24
//   x y z
//   ...
//   normals 24
//   ...
//   uvs 24
//   u v
//   ...
//   indices 36
//   a b c
//   ...
//
// `#` lines are comments; unknown keywords are skipped (tolerant load).
bool meshToText(const MeshData& mesh, std::string& out);
bool meshFromText(const std::string& text, MeshData& out, std::string& error);

}  // namespace kimia
