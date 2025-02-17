import os
import sys
import subprocess
import platform
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        super().__init__(name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def build_extension(self, ext):
        # Determine the output directory for the built module.
        extdir = os.path.abspath(os.path.dirname(
            self.get_ext_fullpath(ext.name)))

        # Choose build type: use debug if DEBUG_BUILD=1 in the environment.
        build_type = "Debug" if os.environ.get(
            "DEBUG_BUILD") == "1" else "Release"

        # Set up the CMake arguments.
        cmake_args = [
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}"
        ]
        build_args = ["--config", build_type]

        if platform.system() == "Windows":
            cmake_args += ["-A", "x64" if sys.maxsize > 2**32 else "Win32"]
            build_args += ["--", "/m"]
        else:
            build_args += ["--", "-j2"]

        os.makedirs(self.build_temp, exist_ok=True)
        subprocess.check_call(["cmake", ext.sourcedir] +
                              cmake_args, cwd=self.build_temp)
        subprocess.check_call(["cmake", "--build", "."] +
                              build_args, cwd=self.build_temp)


setup(
    name="go_data_gen",
    version="0.2.0",
    description="A C++ library with pybind11 bindings for Computer Go input generation",
    ext_modules=[CMakeExtension("go_data_gen")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
)
