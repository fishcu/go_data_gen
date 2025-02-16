#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

#include "types.hpp"

namespace go_data_gen {

class Board {
public:
    static constexpr int max_board_size = 19;
    static constexpr int padding = 1;
    static constexpr int data_size = max_board_size + 2 * padding;

    Board(Vec2 board_size = {19, 19}, float komi = 7.5);

    ~Board() = default;

    float komi;

    void reset();
    // Used for handicap and setup moves.
    // Does not handle legality checks or captures.
    // Does not influence superko.
    // Does not get recorded history.
    // Allows for "Empty" moves, meaning it erases stones from the board.
    void setup_move(Move move);

    MoveLegality get_move_legality(Move move);

    void play(Move move);

    static constexpr int num_feature_planes = 18;
    using StackedFeaturePlanes =
        std::array<std::array<std::array<float, num_feature_planes>, data_size>, data_size>;
    StackedFeaturePlanes get_feature_planes(Color to_play);

    static constexpr int num_feature_scalars = 7;
    using FeatureVector = std::array<float, num_feature_scalars>;
    FeatureVector get_feature_scalars(Color to_play);

    void print(std::function<bool(int x, int y)> highlight_fn = [](int, int) { return false; });
    void print_group_sizes();
    void print_liberties();

private:
    char board[data_size][data_size];
    Vec2 board_size;

    std::vector<Move> history;

    Vec2 parent[data_size][data_size];
    std::vector<Vec2> group[data_size][data_size];
    std::set<Vec2> liberties[data_size][data_size];

    Vec2 find(Vec2 coord);
    void unite(Vec2 a, Vec2 b);

    uint64_t zobrist;
    std::set<uint64_t> zobrist_history;

    bool any_superko_move(Color to_play);
};

}  // namespace go_data_gen
