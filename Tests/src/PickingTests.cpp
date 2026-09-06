#include <kimia/MathUtils.h>
#include <kimia/Picking.h>
#include <kimia_test.h>

#include <cmath>
#include <string>
#include <vector>

namespace {

using kimia::Mat4;
using kimia::Vec3;
using kimia::f64;
using kimia::i32;
using kimia::pick::Hit;
using kimia::pick::Ray;
using kimia::pick::Target;
using kimia::pick::Viewport;

bool near(f64 a, f64 b, f64 tolerance = 1e-6) { return std::abs(a - b) <= tolerance; }

// A camera looking down at the origin from behind, the way the editor
// viewport actually sits.
Viewport editorCamera() {
  Viewport viewport;
  viewport.eye = Vec3{0.0, 6.0, 10.0};
  viewport.view = Mat4::lookAt(viewport.eye, Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 1.0, 0.0});
  viewport.projection = Mat4::perspective(kimia::radians(60.0), 640.0 / 480.0, 0.1, 100.0);
  viewport.width = 640;
  viewport.height = 480;
  return viewport;
}

Target box(const char* name, const Vec3& at, f64 half = 0.5) {
  Target target;
  target.name = name;
  target.center = at;
  target.halfExtents = Vec3{half, half, half};
  return target;
}

// Finds the pixel that looks at a world point, by searching the screen.
// Tests then "tap" that pixel, which is what a finger really does.
bool pixelFor(const Viewport& viewport, const Vec3& world, f64& outX, f64& outY) {
  f64 best = 1e9;
  for (i32 y = 0; y < viewport.height; y += 2) {
    for (i32 x = 0; x < viewport.width; x += 2) {
      Vec3 on;
      if (!kimia::pick::pixelOnPlane(viewport, x, y, world.y, on)) continue;
      const f64 distance = (on - world).length();
      if (distance >= best) continue;
      best = distance;
      outX = x;
      outY = y;
    }
  }
  return best < 0.5;
}

}  // namespace

// --- The live viewport: turning a tap into an object ---

KIMIA_TEST(picking_the_centre_pixel_looks_at_what_the_camera_looks_at) {
  const Viewport viewport = editorCamera();
  Vec3 landed;
  KIMIA_REQUIRE(kimia::pick::pixelOnPlane(viewport, 320.0, 240.0, 0.0, landed));
  // The camera is aimed at the origin, so the middle of the picture is it.
  KIMIA_REQUIRE(near(landed.x, 0.0, 0.05));
  KIMIA_REQUIRE(near(landed.z, 0.0, 0.05));
  KIMIA_REQUIRE(near(landed.y, 0.0, 1e-9));

  // The ray points away from the camera and is a unit vector, or every
  // distance it reports would be wrong.
  const Ray ray = kimia::pick::rayThroughPixel(viewport, 320.0, 240.0);
  KIMIA_REQUIRE(near(ray.direction.length(), 1.0, 1e-9));
  KIMIA_REQUIRE(ray.direction.y < 0.0);  // looking downward
  KIMIA_REQUIRE(ray.direction.z < 0.0);  // and forward
}

KIMIA_TEST(picking_a_tap_finds_the_object_under_it) {
  const Viewport viewport = editorCamera();
  std::vector<Target> targets;
  targets.push_back(box("Left", Vec3{-3.0, 0.5, 0.0}));
  targets.push_back(box("Right", Vec3{3.0, 0.5, 0.0}));

  for (const Target& target : targets) {
    f64 x = 0.0;
    f64 y = 0.0;
    KIMIA_REQUIRE(pixelFor(viewport, target.center, x, y));
    const Hit hit = kimia::pick::pickAt(viewport, targets, x, y);
    KIMIA_REQUIRE(hit.hit);
    KIMIA_REQUIRE(hit.name == target.name);
    KIMIA_REQUIRE(hit.distance > 0.0);
  }

  // Tapping empty sky selects nothing rather than guessing.
  KIMIA_REQUIRE(!kimia::pick::pickAt(viewport, targets, 320.0, 2.0).hit);
  // So does tapping an empty patch of floor.
  KIMIA_REQUIRE(!kimia::pick::pickAt(viewport, targets, 320.0, 300.0).hit);
}

