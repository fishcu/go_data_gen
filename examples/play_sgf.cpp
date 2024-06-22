#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "go_data_gen/board.hpp"
#include "go_data_gen/sgf.hpp"

using namespace go_data_gen;

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
        // if (move.color == Black || move.color == White) {
        //     std::cout << "Illegal moves for " << (opposite(move.color) == Black ? "Black" :
        //     "White")
        //               << ":\n";
        //     board.print(opposite(move.color) == Black ? Board::PrintMode::IllegalMovesBlack
        //                                               : Board::PrintMode::IllegalMovesWhite);
        // }
        // printf("Liberty size: \n");
        // board.print(Board::PrintMode::Liberties);

        // Check after 75 moves
        if (i == 75) {
            auto nn_input_data = board.get_nn_input_data(opposite(move.color));
            auto [stacked_maps, features] = nn_input_data;
            std::cout << "NN Input Data after 75 moves:" << std::endl;
            std::cout << "Stacked Maps:" << std::endl;

            // Split the stacked maps into individual 2D maps and print them
            for (int64_t idx = 0; idx < stacked_maps.size(0); ++idx) {
                std::cout << "Map " << (idx + 1) << ":" << std::endl;
                for (int64_t row = 0; row < stacked_maps.size(1); ++row) {
                    for (int64_t col = 0; col < stacked_maps.size(2); ++col) {
                        std::cout << std::setw(2)
                                  << static_cast<int>(stacked_maps[idx][row][col].item<float>())
                                  << " ";
                    }
                    std::cout << std::endl;
                }
                std::cout << std::endl;
            }

            std::cout << "Features:" << std::endl;
            // Print the features
            std::cout << features << std::endl << std::endl;
        }
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
