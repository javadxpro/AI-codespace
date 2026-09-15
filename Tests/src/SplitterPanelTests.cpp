#include <kimia_test.h>
#include <kimia/SplitterPanel.h>

KIMIA_TEST(Splitter_DrawHorizontalDoesNotCrash) {
  kimia::ui::drawSplitterPanel({100, 100, 4, 200},
                               kimia::ui::SplitterAxis::Horizontal);
}

KIMIA_TEST(Splitter_DrawVerticalDoesNotCrash) {
  kimia::ui::drawSplitterPanel({100, 100, 200, 4},
                               kimia::ui::SplitterAxis::Vertical);
}

KIMIA_TEST(Splitter_DrawTinyDoesNotCrash) {
  kimia::ui::drawSplitterPanel({0, 0, 1, 1},
                               kimia::ui::SplitterAxis::Horizontal);
  kimia::ui::drawSplitterPanel({0, 0, 1, 1},
                               kimia::ui::SplitterAxis::Vertical);
}

KIMIA_TEST(Splitter_DrawLargeDoesNotCrash) {
  kimia::ui::drawSplitterPanel({0, 0, 4, 800},
                               kimia::ui::SplitterAxis::Horizontal);
  kimia::ui::drawSplitterPanel({0, 0, 800, 4},
                               kimia::ui::SplitterAxis::Vertical);
}
