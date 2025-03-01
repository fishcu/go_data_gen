#pragma once

#include <string>
#include <vector>

#include "go_data_gen/board.hpp"

namespace go_data_gen {

// Return false if the file is in the encore phase, true otherwise
bool load_sgf(const std::string& file_path, Board& board, std::vector<Move>& moves, float& result);

}  // namespace go_data_gen
