#include <pybind11/pybind11.h>

int add(int i, int j) {
  return i + j;
}

PYBIND11_MODULE(windpyapi, module) {
    module.doc() = "";
    module.def("add", &add, "A function that adds two numbers");
}
