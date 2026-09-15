#pragma once
#include "EditorUI.h"

namespace kimia::ui {

void drawAboutPanel(const Rect& rect,
                    const std::string& engineName,
                    const std::string& version,
                    const std::string& buildDate,
                    const std::string& platform,
                    const std::string& gpu);

}
