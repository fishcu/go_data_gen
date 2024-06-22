#include "go_data_gen/python_env.hpp"

namespace go_data_gen {

PythonEnvironment& PythonEnvironment::instance() {
    static PythonEnvironment instance;
    return instance;
}

PythonEnvironment::PythonEnvironment() { Py_Initialize(); }

PythonEnvironment::~PythonEnvironment() { Py_Finalize(); }

}  // namespace go_data_gen
