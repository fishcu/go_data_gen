#define PYBIND11_DETAILED_ERROR_MESSAGES
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "go_data_gen/board.hpp"
#include "go_data_gen/sgf.hpp"
#include "go_data_gen/types.hpp"

namespace py = pybind11;

namespace pybind11 {
namespace detail {
template <>
struct type_caster<go_data_gen::Vec2> {
public:
    PYBIND11_TYPE_CASTER(go_data_gen::Vec2, _("Vec2"));

    // Conversion part 1 (Python -> C++)
    bool load(handle src, bool) {
        if (!py::isinstance<py::tuple>(src))
            return false;
        auto tuple = src.cast<py::tuple>();
        if (tuple.size() != 2)
            return false;
        value.x = tuple[0].cast<int>();
        value.y = tuple[1].cast<int>();
        return true;
    }

    // Conversion part 2 (C++ -> Python)
    static handle cast(go_data_gen::Vec2 src, return_value_policy, handle) {
        return py::make_tuple(src.x, src.y).release();
    }
};
}  // namespace detail
}  // namespace pybind11

PYBIND11_MODULE(go_data_gen, m) {
    m.doc() = "Python bindings for go_data_gen C++ library";

    // Bind Color enum
    py::enum_<go_data_gen::Color>(m, "Color")
        .value("Empty", go_data_gen::Empty)
        .value("Black", go_data_gen::Black)
        .value("White", go_data_gen::White)
        .value("OffBoard", go_data_gen::OffBoard);

    // Bind the opposite function
    m.def("opposite", &go_data_gen::opposite, "Get the opposite color", py::arg("color"));

    // Bind Vec2 struct as a tuple
    py::class_<go_data_gen::Vec2>(m, "Vec2")
        .def(py::init<int, int>())
        .def_readwrite("x", &go_data_gen::Vec2::x)
        .def_readwrite("y", &go_data_gen::Vec2::y);

    // Bind Move struct
    py::class_<go_data_gen::Move>(m, "Move")
        .def(py::init<go_data_gen::Color, go_data_gen::Vec2>())
        .def_readwrite("color", &go_data_gen::Move::color)
        .def_readwrite("coord", &go_data_gen::Move::coord);

    // Bind PrintMode enum
    py::enum_<go_data_gen::Board::PrintMode>(m, "PrintMode")
        .value("Default", go_data_gen::Board::Default)
        .value("GroupSize", go_data_gen::Board::GroupSize)
        .value("Liberties", go_data_gen::Board::Liberties)
        .value("IllegalMovesBlack", go_data_gen::Board::IllegalMovesBlack)
        .value("IllegalMovesWhite", go_data_gen::Board::IllegalMovesWhite);

    py::class_<go_data_gen::Board>(m, "Board")
        .def_readonly_static("max_board_size", &go_data_gen::Board::max_board_size)
        .def_readonly_static("padding", &go_data_gen::Board::padding)
        .def_readonly_static("data_size", &go_data_gen::Board::data_size)
        .def(py::init<>())
        .def(py::init<go_data_gen::Vec2, float>())
        .def("reset", &go_data_gen::Board::reset)
        .def("is_legal", &go_data_gen::Board::is_legal)
        .def("play", &go_data_gen::Board::play)
        .def("get_mask", &go_data_gen::Board::get_mask)
        .def("get_legal_map", &go_data_gen::Board::get_legal_map)
        .def("get_stone_map", &go_data_gen::Board::get_stone_map)
        .def("get_history_map", &go_data_gen::Board::get_history_map)
        .def("get_liberty_map", &go_data_gen::Board::get_liberty_map, py::arg("color"),
             py::arg("num"), py::arg("or_greater") = false)
        .def("get_komi_from_player_perspective",
             &go_data_gen::Board::get_komi_from_player_perspective)
        .def_readonly_static("num_feature_planes", &go_data_gen::Board::num_feature_planes)
        .def_readonly_static("num_feature_scalars", &go_data_gen::Board::num_feature_scalars)
        .def("get_nn_input_data", &go_data_gen::Board::get_nn_input_data)
        .def("print", &go_data_gen::Board::print, py::arg("mode") = go_data_gen::Board::Default)
        .def("print_feature_planes", &go_data_gen::Board::print_feature_planes, py::arg("to_play"),
             py::arg("feature_plane_index") = 0);

    // Bind free function to return a tuple
    m.def(
        "load_sgf",
        [](const std::string& file_path) {
            go_data_gen::Board board;
            std::vector<go_data_gen::Move> moves;
            float result;
            go_data_gen::load_sgf(file_path, board, moves, result);
            return std::make_tuple(board, moves, result);
        },
        "Load SGF file and return board, moves, and result", py::arg("file_path"));
}
