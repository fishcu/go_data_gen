import sys
import go_data_gen


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <sgf_file_path>")
        return 1

    file_path = sys.argv[1]
    # Assuming a 19x19 board and a komi of 6.5
    board = go_data_gen.Board(go_data_gen.Vec2(19, 19), 6.5)
    moves = []
    result = []

    go_data_gen.load_sgf(file_path, board, moves, result)

    for i, move in enumerate(moves, start=1):
        board.play(move)
        print(f"Move no. {i}:")
        board.print(go_data_gen.PrintMode.Default)
        print()
        # if move.color in [go_data_gen.Color.Black, go_data_gen.Color.White]:
        #     opposite_color = go_data_gen.Color.Black if move.color == go_data_gen.Color.White else go_data_gen.Color.White
        #     print(f"Illegal moves for {'Black' if opposite_color == go_data_gen.Color.Black else 'White'}:")
        #     board.print(go_data_gen.PrintMode.IllegalMovesBlack if opposite_color == go_data_gen.Color.Black else go_data_gen.PrintMode.IllegalMovesWhite)
        #     print("Liberty size:")
        #     board.print(go_data_gen.PrintMode.Liberties)

    print("Result: ", end="")
    if result[0] < 0:
        print(f"B+{-result[0]:.1f}")
    elif result[0] > 0:
        print(f"W+{result[0]:.1f}")
    else:
        print("0")

    return 0


if __name__ == "__main__":
    sys.exit(main())