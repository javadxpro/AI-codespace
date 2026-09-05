#pragma once

// KIMIA engine version — the single source of truth in code.
//
// Semantic numbering, MAJOR.MINOR.PATCH, moved by the SIZE of a change and
// never by the calendar (the user's rule):
//   - a small change or a bug fix  -> PATCH + 1            (0.1.0 -> 0.1.1)
//   - a stage or a big change      -> MINOR + 1, PATCH = 0 (0.1.1 -> 0.2.0)
//   - MAJOR                        -> only when the user declares it (a game release)
// CMakeLists.txt's project(VERSION) carries the same value; the configure step
// and a unit test keep the two identical, and the test also requires the
// NEWEST entry of CHANGELOG.md to be this version (version, what changed,
// development time, the day's date). Versions before 0.1.0 were Jalali
// release dates (1405.06.05 … 1405.06.14.3); those entries stay as written.

#include <kimia/Types.h>

// The literal is spelled once; the banner string is built from it by literal
// concatenation and the numeric fields are pinned to it by a test.
#define KIMIA_ENGINE_VERSION "0.1.0"

namespace kimia {

inline constexpr const char* kEngineName = "KIMIA";
inline constexpr const char* kEngineVersion = KIMIA_ENGINE_VERSION;
inline constexpr u32 kEngineVersionMajor = 0U;
inline constexpr u32 kEngineVersionMinor = 1U;
inline constexpr u32 kEngineVersionPatch = 0U;

// "KIMIA 0.1.0" — for banners and --version.
inline constexpr const char* kEngineVersionString = "KIMIA " KIMIA_ENGINE_VERSION;

}  // namespace kimia
