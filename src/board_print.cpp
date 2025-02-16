#include "go_data_gen/board.hpp"

namespace go_data_gen {

void Board::print(std::function<bool(int x, int y)> highlight_fn) {
    // ANSI color codes
    const char* const BOARD_BG = "\033[48;5;136m";      // Brown background
    const char* const HIGHLIGHT_BG = "\033[48;5;179m";  // Light brown background
    const char* const CYAN_BG = "\033[48;5;51m";        // Cyan background
    const char* const BLACK_FG = "\033[38;5;0m";        // Black foreground
    const char* const WHITE_FG = "\033[38;5;15m";       // White foreground
    const char* const RESET = "\033[0m";                // Reset all colors
    const char* const OFFBOARD_BG = "\033[48;5;0m";     // Black background for off-board

    // Get last move coordinates if available
    bool has_last_move = false;
    Vec2 last_coord{-1, -1};
    if (!history.empty() && !history.back().is_pass) {
        has_last_move = true;
        last_coord = history.back().coord;
    }

    // Print column coordinates
    printf("     ");
    for (int mem_x = padding; mem_x < board_size.x + padding; ++mem_x) {
        const int x = mem_x - padding;
        printf("%c ", static_cast<char>(x < 8 ? 'A' + x : 'B' + x));
    }
    printf("\n");

    for (int mem_y = 0; mem_y <= board_size.y + padding; ++mem_y) {
        const int y = mem_y - padding;
        if (y >= 0 && y < board_size.y) {
            printf("%2d ", board_size.y - y);
        } else {
            printf("   ");
        }

        for (int mem_x = 0; mem_x <= board_size.x + padding; ++mem_x) {
            const int x = mem_x - padding;
            const bool is_last_move = has_last_move && last_coord == Vec2{x, y};
            const bool is_highlighted = highlight_fn(mem_x, mem_y);

            // Select background color
            if (is_highlighted) {
                printf("%s", CYAN_BG);
            } else if (board[mem_y][mem_x] == Color::OffBoard) {
                printf("%s", OFFBOARD_BG);
            } else {
                printf("%s", is_last_move ? HIGHLIGHT_BG : BOARD_BG);
            }

            switch (board[mem_y][mem_x]) {
            case Color::Empty: {
                printf("%s", BLACK_FG);
                if (x == 0 && y == 0)
                    printf("┌─");
                else if (x == board_size.x - 1 && y == 0)
                    printf("┐ ");
                else if (x == 0 && y == board_size.y - 1)
                    printf("└─");
                else if (x == board_size.x - 1 && y == board_size.y - 1)
                    printf("┘ ");
                else if (x == 0)
                    printf("├─");
                else if (x == board_size.x - 1)
                    printf("┤ ");
                else if (y == 0)
                    printf("┬─");
                else if (y == board_size.y - 1)
                    printf("┴─");
                else
                    printf("┼─");
                break;
            }
            case Color::Black:
                printf("%s%s ", BLACK_FG, is_last_move ? "◉" : "●");
                break;
            case Color::White:
                printf("%s%s ", WHITE_FG, is_last_move ? "◉" : "●");
                break;
            case Color::OffBoard:
                printf("  ");
                break;
            }
            printf("%s", RESET);
        }
        printf("\n");
    }
}

void Board::print_group_sizes() {
    for (int y = 0; y < board_size.y; ++y) {
        for (int x = 0; x < board_size.x; ++x) {
            const auto root = find({x + padding, y + padding});
            const int size = group[root.y][root.x].size();
            printf("%2d ", size);
        }
        printf("\n");
    }
    printf("\n");
}

void Board::print_liberties() {
    for (int y = 0; y < board_size.y; ++y) {
        for (int x = 0; x < board_size.x; ++x) {
            const auto root = find({x + padding, y + padding});
            const int libs = liberties[root.y][root.x].size();
            printf("%2d ", libs);
        }
        printf("\n");
    }
    printf("\n");
}

}  // namespace go_data_gen
