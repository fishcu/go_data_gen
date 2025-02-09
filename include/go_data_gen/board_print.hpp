#pragma once

#include <pybind11/numpy.h>

#include <optional>
#include <string>

#include "go_data_gen/board.hpp"

namespace py = pybind11;

namespace go_data_gen {

class BoardPrinter {
public:
    static void print(Board& board, std::optional<int> highlight_feature);

    // Debug prints
    static void print_group_sizes(const Board& board);
    static void print_liberties(const Board& board);

private:
    static bool is_on_board(const Board& board, int row, int col);
};

}  // namespace go_data_gen
