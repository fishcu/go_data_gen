#pragma once

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

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

    Board() = default;
    Board(Vec2 board_size, float komi);

    ~Board() = default;

    float komi;

    void reset();
    // Used for handicap and setup moves.
    // Does not handle legality checks or captures.
    // Does not influence superko.
    // Does not get recorded history.
    // Allows for "Empty" moves, meaning it erases stones from the board.
    void setup_move(Move move);

    bool is_legal(Move move);
    void play(Move move);

    pybind11::array_t<float> get_mask();
    pybind11::array_t<float> get_legal_map(Color color);
    pybind11::array_t<float> get_stone_map(Color color);
    pybind11::array_t<float> get_history_map(int dist);
    pybind11::array_t<float> get_liberty_map(Color color, int num, bool or_greater = false);

    float get_komi_from_player_perspective(Color to_play);

    // Gets all relevant NN input data in one function, calling all of the above.
    // Returns a tuple of tensors:
    // The first tensor contains the stacked feature maps.
    // The second tensor is a vector of scalar features.
    static constexpr int num_feature_planes = 17;
    static constexpr int num_feature_scalars = 1;
    pybind11::tuple get_nn_input_data(Color to_play);

    enum PrintMode {
        Default = 0,
        GroupSize,
        Liberties,
        IllegalMovesBlack,
        IllegalMovesWhite,
    };
    void print(PrintMode mode = Default);

    void print_feature_planes(Color to_play, int feature_plane_index = 0);

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

    // Common function for all get_*_map functions
    pybind11::array_t<float> get_map(Color color, std::function<bool(int, int)> condition);
};

}  // namespace go_data_gen
