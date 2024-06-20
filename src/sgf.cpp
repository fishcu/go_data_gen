#include "go_data_gen/sgf.hpp"

#include <cassert>
#include <fstream>
#include <regex>
#include <sstream>

#include "go_data_gen/types.hpp"
#include "go_data_gen/board.hpp"

namespace go_data_gen {

void load_sgf(const std::string& file_path, Board& board, std::vector<Move>& moves) {
    std::ifstream file(file_path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    // Extract size
    const std::regex size_regex(R"(SZ\[(\d+)(?::(\d+))?\])");
    std::smatch size_match;
    const bool size_found = std::regex_search(content, size_match, size_regex);
    assert(size_found && "Size not found in the SGF file");
    const int size_x = std::stoi(size_match[1]);
    const int size_y = size_match.size() > 2 && size_match[2].matched ? std::stoi(size_match[2]) : size_x;
    assert(size_x <= Board::max_size && size_y <= Board::max_size && "Maximum size exceeded");

    // Extract komi
    const std::regex komi_regex(R"(KM\[(-?\d+(?:\.\d+)?)\])");
    std::smatch komi_match;
    const bool komi_found = std::regex_search(content, komi_match, komi_regex);
    assert(komi_found && "Komi not found in the SGF file");
    const double komi = std::stod(komi_match[1]);

    board = Board(Vec2{size_x, size_y}, komi);

    // Extract moves
    const std::regex move_regex(R"([BW]\[([a-z]{2})?\])");
    std::sregex_iterator move_iter(content.begin(), content.end(), move_regex);
    const std::sregex_iterator move_end;
    while (move_iter != move_end) {
        const std::smatch move_match = *move_iter;
        const Color color = move_match[0].str()[0] == 'B' ? Black : White;
        if (move_match[1].matched) {
            const char col = move_match[1].str()[0] - 'a';
            const char row = move_match[1].str()[1] - 'a';
            moves.push_back({color, {static_cast<int>(col), static_cast<int>(row)}});
        } else {
            moves.push_back(pass);
        }
        ++move_iter;
    }
}

}  // namespace go_data_gen