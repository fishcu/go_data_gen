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
    zobrist_hashes[go_data_gen::Board::max_board_size * go_data_gen::Board::max_board_size * 2];

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
    assert(color == go_data_gen::Color::Black || color == go_data_gen::Color::White);
    mem_coord.x -= go_data_gen::Board::padding;
    mem_coord.y -= go_data_gen::Board::padding;
    return zobrist_hashes[(color == go_data_gen::Color::Black ? 0 : 1) +
                          (mem_coord.x + mem_coord.y * go_data_gen::Board::max_board_size) * 2];
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
    for (int y = 0; y < data_size; ++y) {
        for (int x = 0; x < data_size; ++x) {
            board[y][x] = char(OffBoard);
            if (y >= padding && x >= padding && y < padding + board_size.y &&
                x < padding + board_size.x) {
                board[y][x] = Empty;
            }

            parent[y][x] = {x, y};
            group[y][x].clear();
            liberties[y][x].clear();
        }
    }
    history.clear();
    zobrist = 0;
    zobrist_history = std::set<uint64_t>{zobrist};
}

void Board::setup_move(Move move) {
    assert(!move.is_pass);
    assert(move.color != OffBoard);

    // Shift coordinate to account for padding of data fields.
    const Vec2 mem_coord{move.coord.x + padding, move.coord.y + padding};

    board[mem_coord.y][mem_coord.x] = move.color;

    if (move.color == Black || move.color == White) {
        // Initialize new group
        parent[mem_coord.y][mem_coord.x] = mem_coord;
        group[mem_coord.y][mem_coord.x].clear();
        group[mem_coord.y][mem_coord.x].push_back(mem_coord);
        liberties[mem_coord.y][mem_coord.x].clear();
    }

    Vec2 neighbor, root;

    // Update liberties and connect groups
    FOR_EACH_NEIGHBOR(
        mem_coord, neighbor,
        if (move.color == Empty) {
            if (board[neighbor.y][neighbor.x] == Black || board[neighbor.y][neighbor.x] == White) {
                root = find(neighbor);
                liberties[root.y][root.x].insert(mem_coord);
            }
        } else {
            const auto opp_col = opposite(move.color);
            if (board[neighbor.y][neighbor.x] == Empty) {
                root = find(mem_coord);
                liberties[root.y][root.x].insert(neighbor);
            } else if (board[neighbor.y][neighbor.x] == move.color) {
                root = find(neighbor);
                liberties[root.y][root.x].erase(mem_coord);
                unite(mem_coord, neighbor);
            } else if (board[neighbor.y][neighbor.x] == opp_col) {
                root = find(neighbor);
                liberties[root.y][root.x].erase(mem_coord);
            }
        }

    )
}

MoveLegality Board::get_move_legality(Move move) {
    assert(move.color == Black || move.color == White);

    if (move.is_pass) {
        return Legal;
    }

    // Shift coordinate to account for padding of data fields.
    const Vec2 mem_coord{move.coord.x + padding, move.coord.y + padding};

    // Board must be empty
    if (board[mem_coord.y][mem_coord.x] != Empty) {
        return NonEmpty;
    }

    // Simulate playing stone
    const auto opp_col = opposite(move.color);
    auto new_zobrist = zobrist ^ mem_coord_color_to_zobrist(mem_coord, move.color);

    Vec2 neighbor, root;

    // Figure out liberties and captured groups
    // We need to do it in two passes to avoid simulating a removal of the same group multiple times
    // with even parity, which would cancel out the zobrist hash changes.
    std::set<Vec2> added_liberties;
    std::set<Vec2> captures;
    FOR_EACH_NEIGHBOR(
        mem_coord, neighbor,
        if (board[neighbor.y][neighbor.x] == Empty) {
            added_liberties.insert(neighbor);
        } else if (board[neighbor.y][neighbor.x] == move.color) {
            root = find(neighbor);
            added_liberties.insert(liberties[root.y][root.x].begin(),
                                   liberties[root.y][root.x].end());
        } else if (board[neighbor.y][neighbor.x] == opp_col) {
            root = find(neighbor);
            if (liberties[root.y][root.x].size() == 1) {
                captures.insert(root);
            }
        })

    // Must not be suicide
    if (captures.empty()) {
        // Account for this move stealing last liberty of neighboring group without adding any
        added_liberties.erase(mem_coord);
        if (added_liberties.empty()) {
            // assert(false);
            return Suicidal;
        }
    }

    // Calculate zobrist if captured groups are removed
    for (auto capture : captures) {
        for (auto stone : group[capture.y][capture.x]) {
            new_zobrist ^= mem_coord_color_to_zobrist(stone, opp_col);
        }
    }

    // Must not be superko
    if (zobrist_history.find(new_zobrist) != zobrist_history.end()) {
        // assert(false);
        return Superko;
    }

    return Legal;
}

