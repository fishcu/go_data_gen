#include "go_data_gen/sgf.hpp"

#include <cassert>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "go_data_gen/board.hpp"
#include "go_data_gen/types.hpp"

namespace go_data_gen {

void load_sgf(const std::string& file_path, Board& board, std::vector<Move>& moves, float& result) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open the file: " + file_path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    // Extract size
    const std::regex size_regex(R"(SZ\[(\d+)(?::(\d+))?\])");
    std::smatch size_match;
    const bool size_found = std::regex_search(content, size_match, size_regex);
    assert(size_found && "Size not found in the SGF file");
    const int size_x = std::stoi(size_match[1]);
    const int size_y =
        size_match.size() > 2 && size_match[2].matched ? std::stoi(size_match[2]) : size_x;
    assert(size_x <= Board::max_size && size_y <= Board::max_size && "Maximum size exceeded");

    // Extract komi
    const std::regex komi_regex(R"(KM\[(-?\d+(?:\.\d+)?)\])");
    std::smatch komi_match;
    const bool komi_found = std::regex_search(content, komi_match, komi_regex);
    assert(komi_found && "Komi not found in the SGF file");
    const double komi = std::stod(komi_match[1]);

    board = Board(Vec2{size_x, size_y}, komi);

    // Extract moves and handicap stones
    // Doesn't handle branches!!
    const std::regex move_regex(R"(([BW])\[([a-z]{2})?\]|A([BW])(\[[a-z]{2}\])+)");
    const std::regex coord_regex(R"(\[([a-z]{2})\])");
    std::sregex_iterator move_iter(content.begin(), content.end(), move_regex);
    const std::sregex_iterator end;
    while (move_iter != end) {
        const std::smatch move_match = *move_iter;

        if (move_match[1].matched) {
            // Regular move
            const Color color = move_match[1].str()[0] == 'B' ? Black : White;
            if (move_match[2].matched) {
                const char col = move_match[2].str()[0] - 'a';
                const char row = move_match[2].str()[1] - 'a';
                moves.push_back({color, {static_cast<int>(col), static_cast<int>(row)}});
            } else {
                moves.push_back(pass);
            }
        } else if (move_match[3].matched) {
            // Handicap stones (AB or AW)
            const Color color = move_match[3].str()[0] == 'B' ? Black : White;
            const std::string coords_str = move_match[0].str();

            std::sregex_iterator coord_iter(coords_str.begin(), coords_str.end(), coord_regex);

            while (coord_iter != end) {
                const std::smatch coord_match = *coord_iter;
                const char col = coord_match[1].str()[0] - 'a';
                const char row = coord_match[1].str()[1] - 'a';
                moves.push_back({color, {static_cast<int>(col), static_cast<int>(row)}});
                ++coord_iter;
            }
        }

        ++move_iter;
    }

    // Extract result
    const std::regex result_regex(R"(RE\[((?:B|W)\+(?:\d+(?:\.\d+)?|R)?|0|Void)\])");
    std::smatch result_match;
    const bool result_found = std::regex_search(content, result_match, result_regex);
    assert(result_found && "Result not found in the SGF file");

    const std::string result_str = result_match[1].str();
    if (result_str == "B+R") {
        result = -1.0f;  // Black wins by resignation
    } else if (result_str == "W+R") {
        result = 1.0f;  // White wins by resignation
    } else if (result_str == "0" || result_str == "Void") {
        result = 0.0f;  // Draw or void game
    } else {
        // Parse score for B+<score> or W+<score>
        const char winner = result_str[0];
        const float score = std::stof(result_str.substr(2));
        result = (winner == 'W' ? 1.0f : -1.0f) * score;
    }
}

}  // namespace go_data_gen
