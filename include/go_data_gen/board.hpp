#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

#include "rules.hpp"
#include "types.hpp"

namespace go_data_gen {

class Board {
public:
    static constexpr int max_board_size = 19;
    static constexpr int padding = 1;
    static constexpr int data_size = max_board_size + 2 * padding;

    Board(Vec2 board_size = {19, 19}, float komi = 7.5, Ruleset ruleset = TrompTaylorRules,
          int num_handicap_stones = 0);

    ~Board() = default;

    float komi;
    int num_handicap_stones;

    void reset();
    // Used for handicap and setup moves.
    // Does not handle legality checks or captures.
    // Does not influence (super)ko.
    // Does not get recorded history.
    // Allows for "Empty" moves, meaning it erases stones from the board.
    // The number of setup stones is tracked in `num_setup_stones`.
    void setup_move(Move move);

    Ruleset ruleset;
    MoveLegality get_move_legality(Move move);
    bool is_legal(Move move);

    void play(Move move);

    static constexpr int num_feature_planes = 18;
    using StackedFeaturePlanes =
        std::array<std::array<std::array<float, num_feature_planes>, data_size>, data_size>;
    StackedFeaturePlanes get_feature_planes(Color to_play);

    static constexpr int num_feature_scalars = 8;
    using FeatureVector = std::array<float, num_feature_scalars>;
    FeatureVector get_feature_scalars(Color to_play);

    void print(std::function<bool(int x, int y)> highlight_fn = [](int, int) { return false; });
    void print_group_sizes();
    void print_liberties();

private:
    char board[data_size][data_size];
    Vec2 board_size;

    Vec2 parent[data_size][data_size];
    std::vector<Vec2> group[data_size][data_size];
    std::set<Vec2> liberties[data_size][data_size];

    Vec2 find(Vec2 coord);
    void unite(Vec2 a, Vec2 b);

    std::vector<Move> history;
    Color first_player_to_pass;
    int num_captures;  // Number of captures by Black minus number of captures by White
    int num_setup_stones;

    uint64_t zobrist;
    std::vector<uint64_t> zobrist_history;
    bool any_ko_move(Color to_play);
};

}  // namespace go_data_gen
