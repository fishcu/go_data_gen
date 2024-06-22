#pragma once

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace go_data_gen {

class PythonEnvironment {
public:
    static PythonEnvironment& instance();

    PythonEnvironment(const PythonEnvironment&) = delete;
    PythonEnvironment& operator=(const PythonEnvironment&) = delete;

private:
    PythonEnvironment();
    ~PythonEnvironment();
};

}  // namespace go_data_gen
