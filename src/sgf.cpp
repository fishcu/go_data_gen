#include "go_data_gen/sgf.hpp"

#include <cassert>
#include <fstream>
#include <regex>
#include <sstream>

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
    assert(size_x <= Board::max_board_size && size_y <= Board::max_board_size &&
           "Maximum size exceeded");

    // Extract komi
    const std::regex komi_regex(R"(KM\[(-?\d+(?:\.\d+)?)\])");
    std::smatch komi_match;
    const bool komi_found = std::regex_search(content, komi_match, komi_regex);
    assert(komi_found && "Komi not found in the SGF file");
    const double komi = std::stod(komi_match[1]);

    board = Board(Vec2{size_x, size_y}, komi);

    const std::sregex_iterator end;

    // Handle setup and handicap moves
    const std::regex setup_regex(R"(A([BWE])(\[[a-z]{2}\])+)");
    std::sregex_iterator setup_iter(content.begin(), content.end(), setup_regex);
    const std::regex coord_regex(R"(\[([a-z]{2})\])");
    while (setup_iter != end) {
        const std::smatch setup_match = *setup_iter;
        const char setup_type = setup_match[1].str()[0];
        const std::string coords_str = setup_match[0].str();

        std::sregex_iterator coord_iter(coords_str.begin(), coords_str.end(), coord_regex);

        while (coord_iter != end) {
            const std::smatch coord_match = *coord_iter;
            const int x = static_cast<int>(coord_match[1].str()[0] - 'a');
            const int y = static_cast<int>(coord_match[1].str()[1] - 'a');

            if (setup_type == 'B') {
                board.setup_move(Move{Black, false, {x, y}});
            } else if (setup_type == 'W') {
                board.setup_move(Move{White, false, {x, y}});
            } else if (setup_type == 'E') {
                board.setup_move(Move{Empty, false, {x, y}});
            }

            ++coord_iter;
        }

        ++setup_iter;
    }

    // Extract moves
    // Doesn't handle branches!!
    // Stop extraction after two consecutive passes
    const std::regex move_regex(R"(([BW])\[([a-z]{2})?\])");
    std::sregex_iterator move_iter(content.begin(), content.end(), move_regex);
    int consecutive_passes = 0;
    while (move_iter != end && consecutive_passes < 2) {
        const std::smatch move_match = *move_iter;

        // Explictly need to skip false AB and AW matches since C++ regex doesn't support negative
        // lookbehind assertions
        std::string move_context;
        if (move_match.position() >= 1) {
            move_context = content.substr(move_match.position() - 1, 2);
        } else {
            move_context = "";  // No valid context if we're at the start of the string
        }
        if (move_context == "AB" || move_context == "AW" || move_context == "AE") {
            ++move_iter;
            continue;
        }

        const Color color = move_match[1].str()[0] == 'B' ? Black : White;

        if (move_match[2].matched) {
            const int x = static_cast<int>(move_match[2].str()[0] - 'a');
            const int y = static_cast<int>(move_match[2].str()[1] - 'a');
            moves.push_back(Move{color, false, {x, y}});
            consecutive_passes = 0;
        } else {
            moves.push_back(Move{color, true, {}});
            ++consecutive_passes;
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
        result = 1000.0f;  // Black wins by resignation
    } else if (result_str == "W+R") {
        result = -1000.0f;  // White wins by resignation
    } else if (result_str == "0" || result_str == "Void") {
        result = 0.0f;  // Draw or void game
    } else {
        // Parse score for B+<score> or W+<score>
        const char winner = result_str[0];
        const float score = std::stof(result_str.substr(2));
        result = (winner == 'W' ? -1.0f : 1.0f) * score;
    }
}

}  // namespace go_data_gen
