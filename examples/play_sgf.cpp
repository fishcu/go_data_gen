#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "go_data_gen/board.hpp"
#include "go_data_gen/sgf.hpp"

using namespace go_data_gen;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <sgf_file_path>\n", argv[0]);
        return 1;
    }

    std::string file_path = argv[1];
    Board board;
    std::vector<Move> moves;
    float result;

    load_sgf(file_path, board, moves, result);

    printf("Board after setup:\n");
    board.print();

    int i = 1;
    for (const auto& move : moves) {
        printf("Playing move no. %d, which is: %s ", i,
               move.color == Color::Black ? "Black" : "White");
        if (move.is_pass) {
            printf("pass\n");
        } else {
            printf("(%d, %d)\n", move.coord.x, move.coord.y);
        }
        board.play(move);
        board.print();

        // Check after some moves
        if (i >= 180 && i <= 183 || i == 224) {
            printf("NN Input Data after %d moves:\n", i);
            const auto feature_planes = board.get_feature_planes(opposite(move.color));
            assert(feature_planes[0][0].size() == board.num_feature_planes);
            for (int c = 0; c < board.num_feature_planes; ++c) {
                printf("Feature plane %d: \n", c);
                board.print([&feature_planes, c](int x, int y) {
                    return static_cast<bool>(feature_planes[y][x][c]);
                });
            }
            board.print_group_sizes();
            board.print_liberties();
            const auto feature_vector = board.get_feature_scalars(opposite(move.color));
            assert(feature_vector.size() == board.num_feature_scalars);
            for (int c = 0; c < board.num_feature_scalars; ++c) {
                printf("Scalar feature %d = %f\n", c, feature_vector[c]);
            }
        }

        ++i;
    }

    printf("Result: ");
    if (result > 0) {
        printf("B+%.1f\n", result);
    } else if (result < 0) {
        printf("W+%.1f\n", -result);
    } else {
        printf("Draw or void game\n");
    }

    return 0;
}
