#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "go_data_gen/board.hpp"
#include "go_data_gen/sgf.hpp"
#include "go_data_gen/types.hpp"

namespace py = pybind11;

PYBIND11_MODULE(go_data_gen, m) {
    m.doc() = "Python bindings for go_data_gen C++ library";

    // Bind free function
    m.def("load_sgf", &go_data_gen::load_sgf, "Load SGF file into board and move history",
          py::arg("file_path"), py::arg("board"), py::arg("moves"), py::arg("result"));

    // Bind Color enum
    py::enum_<go_data_gen::Color>(m, "Color")
        .value("Empty", go_data_gen::Empty)
        .value("Black", go_data_gen::Black)
        .value("White", go_data_gen::White)
        .value("OffBoard", go_data_gen::OffBoard);

    // Bind Vec2 struct
    py::class_<go_data_gen::Vec2>(m, "Vec2")
        .def(py::init<int, int>())
        .def_readwrite("x", &go_data_gen::Vec2::x)
        .def_readwrite("y", &go_data_gen::Vec2::y);

    // Bind Move struct
    py::class_<go_data_gen::Move>(m, "Move")
        .def(py::init<go_data_gen::Color, go_data_gen::Vec2>())
        .def_readwrite("color", &go_data_gen::Move::color)
        .def_readwrite("coord", &go_data_gen::Move::coord);

    // Bind Board class
    py::class_<go_data_gen::Board>(m, "Board")
        .def(py::init<go_data_gen::Vec2, float>())
        .def("reset", &go_data_gen::Board::reset)
        .def("is_legal", &go_data_gen::Board::is_legal)
        .def("play", &go_data_gen::Board::play)
        .def("print", &go_data_gen::Board::print, py::arg("mode") = go_data_gen::Board::Default);
}
