import sys
import go_data_gen


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <sgf_file_path>")
        return 1

    file_path = sys.argv[1]
    is_valid, board, moves, result = go_data_gen.load_sgf(file_path)

    if not is_valid:
        print(f"Error: The SGF file {file_path} is not suitable for training.")
        return 1

    print("Board after setup:")
    board.print()

    for i, move in enumerate(moves, start=1):
        print(
            f"Playing move no. {i}, which is: {'Black' if move.color == go_data_gen.Color.Black else 'White'} ", end="")
        if move.is_pass:
            print("pass")
        else:
            print(f"({move.coord.x}, {move.coord.y})")

        board.play(move)
        board.print()

        # Check after specific moves
        if i >= 180 and i <= 183 or i == 224:
            print(f"NN Input Data after {i} moves:")
            feature_planes = board.get_feature_planes(
                go_data_gen.opposite(move.color))
            assert len(feature_planes[0][0]) == board.num_feature_planes

            for c in range(board.num_feature_planes):
                print(f"Feature plane {c}:")
                board.print(lambda x, y: bool(feature_planes[y][x][c]))

            board.print_group_sizes()
            board.print_liberties()

            feature_vector = board.get_feature_scalars(
                go_data_gen.opposite(move.color))
            assert len(feature_vector) == board.num_feature_scalars
            for c in range(board.num_feature_scalars):
                print(f"Scalar feature {c} = {feature_vector[c]}")

    print("Result: ", end="")
    if result > 0:
        print(f"B+{result:.1f}")
    elif result < 0:
        print(f"W+{-result:.1f}")
    else:
        print("Draw or void game")

    return 0


if __name__ == "__main__":
    sys.exit(main())
