#include <pybind11/pybind11.h>
#include <torch/torch.h>

#include "go_data_gen/board.h"
#include "go_data_gen/sgf.h"

namespace py = pybind11;

// PYBIND11_MODULE(go_data_gen_pybind, m) {
//     m.doc() = "go_data_gen python bindings";
    
//     py::class_<go_data_gen::Board>(m, "Board")
//         .def(py::init<>())
//         .def("legal_moves", &go_data_gen::Board::LegalMoves)
//         .def("liberties", &go_data_gen::Board::Liberties);
        
//     m.def("load_sgf", &go_data_gen::LoadSgf, "Load SGF file");
// }
