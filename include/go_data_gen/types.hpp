#pragma once

#include <cassert>
#include <utility>

namespace go_data_gen {

enum Color {
    Empty = 0,
    Black = 1,
    White = 2,
    OffBoard = 3,
};

inline Color opposite(Color c) {
    assert(c == Black || c == White);
    return c == Black ? White : Black;
}

struct Vec2 {
    int x, y;
    bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Vec2& other) const { return !(operator==(other)); }
    bool operator<(const Vec2& other) const {
        if (x < other.x)
            return true;
        if (x > other.x)
            return false;
        return y < other.y;
    }
};

constexpr Vec2 pass{-1, -1};

struct Move {
    Color color;
    Vec2 coord;
    bool operator==(const Move& other) const {
        return color == other.color && coord == other.coord;
    }
    bool operator!=(const Move& other) const { return !(operator==(other)); }
};

}  // namespace go_data_gen
