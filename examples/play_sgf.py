import sys
import go_data_gen


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <sgf_file_path>")
        return 1

    file_path = sys.argv[1]

    board, moves, result = go_data_gen.load_sgf(file_path)

    for i, move in enumerate(moves, start=1):
        board.play(move)
        print(f"Move no. {i}:")
        board.print()

    print("Result: ", end="")
    if result < 0:
        print(f"B+{-result:.1f}")
    elif result > 0:
        print(f"W+{result:.1f}")
    else:
        print("0")

    return 0


if __name__ == "__main__":
    sys.exit(main())
