#include "go_data_gen/sgf.hpp"

#include <cassert>
#include <fstream>
#include <regex>
#include <sstream>

#include "go_data_gen/board.hpp"
#include "go_data_gen/types.hpp"

namespace go_data_gen {

bool load_sgf(const std::string& file_path, Board& board, std::vector<Move>& moves, float& result) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open the file: " + file_path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    const std::regex encore_regex(R"(beganInEncorePhase=(\d+))");
    std::smatch encore_match;
    if (std::regex_search(content, encore_match, encore_regex)) {
        return false;
    }

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

    // Extract number of handicap stones
    const std::regex handicap_regex(R"(HA\[(\d+)\])");
    std::smatch handicap_match;
    const bool handicap_found = std::regex_search(content, handicap_match, handicap_regex);
    const int num_handicap_stones = handicap_found ? std::stoi(handicap_match[1]) : 0;

    // Extract komi
    const std::regex komi_regex(R"(KM\[(-?\d+(?:\.\d+)?)\])");
    std::smatch komi_match;
    const bool komi_found = std::regex_search(content, komi_match, komi_regex);
    assert(komi_found && "Komi not found in the SGF file");
    const double komi = std::stod(komi_match[1]);

    // Extract ruleset
    const std::regex ruleset_regex(R"(RU\[([^\]]+)\])");
    std::smatch ruleset_match;
    const bool ruleset_found = std::regex_search(content, ruleset_match, ruleset_regex);
    assert(ruleset_found && "Ruleset not found in the SGF file");

    const std::string rules_str = ruleset_match[1];
    Ruleset ruleset;

    // Parse ko rule
    if (rules_str.find("koPOSITIONAL") != std::string::npos) {
        ruleset.ko_rule = KoRule::PositionalSuperko;
    } else if (rules_str.find("koSITUATIONAL") != std::string::npos) {
        ruleset.ko_rule = KoRule::SituationalSuperko;
    } else {
        ruleset.ko_rule = KoRule::Simple;
    }

    // Parse suicide rule
    if (rules_str.find("sui1") != std::string::npos) {
        ruleset.suicide_rule = SuicideRule::Allowed;
    } else {
        ruleset.suicide_rule = SuicideRule::Disallowed;
    }

    // Parse scoring rule
    if (rules_str.find("scoreAREA") != std::string::npos) {
        ruleset.scoring_rule = ScoringRule::Area;
    } else {
        ruleset.scoring_rule = ScoringRule::Territory;
    }

    // Parse tax rule
    if (rules_str.find("taxALL") != std::string::npos) {
        ruleset.tax_rule = TaxRule::All;
    } else if (rules_str.find("taxSEKI") != std::string::npos) {
        ruleset.tax_rule = TaxRule::Seki;
    } else {
        ruleset.tax_rule = TaxRule::NoTax;
    }

    // Parse button (first player pass bonus) rule
    if (rules_str.find("button1") != std::string::npos) {
        ruleset.first_player_pass_bonus_rule = FirstPlayerPassBonusRule::Bonus;
    } else {
        ruleset.first_player_pass_bonus_rule = FirstPlayerPassBonusRule::NoBonus;
    }

    const std::sregex_iterator end;

    // Handle setup and handicap moves
    std::vector<Move> setup_moves;
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
                setup_moves.push_back(Move{Black, false, {x, y}});
            } else if (setup_type == 'W') {
                setup_moves.push_back(Move{White, false, {x, y}});
            } else if (setup_type == 'E') {
                setup_moves.push_back(Move{Empty, false, {x, y}});
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

    // Set up board and verify that all moves are legal
    board = Board(Vec2{size_x, size_y}, komi, ruleset, num_handicap_stones);
    for (const Move& move : setup_moves) {
        board.setup_move(move);
    }

    for (const Move& move : moves) {
        const auto legality = board.get_move_legality(move);
        if (legality != MoveLegality::Legal) {
            printf("Illegal move detected: %s (%d, %d)", move.color == Black ? "Black" : "White",
                   move.coord.x, move.coord.y);
            printf("Move legality: ");
            switch (legality) {
            case MoveLegality::NonEmpty:
                printf("Non-empty\n");
                break;
            case MoveLegality::Suicidal:
                printf("Suicidal\n");
                break;
            case MoveLegality::Ko:
                printf("Ko\n");
                break;
            default:
                assert(false && "Invalid move legality");
            }
            board.print([&move](int mem_x, int mem_y) {
                return !move.is_pass && mem_x == move.coord.x + Board::padding &&
                       mem_y == move.coord.y + Board::padding;
            });
            throw std::runtime_error("Illegal move");
        }
        board.play(move);
    }

    board.reset();

    for (const Move& move : setup_moves) {
        board.setup_move(move);
    }

    // Extract start turn index
    const std::regex start_turn_regex(R"(startTurnIdx=(\d+))");
    std::smatch start_turn_match;
    const bool start_turn_found = std::regex_search(content, start_turn_match, start_turn_regex);
    assert(start_turn_found && "Start turn not found in the SGF file");
    const int start_turn_index = std::stoi(start_turn_match[1]);

    // Treat the first startTurnIdx moves as setup moves that should not be included in the training
    // data.
    for (int i = 0; i < start_turn_index; i++) {
        board.play(moves[i]);
    }
    moves.erase(moves.begin(), moves.begin() + start_turn_index);

    return true;
}

}  // namespace go_data_gen
