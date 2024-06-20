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

    load_sgf(file_path, board, moves);

    int i = 1;
    for (const auto& move : moves) {
        board.play(move);
        std::cout << "Move no. " << i++ << ":\n";
        board.print();
        std::cout << std::endl;
    }

    return 0;
}
