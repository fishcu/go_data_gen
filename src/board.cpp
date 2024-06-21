#include "go_data_gen/board.hpp"

#include <iomanip>
#include <iostream>
#include <random>

#include "go_data_gen/types.hpp"

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

uint64_t zobrist_hashes[go_data_gen::Board::max_size * go_data_gen::Board::max_size * 3];

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
                          (mem_coord.x + mem_coord.y * go_data_gen::Board::max_size) * 3];
}

}  // namespace

namespace go_data_gen {

Board::Board(Vec2 _size, float _komi) : size{_size}, komi{_komi} {
    assert(size.x <= Board::max_size && size.y <= Board::max_size && "Maximum size exceeded");

    init_zobrist();
    reset();
}

void Board::reset() {
    for (int i = 0; i < max_size + 2 * padding; ++i) {
        for (int j = 0; j < max_size + 2 * padding; ++j) {
            board[i][j] = char(OffBoard);
            if (i > 0 && j > 0 && i <= size.x && j <= size.y) {
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

bool Board::is_legal(Move move) {
    if (move == pass) {
        return true;
    }

    // Shift to accommodate border stored in data fields
    move.coord.x += go_data_gen::Board::padding;
    move.coord.y += go_data_gen::Board::padding;

    // Board must be empty
    if (board[move.coord.x][move.coord.y] != Empty) {
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
        return false;
    }

    // Must not be suicide
    if (captures.empty()) {
        // Account for this move stealing last liberty of neighboring group without adding any
        added_liberties.erase(move.coord);
        if (added_liberties.empty()) {
            return false;
        }
    }

    return true;
}

void Board::play(Move move) {
    assert(is_legal(move));

    history.push_back(move);

    if (move == pass) {
        return;
    }

    // Shift to accommodate border stored in data fields
    move.coord.x += go_data_gen::Board::padding;
    move.coord.y += go_data_gen::Board::padding;

    const auto opp_col = opposite(move.color);

    // Initialize new group
    board[move.coord.x][move.coord.y] = move.color;
    group[move.coord.x][move.coord.y].clear();
    group[move.coord.x][move.coord.y].push_back(move.coord);
    liberties[move.coord.x][move.coord.y].clear();

    Vec2 neighbor, root, root2;

    // Add liberties, connect to own groups, and figure out captured groups
    std::set<Vec2> captures;
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

void Board::print(PrintMode mode) {
    std::cout << "   ";
    for (int col = 0; col < size.x; ++col) {
        if (mode == Liberties) {
            std::cout << " ";
        }
        std::cout << static_cast<char>(col < 8 ? 'A' + col : 'B' + col) << " ";
    }
    std::cout << std::endl;

    Vec2 root;
    Vec2 lastMove = history.back().coord;

    for (int row = 0; row < size.y; ++row) {
        std::cout << std::setw(2) << (size.y - row) << " ";

        for (int col = 0; col < size.x; ++col) {
            switch (mode) {
            case Default:
                // Shift  to accommodate border
                switch (board[col + padding][row + padding]) {
                case Empty:
                    std::cout << "\033[48;5;94m\033[38;5;0m. \033[0m";
                    break;
                case Black:
                    if (col == lastMove.x && row == lastMove.y) {
                        std::cout << "\033[48;5;208m\033[38;5;0m● \033[0m";
                    } else {
                        std::cout << "\033[48;5;94m\033[38;5;0m● \033[0m";
                    }
                    break;
                case White:
                    if (col == lastMove.x && row == lastMove.y) {
                        std::cout << "\033[48;5;208m\033[38;5;15m● \033[0m";
                    } else {
                        std::cout << "\033[48;5;94m\033[38;5;15m● \033[0m";
                    }
                    break;
                }
                break;
            case GroupSize:
                root = find({col + padding, row + padding});
                if (group[root.x][root.y].size() > 0) {
                    std::cout << std::setw(2) << group[root.x][root.y].size() << " ";
                } else {
                    std::cout << "   ";
                }
                break;
            case Liberties:
                root = find({col + padding, row + padding});
                if (liberties[root.x][root.y].size() > 0) {
                    std::cout << std::setw(2) << liberties[root.x][root.y].size() << " ";
                } else {
                    std::cout << " . ";
                }
                break;
            case IllegalMovesBlack:
            case IllegalMovesWhite:
                const Move move = {mode == IllegalMovesBlack ? Black : White, {col, row}};
                if (board[col + padding][row + padding] == Empty && !is_legal(move)) {
                    std::cout << "0 ";
                } else {
                    std::cout << ". ";
                }
                break;
            }
        }
        std::cout << std::endl;
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
