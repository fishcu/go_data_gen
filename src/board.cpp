#include "go_data_gen/board.hpp"

#include "go_data_gen/types.hpp"

namespace go_data_gen {

class Board::Board(Vec2 _size, float _komi) : size{_size}, komi{_komi} {
    assert(size.x <= Board::max_size && size.y <= Board::max_size && "Maximum size exceeded");

    reset();
}

void Board::reset() {
    for (int i = 0; i < max_size + 2; ++i) {
        for (int j = 0; j < max_size + 2; ++j) {
            board[i][j] = char(Empty);
            if (i == 0 || j == 0 || i == size.x || j == size.y) {
                board[i][j] = OffBoard;
            }

            parent[i][j] = Vec2{i, j};
            size[i][j] = 0;

            liberties[i][j] = std::set<Vec2>();
        }
    }
}

void Board::play(Move move) {
    // TODO
}


Vec2 Board::find(Vec2 coord) {
    while (parent[coord.x][coord.y] != coord) {
        Vec2 & coord_parent = parent[coord.x][coord.y];
        coord_parent = parent[coord_parent.x][coord_parent.y];
        coord = coord_parent;
    }
    return coord;
}

void Board::union(Vec2 a, Vec2 b) {
    a = find(a);
    b = find(b);
    if (a == b) {
        return;
    }

    if (size[a.x][a.y] < size[b.x][b.y]) {
        const Vec2 tmp = a;
        a = b;
        b = tmp;
    }

    parent[b.x][b.y] = a;
    size[a.x][a.y] += size[b.x][b.y];
}

}  // namespace go_data_gen