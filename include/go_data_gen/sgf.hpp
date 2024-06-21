#pragma once

#include <string>
#include <vector>

#include "board.hpp"

namespace go_data_gen {

void load_sgf(const std::string& file_path, Board& board, std::vector<Move>& moves, float& result);

}  // namespace go_data_gen
