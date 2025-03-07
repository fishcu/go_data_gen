#define PYBIND11_DETAILED_ERROR_MESSAGES
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "go_data_gen/board.hpp"
#include "go_data_gen/sgf.hpp"
#include "go_data_gen/types.hpp"

namespace py = pybind11;

using namespace go_data_gen;

PYBIND11_MODULE(go_data_gen, m) {
    m.doc() = "Python bindings for go_data_gen C++ library";

#ifndef NDEBUG
    py::print(
        "WARNING: go_data_gen has been compiled in debug mode. "
        "Extended runtime checks and decreased performance.",
        py::arg("file") = py::module_::import("sys").attr("stderr"));
#endif

    py::enum_<Color>(m, "Color")
        .value("Empty", Empty)
        .value("Black", Black)
        .value("White", White)
        .value("OffBoard", OffBoard);

    py::class_<Vec2>(m, "Vec2")
        .def(py::init<>())
        .def(py::init<int, int>(), py::arg("x"), py::arg("y"))
        .def_readwrite("x", &Vec2::x)
        .def_readwrite("y", &Vec2::y)
        .def("__eq__", &Vec2::operator==)
        .def("__ne__", &Vec2::operator!=)
        .def("__lt__", &Vec2::operator<);

    m.def("opposite", &opposite, "Get the opposite color", py::arg("color"));

    py::class_<Move>(m, "Move")
        .def(py::init<Color, bool, Vec2>())
        .def_readwrite("color", &Move::color)
        .def_readwrite("is_pass", &Move::is_pass)
        .def_readwrite("coord", &Move::coord);

    py::enum_<MoveLegality>(m, "MoveLegality")
        .value("Legal", MoveLegality::Legal)
        .value("NonEmpty", MoveLegality::NonEmpty)
        .value("Suicidal", MoveLegality::Suicidal)
        .value("Ko", MoveLegality::Ko);

    py::class_<Board>(m, "Board")
        .def_readonly_static("max_board_size", &Board::max_board_size)
        .def_readonly_static("padding", &Board::padding)
        .def_readonly_static("data_size", &Board::data_size)
        .def(py::init<>())
        .def(py::init<Vec2, float>())
        .def("get_board_size", &Board::get_board_size)
        .def_readwrite("komi", &Board::komi)
        .def("reset", &Board::reset)
        .def("setup_move", &Board::setup_move)
        .def("get_move_legality", &Board::get_move_legality)
        .def("is_legal", &Board::is_legal)
        .def("play", &Board::play)
        .def_readonly_static("num_feature_planes", &Board::num_feature_planes)
        .def("get_feature_planes",
             [](Board& self, Color to_play) {
                 auto feature_planes = self.get_feature_planes(to_play);
                 auto features_array = py::array_t<float>(
                     {Board::data_size, Board::data_size, Board::num_feature_planes});
                 auto features_mu = features_array.mutable_unchecked<3>();
                 for (size_t y = 0; y < Board::data_size; ++y) {
                     for (size_t x = 0; x < Board::data_size; ++x) {
                         for (size_t c = 0; c < Board::num_feature_planes; ++c) {
                             features_mu(y, x, c) = feature_planes[y][x][c];
                         }
                     }
                 }
                 return features_array;
             })
        .def_readonly_static("num_feature_scalars", &Board::num_feature_scalars)
        .def("get_feature_scalars",
             [](Board& self, Color to_play) {
                 auto scalar_features = self.get_feature_scalars(to_play);
                 auto scalars_array = py::array_t<float>({Board::num_feature_scalars});
                 auto scalars_mu = scalars_array.mutable_unchecked<1>();
                 std::memcpy(scalars_mu.mutable_data(0), scalar_features.data(),
                             Board::num_feature_scalars * sizeof(float));
                 return scalars_array;
             })
        .def("print", &Board::print,
             py::arg("highlight_fn") = py::cpp_function([](int, int) { return false; }))
        .def("print_group_sizes", &Board::print_group_sizes)
        .def("print_liberties", &Board::print_liberties);

    m.def(
        "load_sgf",
        [](const std::string& file_path) {
            Board board;
            std::vector<Move> moves;
            float result;
            bool is_valid = load_sgf(file_path, board, moves, result);

            if (!is_valid) {
                // Return (False, None, None, None) for invalid games
                return py::make_tuple(false, py::none(), py::none(), py::none());
            } else {
                // Return (True, board, moves, result) for valid games
                return py::make_tuple(true, board, moves, result);
            }
        },
        "Load SGF file and return (is_valid, board, moves, result). "
        "If the game is in encore phase, is_valid will be False and the other values will be None.",
        py::arg("file_path"));
}
