#include "../src/ui/Layout.hpp"
#include <array>
#include <cassert>

int main() {
    using namespace roadui::layout;
    const std::array zones = {header, tabs, toolbar, content, footer};
    for (std::size_t i = 0; i < zones.size(); ++i) {
        assert(zones[i].inside(width, height));
        for (std::size_t j = i + 1; j < zones.size(); ++j)
            assert(!zones[i].overlaps(zones[j]));
    }
    for (int i = 0; i < 3; ++i) {
        const auto button = tab(i);
        assert(button.inside(width, height));
        assert(!button.overlaps(content));
        assert(!button.overlaps(toolbar));
        if (i < 2) assert(!button.overlaps(tab(i + 1)));
    }
    const float screens[][2] = {{568, 320}, {480, 270}, {800, 600}, {1024, 576}};
    for (auto const& screen : screens) {
        const float scale = scaleFor(screen[0], screen[1]);
        assert(scale > .8f && scale <= 1.f);
        assert(width * scale <= screen[0] - 22.f + .01f);
        assert(height * scale <= screen[1] - 16.f + .01f);
    }
    // The RDoD information button is entirely outside the native cell.
    assert(nativeCellWidth + detailGutter == contentWidth);
    assert(rowHeight <= contentHeight);
}
