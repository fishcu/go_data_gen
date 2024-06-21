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
    bool is_legal(Move move);
    void play(Move move);

    enum PrintMode {
        Default = 0,
        GroupSize,
        Liberties,
        IllegalMovesBlack,
        IllegalMovesWhite,
    };
    void print(PrintMode mode = Default);

private:
    char board[max_size + 2][max_size + 2];
    Vec2 size;

    float komi;

    Vec2 parent[max_size + 2][max_size + 2];
    std::vector<Vec2> group[max_size + 2][max_size + 2];
    std::set<Vec2> liberties[max_size + 2][max_size + 2];

    Vec2 find(Vec2 coord);
    void unite(Vec2 a, Vec2 b);

    uint64_t zobrist;
    std::set<uint64_t> zobrist_history;
};

}  // namespace go_data_gen