KIMIA_TEST(picking_takes_the_nearest_when_two_line_up) {
  // Two objects the SAME ray passes through, one behind the other. A
  // person means the front one.
  //
  // Building this needs care: my first attempt placed two boxes that only
  // looked stacked, and the ray never reached the far one at all — so the
  // test passed with the nearest-hit check deleted. These two are strung
  // along one ray on purpose.
  const Viewport viewport = editorCamera();
  const Ray ray = kimia::pick::rayThroughPixel(viewport, 320.0, 260.0);
  const Vec3 nearCentre = ray.origin + ray.direction * 9.0;
  const Vec3 farCentre = ray.origin + ray.direction * 14.0;

  std::vector<Target> targets;
  targets.push_back(box("Far", farCentre, 1.0));
  targets.push_back(box("Near", nearCentre, 1.0));
  // Both really are under that pixel, or there is nothing to resolve.
  KIMIA_REQUIRE(kimia::pick::pickAt(viewport, {box("Far", farCentre, 1.0)}, 320.0, 260.0).hit);
  KIMIA_REQUIRE(kimia::pick::pickAt(viewport, {box("Near", nearCentre, 1.0)}, 320.0, 260.0).hit);

  const Hit hit = kimia::pick::pickAt(viewport, targets, 320.0, 260.0);
  KIMIA_REQUIRE(hit.hit);
  KIMIA_REQUIRE(hit.name == "Near");

  // Listing them the other way round must not change the answer, or the
  // selection would depend on scene order rather than on what you see.
  std::vector<Target> reversed;
  reversed.push_back(box("Near", nearCentre, 1.0));
  reversed.push_back(box("Far", farCentre, 1.0));
  KIMIA_REQUIRE(kimia::pick::pickAt(viewport, reversed, 320.0, 260.0).name == "Near");
}

KIMIA_TEST(picking_a_thin_object_is_still_tappable) {
  // An editor where you cannot select something because it is flat is a
  // broken editor, so tiny boxes are padded out to a fingertip.
  kimia::Scene scene;
  kimia::EntityData flat;
  flat.name = "Sign";
  flat.transform.position = Vec3{0.0, 0.5, 0.0};
  flat.transform.scale = Vec3{1.0, 1.0, 0.01};  // paper thin
  scene.create(flat);
  kimia::EntityData ground;
  ground.name = "Ground";
  ground.transform.scale = Vec3{20.0, 0.1, 20.0};
  scene.create(ground);

  const std::vector<Target> targets = kimia::pick::targetsFromScene(scene);
  // The ground is the backdrop, not something you select by tapping the
  // middle of the screen — it would swallow every miss.
  KIMIA_REQUIRE(targets.size() == 1U);
  KIMIA_REQUIRE(targets[0].name == "Sign");
  KIMIA_REQUIRE(targets[0].halfExtents.z > 0.1);  // padded, not 0.005
}

KIMIA_TEST(picking_a_drag_slides_along_the_ground) {
  const Viewport viewport = editorCamera();
  Vec3 delta;
  KIMIA_REQUIRE(kimia::pick::dragDelta(viewport, 320.0, 300.0, 420.0, 300.0, 0.0, delta));
  // Dragging right moves the object right, and NEVER up: sliding
  // something across a scene should not launch it into the air.
  KIMIA_REQUIRE(delta.x > 0.5);
  KIMIA_REQUIRE(delta.y == 0.0);

  // The opposite drag is the mirror image.
  Vec3 back;
  KIMIA_REQUIRE(kimia::pick::dragDelta(viewport, 420.0, 300.0, 320.0, 300.0, 0.0, back));
  KIMIA_REQUIRE(near(back.x, -delta.x, 1e-9));

  // A drag that goes nowhere moves nothing.
  Vec3 still;
  KIMIA_REQUIRE(kimia::pick::dragDelta(viewport, 320.0, 300.0, 320.0, 300.0, 0.0, still));
  KIMIA_REQUIRE(near(still.x, 0.0, 1e-9));
  KIMIA_REQUIRE(near(still.z, 0.0, 1e-9));

  // Dragging an object that sits ABOVE the floor keeps it at its own
  // height. Both ends of a drag lie on the same plane, so asserting
  // delta.y == 0 on a ground-level drag proves nothing — the difference is
  // zero either way. Comparing a raised plane against the floor is what
  // actually shows the height is being held.
  Vec3 low;
  Vec3 high;
  KIMIA_REQUIRE(kimia::pick::dragDelta(viewport, 300.0, 300.0, 400.0, 320.0, 0.0, low));
  KIMIA_REQUIRE(kimia::pick::dragDelta(viewport, 300.0, 300.0, 400.0, 320.0, 3.0, high));
  KIMIA_REQUIRE(high.y == 0.0);
  KIMIA_REQUIRE(low.y == 0.0);
  // The same pixels sweep a different amount of ground at a different
  // height, which is the proof the plane height is really being used.
  KIMIA_REQUIRE(!near(low.x, high.x, 0.05));
}

