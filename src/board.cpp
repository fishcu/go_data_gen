#include "go_data_gen/board.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>

#define FOR_EACH_NEIGHBOR(coord, n_coord, func) \
    (n_coord) = {coord.x - 1, coord.y};         \
    func;                                       \
    (n_coord) = {coord.x + 1, coord.y};         \
    func;                                       \
    (n_coord) = {coord.x, coord.y - 1};         \
    func;                                       \
    (n_coord) = {coord.x, coord.y + 1};         \
    func;

namespace {

// +2 for color to play for situational superko
static constexpr size_t zobrist_hashes_size =
    go_data_gen::Board::max_board_size * go_data_gen::Board::max_board_size * 2 + 2;
uint64_t zobrist_hashes[zobrist_hashes_size];

void init_zobrist() {
    static bool initialized = false;
    if (!initialized) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(0, std::numeric_limits<uint64_t>::max());
        for (size_t i = 0; i < zobrist_hashes_size; ++i) {
            zobrist_hashes[i] = dis(gen);
        }
        initialized = true;
    }
}

uint64_t mem_coord_color_to_zobrist(go_data_gen::Vec2 mem_coord, go_data_gen::Color color) {
    assert(color == go_data_gen::Color::Black || color == go_data_gen::Color::White);
    mem_coord.x -= go_data_gen::Board::padding;
    mem_coord.y -= go_data_gen::Board::padding;
    return zobrist_hashes[(color == go_data_gen::Color::Black ? 0 : 1) +
                          (mem_coord.x + mem_coord.y * go_data_gen::Board::max_board_size) * 2];
}

uint64_t color_to_zobrist(go_data_gen::Color color) {
    return color == go_data_gen::Color::Black ? zobrist_hashes[zobrist_hashes_size - 2]
                                              : zobrist_hashes[zobrist_hashes_size - 1];
}

}  // namespace

