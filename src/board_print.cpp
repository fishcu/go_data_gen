#include "go_data_gen/board_print.hpp"

namespace go_data_gen {

void BoardPrinter::print(Board& board, std::optional<int> highlight_feature) {
    // Get feature planes if required

    // TODO this is broken.
    // First, refactor NN feature planes to return std::array, which can be better handled here.
    py::detail::unchecked_mutable_reference<float, 3> feature_plane;
    if (highlight_feature) {
        const auto to_play = board.history.empty() ? Black : opposite(board.history.back().color);
        auto nn_input = board.get_nn_input_data(to_play);
        auto feature_planes = py::cast<py::array_t<float>>(nn_input[0]);
        feature_plane = feature_planes.mutable_unchecked<3>();
    }

    bool show_padding = highlight_feature.has_value();

    printf("   ");
    if (!include_padding) {
        printf(" ");
    }

    int start = include_padding ? 0 : board.padding;
    int end = include_padding ? board.data_size : board.padding + board.board_size.x;

    for (int col = start; col < end; ++col) {
        if (include_padding && !is_on_board(board, board.padding, col)) {
            printf("  ");
        } else {
            int board_col = include_padding ? col - board.padding : col - board.padding;
            printf("%c ", static_cast<char>(board_col < 8 ? 'A' + board_col : 'B' + board_col));
        }
    }
    printf("\n");

    int start_row = show_padding ? 0 : 0;
    int end_row = show_padding ? board.data_size : board.board_size.y;
    int start_col = show_padding ? 0 : 0;
    int end_col = show_padding ? board.data_size : board.board_size.x;

    for (int row = start_row; row < end_row; ++row) {
        // Row number
        if (is_on_board(board, row + board.padding, board.padding)) {
            printf("%2d ", board.board_size.y - (row));
        } else if (show_padding) {
            printf("    ");
        }

        // Board margin
        if (!show_padding) {
            printf("\033[48;5;94m \033[0m");
        }

        for (int col = start_col; col < end_col; ++col) {
            int board_row = row + (show_padding ? 0 : board.padding);
            int board_col = col + (show_padding ? 0 : board.padding);
            bool on_board = is_on_board(board, board_row, board_col);

            // Determine background color
            std::string bg_color;
            if (feature_plane) {
                float value = (*feature_plane)(*highlight_feature, board_row, board_col);
                bg_color = (value == 1.0f) ? "\033[48;5;14m"
                                           : (on_board ? "\033[48;5;94m" : "\033[48;5;0m");
            } else {
                bool is_last_move = !board.history.empty() && !board.history.back().is_pass &&
                                    col == board.history.back().coord.x &&
                                    row == board.history.back().coord.y;
                bg_color = is_last_move ? "\033[48;5;208m" : "\033[48;5;94m";
            }

            if (on_board) {
                Color stone = board.board[board_col][board_row];
                if (stone == Empty) {
                    const char* symbol;
                    bool is_edge_row = (row == 0 || row == board.board_size.y - 1);
                    bool is_edge_col = (col == 0 || col == board.board_size.x - 1);

                    if (col == 0 && row == 0)
                        symbol = "┌─";
                    else if (col == board.board_size.x - 1 && row == 0)
                        symbol = "┐ ";
                    else if (col == 0 && row == board.board_size.y - 1)
                        symbol = "└─";
                    else if (col == board.board_size.x - 1 && row == board.board_size.y - 1)
                        symbol = "┘ ";
                    else if (col == 0)
                        symbol = "├─";
                    else if (col == board.board_size.x - 1)
                        symbol = "┤ ";
                    else if (row == 0)
                        symbol = "┬─";
                    else if (row == board.board_size.y - 1)
                        symbol = "┴─";
                    else
                        symbol = "┼─";

                    printf("%s\033[38;5;0m%s\033[0m", bg_color.c_str(), symbol);
                } else {
                    std::string fg_color = (stone == Black) ? "\033[38;5;0m" : "\033[38;5;15m";
                    printf("%s%s● \033[0m", bg_color.c_str(), fg_color.c_str());
                }
            } else if (show_padding) {
                // Print padding area
                float value = (*feature_plane)(*highlight_feature, board_row, board_col);
                std::string fg_color = (value == 1.0f) ? "\033[38;5;0m" : "\033[38;5;15m";
                printf("%s%s# \033[0m", bg_color.c_str(), fg_color.c_str());
            }
        }
        printf("\n");
    }
}

void BoardPrinter::print_group_sizes(const Board& board) {
    for (int row = 0; row < board.board_size.y; ++row) {
        printf("%2d ", board.board_size.y - row);
        for (int col = 0; col < board.board_size.x; ++col) {
            Vec2 root = board.find({col + board.padding, row + board.padding});
            size_t size = board.group[root.x][root.y].size();
            printf("%2zu ", size);
        }
        printf("\n");
    }
}

void BoardPrinter::print_liberties(const Board& board) {
    for (int row = 0; row < board.board_size.y; ++row) {
        printf("%2d ", board.board_size.y - row);
        for (int col = 0; col < board.board_size.x; ++col) {
            Vec2 root = board.find({col + board.padding, row + board.padding});
            size_t libs = board.liberties[root.x][root.y].size();
            printf("%2zu ", libs);
        }
        printf("\n");
    }
}

bool BoardPrinter::is_on_board(const Board& board, int row, int col) {
    return row >= board.padding && row < board.padding + board.board_size.y &&
           col >= board.padding && col < board.padding + board.board_size.x;
}

}  // namespace go_data_gen
