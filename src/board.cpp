#include "go_data_gen/board.hpp"

#include <iostream>
#include <random>

#include "go_data_gen/python_env.hpp"
#include "go_data_gen/types.hpp"

namespace py = pybind11;

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

uint64_t
    zobrist_hashes[go_data_gen::Board::max_board_size * go_data_gen::Board::max_board_size * 3];

void init_zobrist() {
    static bool initialized = false;
    if (!initialized) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(0, std::numeric_limits<uint64_t>::max());
        for (size_t i = 0; i < sizeof(zobrist_hashes) / sizeof(uint64_t); ++i) {
            zobrist_hashes[i] = dis(gen);
        }
        initialized = true;
    }
}

uint64_t mem_coord_color_to_zobrist(go_data_gen::Vec2 mem_coord, go_data_gen::Color color) {
    mem_coord.x -= go_data_gen::Board::padding;
    mem_coord.y -= go_data_gen::Board::padding;
    return zobrist_hashes[int(color) +
                          (mem_coord.x + mem_coord.y * go_data_gen::Board::max_board_size) * 3];
}

}  // namespace

namespace go_data_gen {

Board::Board(Vec2 _board_size, float _komi) : board_size{_board_size}, komi{_komi} {
    assert(board_size.x <= max_board_size && board_size.y <= max_board_size &&
           "Maximum size exceeded");

    PythonEnvironment::instance();  // Ensures Python is initialized

    init_zobrist();
    reset();
}

void Board::reset() {
    for (int i = 0; i < data_size; ++i) {
        for (int j = 0; j < data_size; ++j) {
            board[i][j] = char(OffBoard);
            if (i >= padding && j >= padding && i < padding + board_size.x &&
                j < padding + board_size.y) {
                board[i][j] = Empty;
            }

            parent[i][j] = {i, j};
            group[i][j].clear();
            liberties[i][j].clear();
        }
    }
    history.clear();
    zobrist = 0;
    zobrist_history = std::set<uint64_t>{zobrist};
}

void Board::setup_move(Move move) {
    if (move.coord == pass) {
        return;
    }

    // Shift to accommodate border stored in data fields
    move.coord.x += padding;
    move.coord.y += padding;

    board[move.coord.x][move.coord.y] = move.color;

    if (move.color == Black || move.color == White) {
        // Initialize new group
        parent[move.coord.x][move.coord.y] = move.coord;
        group[move.coord.x][move.coord.y].clear();
        group[move.coord.x][move.coord.y].push_back(move.coord);
        liberties[move.coord.x][move.coord.y].clear();
    }

    Vec2 neighbor, root, root2;

    // Update liberties and connect groups
    FOR_EACH_NEIGHBOR(
        move.coord, neighbor,
        if (move.color == Empty) {
            if (board[neighbor.x][neighbor.y] == Black || board[neighbor.x][neighbor.y] == White) {
                root = find(neighbor);
                liberties[root.x][root.y].insert(move.coord);
            }
        } else {
            const auto opp_col = opposite(move.color);
            if (board[neighbor.x][neighbor.y] == Empty) {
                root = find(move.coord);
                liberties[root.x][root.y].insert(neighbor);
            } else if (board[neighbor.x][neighbor.y] == move.color) {
                root = find(neighbor);
                liberties[root.x][root.y].erase(move.coord);
                unite(move.coord, neighbor);
            } else if (board[neighbor.x][neighbor.y] == opp_col) {
                root = find(neighbor);
                liberties[root.x][root.y].erase(move.coord);
            }
        }

    )
}

bool Board::is_legal(Move move) {
    if (move.coord == pass) {
        return true;
    }

    // Shift to accommodate border stored in data fields
    move.coord.x += padding;
    move.coord.y += padding;

    // Board must be empty
    if (board[move.coord.x][move.coord.y] != Empty) {
        // assert(false);
        return false;
    }

    // Simulate playing stone
    const auto opp_col = opposite(move.color);
    auto new_zobrist = zobrist ^ mem_coord_color_to_zobrist(move.coord, move.color);

    Vec2 neighbor, root;

    // Figure out liberties and captured groups
    // We need to do it in two passes to avoid simulating a removal of the same group multiple times
    // with even parity, which would cancel out the zobrist hash changes.
    std::set<Vec2> added_liberties;
    std::set<Vec2> captures;
    FOR_EACH_NEIGHBOR(
        move.coord, neighbor,
        if (board[neighbor.x][neighbor.y] == Empty) {
            added_liberties.insert(neighbor);
        } else if (board[neighbor.x][neighbor.y] == move.color) {
            root = find(neighbor);
            added_liberties.insert(liberties[root.x][root.y].begin(),
                                   liberties[root.x][root.y].end());
        } else if (board[neighbor.x][neighbor.y] == opp_col) {
            root = find(neighbor);
            if (liberties[root.x][root.y].size() == 1) {
                captures.insert(root);
            }
        })

    // Calculate zobrist if captured groups are removed
    for (auto capture : captures) {
        for (auto stone : group[capture.x][capture.y]) {
            new_zobrist ^= mem_coord_color_to_zobrist(stone, opp_col);
        }
    }

    // Must not be superko
    if (zobrist_history.find(new_zobrist) != zobrist_history.end()) {
        // assert(false);
        return false;
    }

    // Must not be suicide
    if (captures.empty()) {
        // Account for this move stealing last liberty of neighboring group without adding any
        added_liberties.erase(move.coord);
        if (added_liberties.empty()) {
            // assert(false);
            return false;
        }
    }

    return true;
}

void Board::play(Move move) {
    assert(is_legal(move));

    history.push_back(move);

    if (move.coord == pass) {
        return;
    }

    // Shift to accommodate border stored in data fields
    move.coord.x += padding;
    move.coord.y += padding;

    const auto opp_col = opposite(move.color);

    board[move.coord.x][move.coord.y] = move.color;

    // Initialize new group
    parent[move.coord.x][move.coord.y] = move.coord;
    group[move.coord.x][move.coord.y].clear();
    group[move.coord.x][move.coord.y].push_back(move.coord);
    liberties[move.coord.x][move.coord.y].clear();

    Vec2 neighbor, root, root2;

    // Add liberties, connect to own groups, and figure out captured groups
    FOR_EACH_NEIGHBOR(
        move.coord, neighbor,
        if (board[neighbor.x][neighbor.y] == Empty) {
            root = find(move.coord);
            liberties[root.x][root.y].insert(neighbor);
        } else if (board[neighbor.x][neighbor.y] == move.color) {
            root = find(neighbor);
            liberties[root.x][root.y].erase(move.coord);
            unite(move.coord, neighbor);
        } else if (board[neighbor.x][neighbor.y] == opp_col) {
            root = find(neighbor);
            liberties[root.x][root.y].erase(move.coord);
            if (liberties[root.x][root.y].size() == 0) {
                // Group is captured
                for (auto stone : group[root.x][root.y]) {
                    board[stone.x][stone.y] = Empty;
                    zobrist ^= mem_coord_color_to_zobrist(stone, opp_col);
                    // Nested macros, now we're entering the danger zone
                    FOR_EACH_NEIGHBOR(
                        stone, neighbor, if (board[neighbor.x][neighbor.y] == move.color) {
                            // The opposite of the opposite of the move color is the move color.
                            // Capturing a group of the opposite color frees liberties for the move
                            // color.
                            root2 = find(neighbor);  // Use root2 to avoid issues with outer loop
                            liberties[root2.x][root2.y].insert(stone);
                        })
                    // We don't need to maintain other data structures here.
                    // For debugging, clean up anyway.
                    // parent[stone.x][stone.y].x = stone.x;
                    // parent[stone.x][stone.y].y = stone.y;
                    // group[stone.x][stone.y].clear();
                    // liberties[stone.x][stone.y].clear();
                }
            }
        })

    // Update zobrist hash
    zobrist ^= mem_coord_color_to_zobrist(move.coord, move.color);
    zobrist_history.insert(zobrist);
}

py::array_t<float> Board::get_map(Color color, std::function<bool(int, int)> condition) {
    py::array_t<float> map({Board::data_size, Board::data_size});
    auto buf = map.mutable_unchecked<2>();
    for (int i = 0; i < Board::data_size; ++i) {
        for (int j = 0; j < Board::data_size; ++j) {
            // Board uses column-major ordering, numpy uses row-major by default.
            buf(i, j) = condition(j, i) ? 1.0 : 0.0;
        }
    }
    return map;
}

py::array_t<float> Board::get_mask() {
    return get_map(OffBoard, [this](int i, int j) { return board[i][j] != OffBoard; });
}

py::array_t<float> Board::get_legal_map(Color color) {
    return get_map(color, [this, color](int i, int j) {
        return is_legal({color, {i - padding, j - padding}});
    });
}

py::array_t<float> Board::get_stone_map(Color color) {
    return get_map(color, [this, color](int i, int j) { return board[i][j] == color; });
}

py::array_t<float> Board::get_history_map(int dist) {
    // dist = 0 implies the move just played
    // dist = 1 implies the 2nd-last move played, and so on
    if (dist >= history.size()) {
        return get_map(OffBoard, [](int, int) { return false; });
    }
    // Pass is encoded just outside the board area, within the padded area.
    const auto move = history.rbegin()[dist];
    return get_map(OffBoard, [this, move](int i, int j) {
        return Vec2{i - padding, j - padding} == move.coord;
    });
}

py::array_t<float> Board::get_liberty_map(Color color, int num, bool or_greater) {
    if (or_greater) {
        return get_map(color, [this, color, num](int i, int j) {
            if (board[i][j] == color) {
                const auto root = find(Vec2{i, j});
                if (liberties[root.x][root.y].size() >= num) {
                    return true;
                }
            }
            return false;
        });
    }
    return get_map(color, [this, color, num](int i, int j) {
        if (board[i][j] == color) {
            const auto root = find(Vec2{i, j});
            if (liberties[root.x][root.y].size() == num) {
                return true;
            }
        }
        return false;
    });
}

float Board::get_komi_from_player_perspective(Color to_play) {
    return to_play == White ? komi : -komi;
}

pybind11::tuple Board::get_nn_input_data(Color to_play) {
    const Color opp_col = opposite(to_play);

    // Stack the feature maps depth-wise
    py::array_t<float> stacked_maps({num_feature_planes, data_size, data_size});
    auto buf = stacked_maps.mutable_unchecked<3>();

    const auto mask = get_mask();
    const auto legal_map = get_legal_map(to_play);
    const auto stone_map_to_play = get_stone_map(to_play);
    const auto stone_map_opp_col = get_stone_map(opp_col);
    const auto hist_map_0 = get_history_map(0);
    const auto hist_map_1 = get_history_map(1);
    const auto hist_map_2 = get_history_map(2);
    const auto hist_map_3 = get_history_map(3);
    const auto hist_map_4 = get_history_map(4);
    const auto lib_map_own_1 = get_liberty_map(to_play, 1);
    const auto lib_map_opp_1 = get_liberty_map(opp_col, 1);
    const auto lib_map_own_2 = get_liberty_map(to_play, 2);
    const auto lib_map_opp_2 = get_liberty_map(opp_col, 2);
    const auto lib_map_own_3 = get_liberty_map(to_play, 3);
    const auto lib_map_opp_3 = get_liberty_map(opp_col, 3);
    const auto lib_map_own_4_or_gr = get_liberty_map(to_play, 4, true);
    const auto lib_map_opp_4_or_gr = get_liberty_map(opp_col, 4, true);
    for (int i = 0; i < data_size; ++i) {
        for (int j = 0; j < data_size; ++j) {
            buf(0, i, j) = mask.at(i, j);
            buf(1, i, j) = legal_map.at(i, j);
            buf(2, i, j) = stone_map_to_play.at(i, j);
            buf(3, i, j) = stone_map_opp_col.at(i, j);
            buf(4, i, j) = hist_map_0.at(i, j);
            buf(5, i, j) = hist_map_1.at(i, j);
            buf(6, i, j) = hist_map_2.at(i, j);
            buf(7, i, j) = hist_map_3.at(i, j);
            buf(8, i, j) = hist_map_4.at(i, j);
            buf(9, i, j) = lib_map_own_1.at(i, j);
            buf(10, i, j) = lib_map_opp_1.at(i, j);
            buf(11, i, j) = lib_map_own_2.at(i, j);
            buf(12, i, j) = lib_map_opp_2.at(i, j);
            buf(13, i, j) = lib_map_own_3.at(i, j);
            buf(14, i, j) = lib_map_opp_3.at(i, j);
            buf(15, i, j) = lib_map_own_4_or_gr.at(i, j);
            buf(16, i, j) = lib_map_opp_4_or_gr.at(i, j);
        }
    }

    // Create a 1D array for the scalar features.
    py::array_t<float> features({num_feature_scalars});
    auto feat_buf = features.mutable_unchecked<1>();
    feat_buf(0) = get_komi_from_player_perspective(to_play);

    return py::make_tuple(stacked_maps, features);
}

void Board::print(PrintMode mode) {
    printf("   ");
    if (mode == Default) {
        printf(" ");
    }
    for (int col = 0; col < board_size.x; ++col) {
        if (mode == Liberties) {
            printf(" ");
        }
        printf("%c ", static_cast<char>(col < 8 ? 'A' + col : 'B' + col));
    }
    printf("\n");

    Vec2 root;
    Vec2 last_move_coord = history.empty() ? pass : history.back().coord;

    for (int row = 0; row < board_size.y; ++row) {
        printf("%2d ", board_size.y - row);

        if (mode == Default) {
            printf("\033[48;5;94m \033[0m");
        }

        for (int col = 0; col < board_size.x; ++col) {
            switch (mode) {
            case Default:
                // Shift to accommodate border
                switch (board[col + padding][row + padding]) {
                case Empty:
                    if (col == 0 && row == 0)
                        printf("\033[48;5;94m\033[38;5;0m┌─\033[0m");
                    else if (col == board_size.x - 1 && row == 0)
                        printf("\033[48;5;94m\033[38;5;0m┐ \033[0m");
                    else if (col == 0 && row == board_size.y - 1)
                        printf("\033[48;5;94m\033[38;5;0m└─\033[0m");
                    else if (col == board_size.x - 1 && row == board_size.y - 1)
                        printf("\033[48;5;94m\033[38;5;0m┘ \033[0m");
                    else if (col == 0)
                        printf("\033[48;5;94m\033[38;5;0m├─\033[0m");
                    else if (col == board_size.x - 1)
                        printf("\033[48;5;94m\033[38;5;0m┤ \033[0m");
                    else if (row == 0)
                        printf("\033[48;5;94m\033[38;5;0m┬─\033[0m");
                    else if (row == board_size.y - 1)
                        printf("\033[48;5;94m\033[38;5;0m┴─\033[0m");
                    else
                        printf("\033[48;5;94m\033[38;5;0m┼─\033[0m");
                    break;
                case Black:
                    if (col == last_move_coord.x && row == last_move_coord.y) {
                        printf("\033[48;5;208m\033[38;5;0m● \033[0m");
                    } else {
                        printf("\033[48;5;94m\033[38;5;0m● \033[0m");
                    }
                    break;
                case White:
                    if (col == last_move_coord.x && row == last_move_coord.y) {
                        printf("\033[48;5;208m\033[38;5;15m● \033[0m");
                    } else {
                        printf("\033[48;5;94m\033[38;5;15m● \033[0m");
                    }
                    break;
                }
                break;
            case GroupSize:
                root = find({col + padding, row + padding});
                if (group[root.x][root.y].size() > 0) {
                    printf("%2ld ", group[root.x][root.y].size());
                } else {
                    printf("   ");
                }
                break;
            case Liberties:
                root = find({col + padding, row + padding});
                if (liberties[root.x][root.y].size() > 0) {
                    printf("%2ld ", liberties[root.x][root.y].size());
                } else {
                    printf(" . ");
                }
                break;
            case IllegalMovesBlack:
            case IllegalMovesWhite:
                const Move move = {mode == IllegalMovesBlack ? Black : White, {col, row}};
                if (board[col + padding][row + padding] == Empty && !is_legal(move)) {
                    printf("0 ");
                } else {
                    printf(". ");
                }
                break;
            }
        }
        printf("\n");
    }
}

void Board::print_feature_planes(Color to_play, int feature_plane_index) {
    auto nn_input = get_nn_input_data(to_play);
    auto feature_planes = py::cast<py::array_t<float>>(nn_input[0]);
    auto feature_plane = feature_planes.mutable_unchecked<3>();

    // Print column headers
    printf("    ");
    for (int col = 0; col < data_size; ++col) {
        if (col >= padding && col < padding + board_size.x) {
            char col_label = static_cast<char>((col - padding) < 8 ? 'A' + (col - padding)
                                                                   : 'B' + (col - padding));
            printf("%c ", col_label);
        } else {
            printf("  ");
        }
    }
    printf("\n");

    for (int row = 0; row < data_size; ++row) {
        if (row >= padding && row < padding + board_size.y) {
            printf("%2d  ", board_size.y - (row - padding));
        } else {
            printf("    ");
        }

        for (int col = 0; col < data_size; ++col) {
            float value = feature_plane(feature_plane_index, row, col);
            bool on_board = (row >= padding && row < padding + board_size.y && col >= padding &&
                             col < padding + board_size.x);

            const char* bg_color = (value == 1.0f) ? "\033[48;5;14m" :  // Cyan
                                       (on_board) ? "\033[48;5;94m"
                                                  :     // Brown
                                       "\033[48;5;0m";  // Black

            if (on_board) {
                Color stone = Color(board[col][row]);
                if (stone == Empty) {
                    if (col == padding && row == padding)
                        printf("%s\033[38;5;0m┌─\033[0m", bg_color);
                    else if (col == padding + board_size.x - 1 && row == padding)
                        printf("%s\033[38;5;0m┐ \033[0m", bg_color);
                    else if (col == padding && row == padding + board_size.y - 1)
                        printf("%s\033[38;5;0m└─\033[0m", bg_color);
                    else if (col == padding + board_size.x - 1 && row == padding + board_size.y - 1)
                        printf("%s\033[38;5;0m┘ \033[0m", bg_color);
                    else if (col == padding)
                        printf("%s\033[38;5;0m├─\033[0m", bg_color);
                    else if (col == padding + board_size.x - 1)
                        printf("%s\033[38;5;0m┤ \033[0m", bg_color);
                    else if (row == padding)
                        printf("%s\033[38;5;0m┬─\033[0m", bg_color);
                    else if (row == padding + board_size.y - 1)
                        printf("%s\033[38;5;0m┴─\033[0m", bg_color);
                    else
                        printf("%s\033[38;5;0m┼─\033[0m", bg_color);
                } else if (stone == Black) {
                    printf("%s\033[38;5;0m● \033[0m", bg_color);
                } else if (stone == White) {
                    printf("%s\033[38;5;15m● \033[0m", bg_color);
                }
            } else {
                const char* fg_color = (value == 1.0f) ? "\033[38;5;0m" : "\033[38;5;15m";
                printf("%s%s# \033[0m", bg_color, fg_color);
            }
        }
        printf("\n");
    }
}

Vec2 Board::find(Vec2 coord) {
    while (parent[coord.x][coord.y] != coord) {
        Vec2& coord_parent = parent[coord.x][coord.y];
        coord_parent = parent[coord_parent.x][coord_parent.y];
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

    if (group[a.x][a.y].size() < group[b.x][b.y].size()) {
        const Vec2 tmp = a;
        a = b;
        b = tmp;
    }

    parent[b.x][b.y] = a;
    group[a.x][a.y].insert(std::end(group[a.x][a.y]), std::begin(group[b.x][b.y]),
                           std::end(group[b.x][b.y]));
    liberties[a.x][a.y].insert(liberties[b.x][b.y].begin(), liberties[b.x][b.y].end());
}

}  // namespace go_data_gen
