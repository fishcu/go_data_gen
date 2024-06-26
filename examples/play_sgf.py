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
        print()
        print(f"Illegal moves for {'Black' if go_data_gen.opposite(
            move.color) == go_data_gen.Color.Black else 'White'}:")
        board.print(go_data_gen.PrintMode.IllegalMovesBlack if go_data_gen.opposite(move.color) == go_data_gen.Color.Black
                    else go_data_gen.PrintMode.IllegalMovesWhite)

        print("Liberty size:")
        board.print(go_data_gen.PrintMode.Liberties)

        # Check after 75 moves
        if i == 75:
            nn_input_data = board.get_nn_input_data(
                go_data_gen.opposite(move.color))
            stacked_maps, features = nn_input_data
            print("NN Input Data after 75 moves:")
            print("Stacked Maps:")
            for idx in range(stacked_maps.shape[0]):
                print(f"Map {idx + 1}:")
                for row in range(stacked_maps.shape[1]):
                    for col in range(stacked_maps.shape[2]):
                        print(
                            f"{int(stacked_maps[idx, row, col]):2d}", end=" ")
                    print()
                print()

            print("Features:")
            for feature in features:
                print(feature, end=" ")
            print("\n")

        print("Feature plane 15:")
        board.print_feature_planes(go_data_gen.opposite(move.color), 15)

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