KIMIA_TEST(picking_a_grazing_drag_is_refused) {
  // A ray near the horizon meets the floor hundreds of metres away.
  // Honouring that would fling the object off the map, so it is a miss.
  const Viewport viewport = editorCamera();
  Vec3 landed;
  KIMIA_REQUIRE(!kimia::pick::pixelOnPlane(viewport, 320.0, 5.0, 0.0, landed));
  Vec3 delta;
  KIMIA_REQUIRE(!kimia::pick::dragDelta(viewport, 320.0, 300.0, 320.0, 5.0, 0.0, delta));

  // A camera looking along the floor cannot place anything on it at all.
  Viewport level;
  level.eye = Vec3{0.0, 1.0, 10.0};
  level.view = Mat4::lookAt(level.eye, Vec3{0.0, 1.0, 0.0}, Vec3{0.0, 1.0, 0.0});
  level.projection = Mat4::perspective(kimia::radians(60.0), 4.0 / 3.0, 0.1, 100.0);
  level.width = 640;
  level.height = 480;
  KIMIA_REQUIRE(!kimia::pick::pixelOnPlane(level, 320.0, 240.0, 1.0, landed));
}

KIMIA_TEST(picking_snaps_to_a_grid_without_changing_height) {
  const Vec3 loose{1.34, 2.0, -0.71};
  const Vec3 snapped = kimia::pick::snapTo(loose, 0.5);
  KIMIA_REQUIRE(near(snapped.x, 1.5));
  KIMIA_REQUIRE(near(snapped.z, -0.5));
  // Height is left alone: snapping is for laying things out on the floor,
  // not for dropping them through a table.
  KIMIA_REQUIRE(near(snapped.y, 2.0));

  // A step of zero means free placement.
  const Vec3 free = kimia::pick::snapTo(loose, 0.0);
  KIMIA_REQUIRE(near(free.x, 1.34));
  KIMIA_REQUIRE(near(free.z, -0.71));
  // Negative nonsense is treated as off rather than producing garbage.
  KIMIA_REQUIRE(near(kimia::pick::snapTo(loose, -1.0).x, 1.34));
}

KIMIA_TEST(picking_survives_a_broken_viewport) {
  // A stale page can send anything; none of it may crash the engine.
  Viewport empty;
  empty.width = 0;
  empty.height = 0;
  const std::vector<Target> none;
  KIMIA_REQUIRE(!kimia::pick::pickAt(empty, none, 10.0, 10.0).hit);
  Vec3 out;
  KIMIA_REQUIRE(!kimia::pick::pixelOnPlane(empty, 10.0, 10.0, 0.0, out));

  // Pixels outside the picture are answered, not refused: a finger can
  // slide past the edge mid-drag.
  const Viewport viewport = editorCamera();
  Vec3 landed;
  const bool offscreen = kimia::pick::pixelOnPlane(viewport, -50.0, 300.0, 0.0, landed);
  KIMIA_REQUIRE(offscreen);
  KIMIA_REQUIRE(landed.x < 0.0);
}
