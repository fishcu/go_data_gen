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
        # if move.color in [go_data_gen.Color.Black, go_data_gen.Color.White]:
        #     opposite_color = go_data_gen.Color.Black if move.color == go_data_gen.Color.White else go_data_gen.Color.White
        #     print(f"Illegal moves for {'Black' if opposite_color == go_data_gen.Color.Black else 'White'}:")
        #     board.print(go_data_gen.PrintMode.IllegalMovesBlack if opposite_color == go_data_gen.Color.Black else go_data_gen.PrintMode.IllegalMovesWhite)
        #     print("Liberty size:")
        #     board.print(go_data_gen.PrintMode.Liberties)

        # Check after 75 moves
        if i == 75:
            nn_input_data = board.get_nn_input_data(go_data_gen.opposite(move.color))
            stacked_maps, features = nn_input_data
            print("NN Input Data after 75 moves:")
            print("Stacked Maps:")

            # Split the stacked maps into individual 2D maps and print them
            for idx, map in enumerate(stacked_maps):
                print(f"Map {idx + 1}:")
                for row in map:
                    print(" ".join(f"{int(value.item()):2d}" for value in row))
                print()

            print("Features:")
            print(features)
            print()

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
