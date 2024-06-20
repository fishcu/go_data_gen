#include <cassert>

namespace go_data_gen {

enum Color {
    Empty = 0,
    Black = 1,
    White = 2,
    OffBoard = 3,
};

inline opposite(Color c) {
    assert(c == Black || c == White);
    return c == Black ? White : Black;
}

struct Vec2 {
    int x, y;
    bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
};

struct Move {
    Color color;
    Vec2 coord;
};

constexpr Move pass{OffBoard, {-1, -1}};

}  // namespace go_data_gen
