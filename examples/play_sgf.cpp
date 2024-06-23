#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "go_data_gen/board.hpp"
#include "go_data_gen/sgf.hpp"

using namespace go_data_gen;

namespace py = pybind11;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <sgf_file_path>" << std::endl;
        return 1;
    }

    std::string file_path = argv[1];
    Board board;
    std::vector<Move> moves;
    float result;

    load_sgf(file_path, board, moves, result);

    int i = 1;
    for (const auto& move : moves) {
        board.play(move);
        std::cout << "Move no. " << i++ << ":\n";
        board.print();

        std::cout << std::endl;
        std::cout << "Illegal moves for " << (opposite(move.color) == Black ? "Black" : "White")
                  << ":\n";
        board.print(opposite(move.color) == Black ? Board::PrintMode::IllegalMovesBlack
                                                  : Board::PrintMode::IllegalMovesWhite);

        printf("Liberty size: \n");
        board.print(Board::PrintMode::Liberties);

        // Check after 75 moves
        if (i == 75) {
            py::tuple nn_input_data = board.get_nn_input_data(opposite(move.color));
            py::array_t<float> stacked_maps = nn_input_data[0].cast<py::array_t<float>>();
            py::array_t<float> features = nn_input_data[1].cast<py::array_t<float>>();

            std::cout << "NN Input Data after 75 moves:" << std::endl;
            std::cout << "Stacked Maps:" << std::endl;

            // Split the stacked maps into individual 2D maps and print them
            auto stacked_maps_unchecked = stacked_maps.unchecked<3>();
            for (py::ssize_t idx = 0; idx < stacked_maps_unchecked.shape(0); ++idx) {
                std::cout << "Map " << (idx + 1) << ":" << std::endl;
                for (py::ssize_t row = 0; row < stacked_maps_unchecked.shape(1); ++row) {
                    for (py::ssize_t col = 0; col < stacked_maps_unchecked.shape(2); ++col) {
                        std::cout << std::setw(2)
                                  << static_cast<int>(stacked_maps_unchecked(idx, row, col)) << " ";
                    }
                    std::cout << std::endl;
                }
                std::cout << std::endl;
            }

            std::cout << "Features:" << std::endl;
            // Print the features
            auto features_unchecked = features.unchecked<1>();
            for (py::ssize_t i = 0; i < features_unchecked.shape(0); ++i) {
                std::cout << features_unchecked(i) << " ";
            }
            std::cout << std::endl << std::endl;
        }

        printf("Feature plane 15:\n");
        board.print_feature_planes(opposite(move.color), 15);
    }

    std::cout << "Result: ";
    if (result < 0) {
        std::cout << "B+" << std::setprecision(1) << std::fixed << -result << std::endl;
    } else if (result > 0) {
        std::cout << "W+" << std::setprecision(1) << std::fixed << result << std::endl;
    } else {
        std::cout << "0" << std::endl;
    }

    return 0;
}
