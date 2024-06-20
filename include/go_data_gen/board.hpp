#pragma once

#include <set>
#include <string>
#include <vector>

#include "types.hpp"

namespace go_data_gen {

class Board {
public:
    static constexpr int max_size = 19;

    Board() = default;
    Board(Vec2 size, float komi);

    ~Board() = default;

    void reset();
    void play(Move move);

private:
    char board[max_size + 2][max_size + 2];
    Vec2 size;

    float komi;

    Vec2 find(Vec2 coord);
    void union(Vec2 a, Vec2 b);
    Vec2 parent[max_size + 2][max_size + 2];
    int size[max_size + 2][max_size + 2];

    std::set<Vec2> liberties[max_size + 2][max_size + 2];
};

}  // namespace go_data_gen