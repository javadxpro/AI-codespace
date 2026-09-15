// SearchPanel — the type-to-find panel layered on top of the
// Object Tree. Filters entity names by case-insensitive substring
// match. Phase 4+ lands the matching logic; Phase 5+ wires the
// click-to-select handler.
//
// The "results" callback returns the indices of matching entities
// from the original list — the caller renders those rows in the
// Object Tree below.
#pragma once

#include "EditorUI.h"
#include <string>
#include <vector>
#include <cctype>

namespace kimia::ui {

// Case-insensitive substring match. Empty needle matches everything.
bool searchMatch(const std::string& needle, const std::string& hay);

// Filter the given names by the search query. Returns the indices
// (into `names`) of entries that match. If `query` is empty, every
// index is returned in order.
std::vector<usize> searchFilter(const std::string& query,
                                const std::vector<std::string>& names);

// Render the search box. The query is drawn as one line of text.
// Phase 4+ draws the box + a placeholder hint; Phase 5+ will wire
// the click-to-focus and the live filtering.
void drawSearchPanel(const Rect& rect, const std::string& query,
                     const std::string& placeholder);

}  // namespace kimia::ui
