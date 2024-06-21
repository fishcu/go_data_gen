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
