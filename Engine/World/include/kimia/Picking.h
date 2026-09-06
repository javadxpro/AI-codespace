#pragma once

#include <kimia/Mat4.h>
#include <kimia/Scene.h>
#include <kimia/Types.h>
#include <kimia/Vec.h>

#include <string>
#include <vector>

namespace kimia {
namespace pick {

// --- Touching the world: turning a tap into an object ---
//
// The editor's viewport is a picture on a phone. Everything here answers
// two questions about that picture:
//
//   * "I tapped at (x, y) — what did I hit?"
//   * "I dragged from (x, y) to (x2, y2) — how far did the thing move?"
//
// It is deliberately free of the renderer and of WorldEditor: given a
// camera and a list of boxes it does pure geometry, so every answer can be
// checked without drawing a single pixel.

// Where the camera is and how it sees, as the viewport already has it.
struct Viewport {
  Mat4 view;
  Mat4 projection;
  Vec3 eye{0.0, 0.0, 0.0};
  i32 width = 640;
  i32 height = 480;
};

// A ray through a pixel, pointing away from the camera.
struct Ray {
  Vec3 origin{0.0, 0.0, 0.0};
  Vec3 direction{0.0, 0.0, 1.0};  // normalised
};

// The ray under a screen pixel. (0,0) is the top-left of the image, which
// is how a browser reports a touch.
Ray rayThroughPixel(const Viewport& viewport, f64 pixelX, f64 pixelY);

// One thing a tap can land on: an entity's box in world space.
struct Target {
  std::string name;
  Vec3 center{0.0, 0.0, 0.0};
  Vec3 halfExtents{0.5, 0.5, 0.5};
};

struct Hit {
  bool hit = false;
  std::string name;
  f64 distance = 0.0;
  Vec3 point{0.0, 0.0, 0.0};
};

// The NEAREST target under the pixel. Ties go to whatever is closest to
// the camera, which is what a person means by "that one".
Hit pickAt(const Viewport& viewport, const std::vector<Target>& targets, f64 pixelX, f64 pixelY);

// Builds the pick list from a scene. An entity's box comes from its
// transform, with a floor under the size so a flat or tiny object is
// still tappable — an editor where you cannot select something because it
// is thin is a broken editor.
std::vector<Target> targetsFromScene(const Scene& scene);

// --- Dragging ---
//
// A drag moves an object across the GROUND PLANE at its own height, not
// through the air toward the camera. That is what people expect when they
// slide something around a scene, and it keeps objects on the floor
// instead of sending them into the sky.

// Beyond this the answer is useless in an editor: a ray grazing the
// horizon technically meets the floor hundreds of metres away, and
// honouring that would fling a dragged object off the map.
inline constexpr f64 kMaxPlaneReach = 200.0;

// Where a pixel lands on the horizontal plane at `planeY`. Returns false
// when the ray runs parallel to the plane, points away from it, or meets
// it so far off that the answer is meaningless.
bool pixelOnPlane(const Viewport& viewport, f64 pixelX, f64 pixelY, f64 planeY, Vec3& out);

// How far to move an object dragged from one pixel to another, keeping it
// at the same height. False when either end of the drag misses the plane.
bool dragDelta(const Viewport& viewport, f64 fromX, f64 fromY, f64 toX, f64 toY, f64 planeY, Vec3& out);

// Rounds a position to a grid, for snapping. A step of 0 leaves it alone.
Vec3 snapTo(const Vec3& position, f64 step);

}  // namespace pick
}  // namespace kimia
