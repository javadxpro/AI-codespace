// Compiles the vendored stb_vorbis decoder as plain C (upstream code; strict
// C++ warnings do not apply). The one-shot decode API is bridged from
// Engine/Assets/src/Audio.cpp via an extern "C" declaration.
#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"
