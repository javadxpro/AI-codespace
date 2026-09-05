#pragma once

// KIMIA engine version — the single source of truth in code.
//
// The version IS the Jalali (Solar Hijri) release date, YEAR.MONTH.DAY
// (e.g. 1405.06.14). A second release on the same day appends ".2".
// CMakeLists.txt's project(VERSION) carries the same value and the test suite
// pins the two together; every bump gets an entry in CHANGELOG.md (engine
// version, development time, Jalali + Gregorian dates).

#include <kimia/Types.h>

// The literal is spelled once; the human-readable string is built from it by
// literal concatenation so the two can never drift apart.
#define KIMIA_ENGINE_VERSION "1405.06.14.2"
#define KIMIA_ENGINE_VERSION_GREGORIAN "2026-09-05"

namespace kimia {

inline constexpr const char* kEngineName = "KIMIA";
inline constexpr const char* kEngineVersion = KIMIA_ENGINE_VERSION;
inline constexpr const char* kEngineVersionGregorian = KIMIA_ENGINE_VERSION_GREGORIAN;
inline constexpr u32 kEngineVersionYear = 1405U;
inline constexpr u32 kEngineVersionMonth = 6U;
inline constexpr u32 kEngineVersionDay = 14U;

// "KIMIA 1405.06.14 (2026-09-05)" — for banners and --version.
inline constexpr const char* kEngineVersionString =
    "KIMIA " KIMIA_ENGINE_VERSION " (" KIMIA_ENGINE_VERSION_GREGORIAN ")";

}  // namespace kimia
