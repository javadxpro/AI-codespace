#pragma once

#include <kimia/Types.h>

namespace kimia {

// Entity handles are 1-based; 0 is the null handle (never allocated).
using EntityHandle = u32;
inline constexpr EntityHandle kNullEntity = 0U;

}  // namespace kimia
