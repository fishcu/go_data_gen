#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "go_data_gen/board.hpp"
#include "go_data_gen/sgf.hpp"

using namespace go_data_gen;

namespace py = pybind11;

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
        printf("Playing move no. %d, which is: (%d, %d)\n", i, move.coord.x, move.coord.y);
        board.play(move);
        board.print();

        printf("Illegal moves for %s:\n", (opposite(move.color) == Black ? "Black" : "White"));
        board.print(opposite(move.color) == Black ? Board::PrintMode::IllegalMovesBlack
                                                  : Board::PrintMode::IllegalMovesWhite);

        printf("Liberty size: \n");
        board.print(Board::PrintMode::Liberties);

        // Check after 75 moves
        if (i == 75) {
            py::tuple nn_input_data = board.get_nn_input_data(opposite(move.color));
            py::array_t<float> stacked_maps = nn_input_data[0].cast<py::array_t<float>>();
            py::array_t<float> features = nn_input_data[1].cast<py::array_t<float>>();

            printf("NN Input Data after 75 moves:\n");
            printf("Stacked Maps:\n");

            // Split the stacked maps into individual 2D maps and print them
            auto stacked_maps_unchecked = stacked_maps.unchecked<3>();
            for (py::ssize_t idx = 0; idx < stacked_maps_unchecked.shape(0); ++idx) {
                printf("Map %ld:\n", idx + 1);
                for (py::ssize_t row = 0; row < stacked_maps_unchecked.shape(1); ++row) {
                    for (py::ssize_t col = 0; col < stacked_maps_unchecked.shape(2); ++col) {
                        printf("%2d ", static_cast<int>(stacked_maps_unchecked(idx, row, col)));
                    }
                    printf("\n");
                }
                printf("\n");
            }

            printf("Features:\n");
            // Print the features
            auto features_unchecked = features.unchecked<1>();
            for (py::ssize_t i = 0; i < features_unchecked.shape(0); ++i) {
                printf("%f ", features_unchecked(i));
            }
            printf("\n\n");
        }

        printf("Feature plane 15:\n");
        board.print_feature_planes(opposite(move.color), 15);

        ++i;
    }

    printf("Result: ");
    if (result < 0) {
        printf("B+%.1f\n", -result);
    } else if (result > 0) {
        printf("W+%.1f\n", result);
    } else {
        printf("0\n");
    }

    return 0;
}