namespace go_data_gen {

Board::Board(Vec2 _board_size, float _komi, Ruleset _ruleset, int _num_handicap_stones)
    : board_size{_board_size},
      komi{_komi},
      ruleset{_ruleset},
      num_handicap_stones{_num_handicap_stones} {
    assert(board_size.x <= max_board_size && board_size.y <= max_board_size &&
           "Maximum size exceeded");
    init_zobrist();
    reset();
}

void Board::reset() {
    for (int y = 0; y < data_size; ++y) {
        for (int x = 0; x < data_size; ++x) {
            board[y][x] = static_cast<char>(OffBoard);
            if (y >= padding && x >= padding && y < padding + board_size.y &&
                x < padding + board_size.x) {
                board[y][x] = static_cast<char>(Empty);
            }

            parent[y][x] = {x, y};
            group[y][x].clear();
            liberties[y][x].clear();
        }
    }
    history.clear();
    first_player_to_pass = Empty;
    num_captures = 0;
    num_setup_stones = 0;

    zobrist = 0;
    if (ruleset.ko_rule == KoRule::Simple || ruleset.ko_rule == KoRule::SituationalSuperko) {
        // On an empty board, black gets to play first.
        zobrist_history = std::vector<uint64_t>{zobrist ^ color_to_zobrist(Black)};
    } else {
        zobrist_history = std::vector<uint64_t>{zobrist};
    }
}

void Board::setup_move(Move move) {
    assert(!move.is_pass);
    assert(move.color != OffBoard);

    // Shift coordinate to account for padding of data fields.
    const Vec2 mem_coord{move.coord.x + padding, move.coord.y + padding};

    if ((move.color == Black || move.color == White) && board[mem_coord.y][mem_coord.x] == Empty) {
        ++num_setup_stones;
    } else if (move.color == Empty && (board[mem_coord.y][mem_coord.x] == Black ||
                                       board[mem_coord.y][mem_coord.x] == White)) {
        --num_setup_stones;
    }

    board[mem_coord.y][mem_coord.x] = static_cast<char>(move.color);

    if (move.color == Black || move.color == White) {
        // Initialize new group
        parent[mem_coord.y][mem_coord.x] = mem_coord;
        group[mem_coord.y][mem_coord.x].clear();
        group[mem_coord.y][mem_coord.x].push_back(mem_coord);
        liberties[mem_coord.y][mem_coord.x].clear();
    }

    Vec2 neighbor, root;

    // Update liberties and connect groups.
    // Captures are not handled.
    Color neighbor_color;
    FOR_EACH_NEIGHBOR(
        mem_coord, neighbor,  //
        neighbor_color = static_cast<Color>(board[neighbor.y][neighbor.x]);
        if (move.color == Empty) {
            if (neighbor_color == Black || neighbor_color == White) {
                root = find(neighbor);
                liberties[root.y][root.x].insert(mem_coord);
            }
        } else {
            const auto opp_col = opposite(move.color);
            if (neighbor_color == Empty) {
                root = find(mem_coord);
                liberties[root.y][root.x].insert(neighbor);
            } else if (neighbor_color == move.color) {
                root = find(neighbor);
                liberties[root.y][root.x].erase(mem_coord);
                unite(mem_coord, neighbor);
            } else if (neighbor_color == opp_col) {
                root = find(neighbor);
                liberties[root.y][root.x].erase(mem_coord);
            }
        }  //
    )

    // Assert setup move is not suicidal
    assert(move.color == Empty || liberties[find(mem_coord).y][find(mem_coord).x].size() > 0);
}

MoveLegality Board::get_move_legality(Move move) {
    assert(move.color == Black || move.color == White);
    assert(history.empty() || move.color == opposite(history.back().color));

    if (move.is_pass) {
        return MoveLegality::Legal;
    }

    // Shift coordinate to account for padding of data fields.
    const Vec2 mem_coord{move.coord.x + padding, move.coord.y + padding};

    // Board must be empty
    if (static_cast<Color>(board[mem_coord.y][mem_coord.x]) != Empty) {
        return MoveLegality::NonEmpty;
    }

    auto new_zobrist = zobrist;

    const auto opp_col = opposite(move.color);

    // Simulate playing stone.
    new_zobrist ^= mem_coord_color_to_zobrist(mem_coord, move.color);
    // Figure out liberties and captured groups
    // We need to do it in two passes to avoid simulating a removal of the same group multiple times
    // with even parity, which would cancel out the zobrist hash changes.
    std::vector<Vec2> neighbors_of_same_color;
    std::set<Vec2> resulting_liberties;
    std::set<Vec2> captures;
    bool connects_to_own_group = false;
    Vec2 neighbor, root;
    Color neighbor_color;
    FOR_EACH_NEIGHBOR(
        mem_coord, neighbor,  //
        neighbor_color = static_cast<Color>(board[neighbor.y][neighbor.x]);
        if (neighbor_color == Empty) {
            resulting_liberties.insert(neighbor);
        } else if (neighbor_color == move.color) {
            connects_to_own_group = true;
            root = find(neighbor);
            resulting_liberties.insert(liberties[root.y][root.x].begin(),
                                       liberties[root.y][root.x].end());
            neighbors_of_same_color.push_back(neighbor);
        } else if (neighbor_color == opp_col) {
            root = find(neighbor);
            if (liberties[root.y][root.x].size() == 1) {
                captures.insert(root);
            }
        }  //
    )

    // Check for suicide
    if (captures.empty()) {
        // Account for this move stealing last liberty of neighboring group without adding any
        resulting_liberties.erase(mem_coord);
        // If suicide is disallowed or if move would be single-stone suicide, move is illegal.
        if (resulting_liberties.empty()) {
            if (ruleset.suicide_rule == SuicideRule::Disallowed || !connects_to_own_group) {
                return MoveLegality::Suicidal;
            }
            // If suicidal move is legal, simulate removing group.
            // Undo the move directly, since we don't want to modify the board state.
            new_zobrist ^= mem_coord_color_to_zobrist(mem_coord, move.color);
            for (const auto& neighbor : neighbors_of_same_color) {
                captures.insert(find(neighbor));
            }
        }
    }

    // Calculate zobrist if captured groups are removed
    for (auto capture : captures) {
        for (auto stone : group[capture.y][capture.x]) {
            new_zobrist ^=
                mem_coord_color_to_zobrist(stone, static_cast<Color>(board[stone.y][stone.x]));
        }
    }

    if (ruleset.ko_rule == KoRule::Simple || ruleset.ko_rule == KoRule::SituationalSuperko) {
        // Simulate switching color-to-play.
        new_zobrist ^= color_to_zobrist(opp_col);
    }

    // Check for ko
    if (ruleset.ko_rule == KoRule::Simple) {
        // Simple ko: Move would repeat state two moves ago.
        if (zobrist_history.size() > 1 && new_zobrist == zobrist_history.rbegin()[1]) {
            return MoveLegality::Ko;
        }
    } else {
        // Superko: Move would repeat any previous board state.
        if (std::find(zobrist_history.begin(), zobrist_history.end(), new_zobrist) !=
            zobrist_history.end()) {
            // printf("Duplicate zobrist hash detected. New zobrist: %016lx. Zobrist history:\n",
            //        new_zobrist);
            // const int start = std::max(0, static_cast<int>(zobrist_history.size()) - 7);
            // for (size_t i = start; i < zobrist_history.size(); ++i) {
            //     printf("%016lx\n", zobrist_history[i]);
            // }
            // print();
            return MoveLegality::Ko;
        }
    }

    return MoveLegality::Legal;
}

bool Board::is_legal(Move move) { return get_move_legality(move) == MoveLegality::Legal; }

void Board::play(Move move) {
    assert(get_move_legality(move) == MoveLegality::Legal);

    const auto opp_col = opposite(move.color);
    if (!move.is_pass) {
        // Shift coordinate to account for padding of data fields.
        const Vec2 mem_coord{move.coord.x + padding, move.coord.y + padding};

        // Even though this move may turn out to be suicidal, we update the board and zobrist
        // immediately to reduce branching.
        board[mem_coord.y][mem_coord.x] = static_cast<char>(move.color);
        zobrist ^= mem_coord_color_to_zobrist(mem_coord, move.color);

        // Initialize new group
        parent[mem_coord.y][mem_coord.x] = mem_coord;
        group[mem_coord.y][mem_coord.x].clear();
        group[mem_coord.y][mem_coord.x].push_back(mem_coord);
        liberties[mem_coord.y][mem_coord.x].clear();

        // Add liberties, connect to own groups, and figure out captured groups
        std::set<Vec2> captures;
        Vec2 neighbor, root;
        Color neighbor_color;
        FOR_EACH_NEIGHBOR(
            mem_coord, neighbor,  //
            neighbor_color = static_cast<Color>(board[neighbor.y][neighbor.x]);
            if (neighbor_color == Empty) {
                root = find(mem_coord);
                liberties[root.y][root.x].insert(neighbor);
            } else if (neighbor_color == move.color) {
                root = find(neighbor);
                liberties[root.y][root.x].erase(mem_coord);
                unite(mem_coord, neighbor);
            } else if (neighbor_color == opp_col) {
                root = find(neighbor);
                liberties[root.y][root.x].erase(mem_coord);
                if (liberties[root.y][root.x].size() == 0) {
                    captures.insert(root);
                }
            }  //
        )

        // Handle suicide
        if (captures.empty()) {
            root = find(mem_coord);
            if (liberties[root.y][root.x].empty()) {
                captures.insert(root);
            }
        }

        // Handle captures
        for (auto capture : captures) {
            const auto removed_color = static_cast<Color>(board[capture.y][capture.x]);
            const auto opp_rem_col = opposite(removed_color);
            if (removed_color == Black) {
                num_captures -= group[capture.y][capture.x].size();
            } else {
                num_captures += group[capture.y][capture.x].size();
            }
            for (auto stone : group[capture.y][capture.x]) {
                zobrist ^= mem_coord_color_to_zobrist(stone, removed_color);
                board[stone.y][stone.x] = static_cast<char>(Empty);
                FOR_EACH_NEIGHBOR(
                    stone, neighbor,  //
                    neighbor_color = static_cast<Color>(board[neighbor.y][neighbor.x]);
                    if (neighbor_color == opp_rem_col) {
                        // Capturing a group frees liberties for the opposite color.
                        root = find(neighbor);
                        liberties[root.y][root.x].insert(stone);
                    }  //
                )
#ifndef NDEBUG
                // We don't need to maintain other data structures here.
                // For debugging, clean up anyway.
                parent[stone.y][stone.x].x = stone.x;
                parent[stone.y][stone.x].y = stone.y;
                group[stone.y][stone.x].clear();
                liberties[stone.y][stone.x].clear();
#endif
            }
        }

        if (ruleset.ko_rule == KoRule::Simple || ruleset.ko_rule == KoRule::SituationalSuperko) {
            // printf("Zobrist hash without opp_col: %016lx\n", zobrist);
            zobrist_history.push_back(zobrist ^ color_to_zobrist(opp_col));
        } else {
            zobrist_history.push_back(zobrist);
        }
        // Assert no duplicates
        assert(ruleset.ko_rule == KoRule::Simple ||
               std::set<uint64_t>(zobrist_history.begin(), zobrist_history.end()).size() ==
                   zobrist_history.size());
        // if (ruleset.ko_rule != KoRule::Simple &&
        //     std::set<uint64_t>(zobrist_history.begin(), zobrist_history.end()).size() !=
        //         zobrist_history.size()) {
        //     printf("Duplicate zobrist hash detected. Zobrist history:\n");
        //     const int start = std::max(0, static_cast<int>(zobrist_history.size()) - 5);
        //     for (size_t i = start; i < zobrist_history.size(); ++i) {
        //         printf("%016lx\n", zobrist_history[i]);
        //     }
        //     print();
        //     printf("Ruleset:\n");
        //     printf("ko_rule: %s\n",
        //            ruleset.ko_rule == KoRule::Simple ? "Simple" : "SituationalSuperko");
        //     printf("suicide_rule: %s\n",
        //            ruleset.suicide_rule == SuicideRule::Allowed ? "Allowed" : "Disallowed");
        //     printf("scoring_rule: %s\n",
        //            ruleset.scoring_rule == ScoringRule::Territory ? "Territory" : "Area");
        //     throw std::runtime_error("Duplicate zobrist hash detected");
        // }
    } else {
        // The "button" rule says that the first player to pass gets a bonus of 0.5 points.
        // In addition to that, the "button" is part of the board state, so the zobrist hash is
        // cleared if the pass is the first of the game.
        if (ruleset.first_player_pass_bonus_rule == FirstPlayerPassBonusRule::Bonus &&
            first_player_to_pass == Empty) {
            zobrist_history.clear();
        }
        first_player_to_pass = move.color;
    }

    history.push_back(move);
}

Board::StackedFeaturePlanes Board::get_feature_planes(Color to_play) {
    static constexpr int num_planes_before_lib_planes = 5;
    static constexpr int num_lib_planes = 4;
    static constexpr int num_planes_before_history_planes =
        num_planes_before_lib_planes + 2 * num_lib_planes;
    static constexpr int num_history_planes = 5;
    static_assert(num_feature_planes == num_planes_before_history_planes + num_history_planes);

    const auto opp_col = opposite(to_play);

    // Zero-initialize.
    StackedFeaturePlanes result{};
    for (int y = 0; y < Board::data_size; ++y) {
        for (int x = 0; x < Board::data_size; ++x) {
            const auto move_legality =
                get_move_legality(Move{to_play, false, {x - padding, y - padding}});

            // Legal to play
            result[y][x][0] = static_cast<float>(move_legality == MoveLegality::Legal);
            // Own color
            result[y][x][1] = static_cast<float>(static_cast<Color>(board[y][x]) == to_play);
            // Opponent color
            result[y][x][2] = static_cast<float>(static_cast<Color>(board[y][x]) == opp_col);
            // Is on-board
            result[y][x][3] = static_cast<float>(static_cast<Color>(board[y][x]) != OffBoard);

            // Mark ko / superko
            result[y][x][4] = static_cast<float>(move_legality == MoveLegality::Ko);

            // Liberties of own and opponent groups
            if (static_cast<Color>(board[y][x]) == Black ||
                static_cast<Color>(board[y][x]) == White) {
                const auto root = find(Vec2{x, y});
                const int num_libs = liberties[root.y][root.x].size();
                if (static_cast<Color>(board[y][x]) == to_play) {
                    result[y][x][num_planes_before_lib_planes + std::min(num_libs, num_lib_planes) -
                                 1] = 1.0;
                } else {
                    result[y][x][num_planes_before_lib_planes + num_lib_planes +
                                 std::min(num_libs, num_lib_planes) - 1] = 1.0;
                }
            }

            // TODO: pass-alive areas, ladder status.
        }
    }

    // History of moves. dist = 0 implies the move just played.
    // dist = 1 implies the 2nd-last move played, and so on.
    for (int dist = 0; dist < num_history_planes; ++dist) {
        if (dist < history.size()) {
            const auto& history_move = history.rbegin()[dist];
            if (!history_move.is_pass) {
                result[history_move.coord.y + padding][history_move.coord.x + padding]
                      [num_planes_before_history_planes + dist] = 1.0;
            }
        }
    }

    return result;
}

Board::FeatureVector Board::get_feature_scalars(Color to_play) {
    static constexpr int num_features_before_pass_features = 5;
    static constexpr int num_pass_features = 3;
    static_assert(num_feature_scalars == num_features_before_pass_features + num_pass_features);

    // Zero-initialize.
    FeatureVector result{};

    // Extra points that the current player gets.
    float points_normalization_factor = 1.0f / 15.0f;
    float bonus = komi;  // From White's perspective
    if (ruleset.first_player_pass_bonus_rule == FirstPlayerPassBonusRule::Bonus) {
        if (first_player_to_pass == Black) {
            bonus -= 0.5f;
        } else if (first_player_to_pass == White) {
            bonus += 0.5f;
        }
    }
    result[0] = (to_play == White ? bonus : -bonus) * points_normalization_factor;

    // There is a superko move
    result[1] = static_cast<float>(any_ko_move(to_play));

    // Scoring is area-based or territory-based
    if (ruleset.scoring_rule == ScoringRule::Area) {
        result[2] = 0.0f;
    } else if (ruleset.scoring_rule == ScoringRule::Territory) {
        result[2] = 1.0f;
    }

    // Stone capture difference from current player's perspective, normalized.
    // num_captures is positive if black captured more stones than white.
    result[3] = (to_play == White ? -1.0f : 1.0f) * static_cast<float>(num_captures) *
                points_normalization_factor;

    // Stage of the game as ratio of moves played over board size
    result[4] =
        static_cast<float>(num_setup_stones + history.size()) / (board_size.x * board_size.y);

    // N-last move was pass
    for (int dist = 0; dist < num_pass_features; ++dist) {
        if (dist < history.size()) {
            const auto& history_move = history.rbegin()[dist];
            result[num_features_before_pass_features + dist] =
                static_cast<float>(history_move.is_pass);
        }
    }

    return result;
}

Vec2 Board::find(Vec2 coord) {
    while (parent[coord.y][coord.x] != coord) {
        Vec2& coord_parent = parent[coord.y][coord.x];
        coord_parent = parent[coord_parent.y][coord_parent.x];
        coord = coord_parent;
    }
    return coord;
}

void Board::unite(Vec2 a, Vec2 b) {
    a = find(a);
    b = find(b);
    if (a == b) {
        return;
    }

    if (group[a.y][a.x].size() < group[b.y][b.x].size()) {
        const Vec2 tmp = a;
        a = b;
        b = tmp;
    }

    parent[b.y][b.x] = a;
    group[a.y][a.x].insert(std::end(group[a.y][a.x]), std::begin(group[b.y][b.x]),
                           std::end(group[b.y][b.x]));
    liberties[a.y][a.x].insert(liberties[b.y][b.x].begin(), liberties[b.y][b.x].end());
}

bool Board::any_ko_move(Color to_play) {
    for (int y = 0; y < board_size.y; ++y) {
        for (int x = 0; x < board_size.x; ++x) {
            const auto move_legality = get_move_legality(Move{to_play, false, {x, y}});
            if (move_legality == MoveLegality::Ko) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace go_data_gen
