#pragma once
#include "EditorUI.h"
#include <vector>

namespace kimia::ui {

void drawWaveformPanel(const Rect& rect,
                       const std::vector<f32>& samples,
                       f32 sampleRate,
                       f32 playheadSeconds,
                       bool playing);

}
