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

struct Move {
    Color color;
    bool is_pass;
    Vec2 coord;  // Only valid if is_pass is false.

    Move() = delete;
    explicit Move(Color c, bool pass, Vec2 pos) : color(c), is_pass(pass), coord(pos) {}

    bool operator==(const Move& other) const {
        // Both moves need to be a pass of the same color (coordinates don't matter),
        // or both need to be not a pass and have the same coordinates.
        return color == other.color &&
               (is_pass ? other.is_pass : (!other.is_pass && coord == other.coord));
    }
    bool operator!=(const Move& other) const { return !(operator==(other)); }
};

enum MoveLegality {
    Legal = 0,
    NonEmpty = 1,
    Suicidal = 2,
    Superko = 3,
};

}  // namespace go_data_gen
