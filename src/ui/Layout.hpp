#pragma once

namespace roadui::layout {
// Coordinates use GD's logical points. The whole window scales together on
// smaller screens, so labels, hit targets and the scroll viewport stay aligned.
inline constexpr float width = 460.f;
inline constexpr float height = 302.f;
inline constexpr float padding = 14.f;
inline constexpr float contentWidth = width - 2.f * padding;
inline constexpr float contentBottom = 38.f;
inline constexpr float contentTop = 214.f;
inline constexpr float contentHeight = contentTop - contentBottom;
inline constexpr float rowHeight = 90.f;
inline constexpr float detailGutter = 30.f;
inline constexpr float nativeCellWidth = contentWidth - detailGutter;

struct Rect {
    float x, y, width, height;
    constexpr bool overlaps(Rect other) const {
        return x < other.x + other.width && x + width > other.x &&
               y < other.y + other.height && y + height > other.y;
    }
    constexpr bool inside(float w, float h) const {
        return x >= 0 && y >= 0 && x + width <= w && y + height <= h;
    }
};

inline constexpr Rect header{padding, 274.f, contentWidth, 26.f};
inline constexpr Rect tabs{padding, 246.f, contentWidth, 24.f};
inline constexpr Rect toolbar{padding, 218.f, contentWidth, 24.f};
inline constexpr Rect content{padding, contentBottom, contentWidth, contentHeight};
inline constexpr Rect footer{padding, 8.f, contentWidth, 24.f};

constexpr Rect tab(int index) {
    return {padding + index * 146.f, tabs.y, 140.f, tabs.height};
}

constexpr float scaleFor(float screenWidth, float screenHeight) {
    const float sx = (screenWidth - 22.f) / width;
    const float sy = (screenHeight - 16.f) / height;
    return sx < sy ? (sx < 1.f ? sx : 1.f) : (sy < 1.f ? sy : 1.f);
}
}