void Board::play(Move move) {
    assert(move.color == Black || move.color == White);
    assert(get_move_legality(move) == Legal);

    history.push_back(move);

    if (move.is_pass) {
        return;
    }

    // Shift coordinate to account for padding of data fields.
    const Vec2 mem_coord{move.coord.x + padding, move.coord.y + padding};

    const auto opp_col = opposite(move.color);

    board[mem_coord.y][mem_coord.x] = move.color;

    // Initialize new group
    parent[mem_coord.y][mem_coord.x] = mem_coord;
    group[mem_coord.y][mem_coord.x].clear();
    group[mem_coord.y][mem_coord.x].push_back(mem_coord);
    liberties[mem_coord.y][mem_coord.x].clear();

    Vec2 neighbor, root, root2;

    // Add liberties, connect to own groups, and figure out captured groups
    FOR_EACH_NEIGHBOR(
        mem_coord, neighbor,
        if (board[neighbor.y][neighbor.x] == Empty) {
            root = find(mem_coord);
            liberties[root.y][root.x].insert(neighbor);
        } else if (board[neighbor.y][neighbor.x] == move.color) {
            root = find(neighbor);
            liberties[root.y][root.x].erase(mem_coord);
            unite(mem_coord, neighbor);
        } else if (board[neighbor.y][neighbor.x] == opp_col) {
            root = find(neighbor);
            liberties[root.y][root.x].erase(mem_coord);
            if (liberties[root.y][root.x].size() == 0) {
                // Group is captured
                for (auto stone : group[root.y][root.x]) {
                    board[stone.y][stone.x] = Empty;
                    zobrist ^= mem_coord_color_to_zobrist(stone, opp_col);
                    // Nested macros, now we're entering the danger zone
                    FOR_EACH_NEIGHBOR(
                        stone, neighbor,
                        // The opposite of the opposite of the move color is the move color.
                        if (board[neighbor.y][neighbor.x] == move.color) {
                            // Capturing a group of the opposite color frees liberties for the move
                            // color.
                            root2 = find(neighbor);  // Use root2 to avoid issues with outer loop
                            liberties[root2.y][root2.x].insert(stone);
                        })
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
        })

    // Update zobrist hash
    zobrist ^= mem_coord_color_to_zobrist(mem_coord, move.color);
    zobrist_history.insert(zobrist);
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
            result[y][x][0] = static_cast<float>(move_legality == Legal);
            // Own color
            result[y][x][1] = static_cast<float>(board[y][x] == to_play);
            // Opponent color
            result[y][x][2] = static_cast<float>(board[y][x] == opp_col);
            // Is on-board
            result[y][x][3] = static_cast<float>(board[y][x] != OffBoard);

            // Mark superko
            result[y][x][4] = static_cast<float>(move_legality == Superko);

            // Liberties of own and opponent groups
            if (board[y][x] == Black || board[y][x] == White) {
                const auto root = find(Vec2{x, y});
                const int num_libs = liberties[root.y][root.x].size();
                if (board[y][x] == to_play) {
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

Board::FeatureVector Board::get_scalar_features(Color to_play) {
    static constexpr int num_features_before_pass_features = 2;
    static constexpr int num_pass_features = 5;
    static_assert(num_feature_scalars == num_features_before_pass_features + num_pass_features);

    // Zero-initialize.
    FeatureVector result{};

    // Komi from player perspective. Normalize like KataGo.
    result[0] = (to_play == White ? komi : -komi) / 15.0;

    // There is a superko move
    result[1] = static_cast<float>(any_superko_move(to_play));

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

pybind11::tuple Board::get_nn_input_data(Color to_play) {
    namespace py = pybind11;

    // Get feature planes and scalar features
    auto feature_planes = get_feature_planes(to_play);
    auto scalar_features = get_scalar_features(to_play);

    // Create numpy array for feature planes
    auto features_array = py::array_t<float>({data_size, data_size, num_feature_planes});
    auto features_mu = features_array.mutable_unchecked<3>();

    // Copy feature planes data using mutable_unchecked
    for (size_t y = 0; y < data_size; ++y) {
        for (size_t x = 0; x < data_size; ++x) {
            for (size_t c = 0; c < num_feature_planes; ++c) {
                features_mu(y, x, c) = feature_planes[y][x][c];
            }
        }
    }

    // Create numpy array for scalar features
    auto scalars_array = py::array_t<float>({num_feature_scalars});

    // Copy scalar features using memcpy for efficiency
    auto scalars_mu = scalars_array.mutable_unchecked<1>();
    std::memcpy(scalars_mu.mutable_data(0), scalar_features.data(),
                num_feature_scalars * sizeof(float));

    return py::make_tuple(features_array, scalars_array);
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

bool Board::any_superko_move(Color to_play) {
    for (int y = 0; y < board_size.y; ++y) {
        for (int x = 0; x < board_size.x; ++x) {
            const auto move_legality = get_move_legality(Move{to_play, false, {x, y}});
            if (move_legality == Superko) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace go_data_gen
