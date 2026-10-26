#pragma once

#include <string>

namespace road {
inline std::string startposQuery(std::string name) {
    // Match GD's 20-character level names/search field. The video searches
    // "Forest Temple startp": the full suffix would miss those truncated names.
    name += " startpos";
    if (name.size() > 20) name.resize(20);
    while (!name.empty() && name.back() == ' ') name.pop_back();
    return name;
}
}
