//
// Created by user on 3/15/18.
//

#include "pybind11/pybind11.h"
#include "cpp/py_undirected_graph.hpp"
#include "cpp/py_embedding.hpp"
#include "cpp/py_regular_graph.hpp"
#include "cpp/py_tree_graph.hpp"
#include "cpp/py_graph_weights.hpp"
#include "cpp/py_graph_image.hpp"
#include "cpp/py_lca_fast.hpp"
#include "cpp/py_hierarchy_core.hpp"
#include "cpp/py_pink_io.hpp"
#include "cpp/py_accumulators.hpp"
#include "cpp/py_algo_tree.hpp"

#define FORCE_IMPORT_ARRAY

#include "xtensor-python/pyarray.hpp"
#include "xtensor-python/pytensor.hpp"

PYBIND11_MODULE(higram, m
) {
m.

doc() = R"pbdoc(
        Higra: Hierarchical Graph Analysis
        -----------------------
        .. currentmodule:: higram
        .. autosummary::
           :toctree: _generate
    )pbdoc";


#ifdef VERSION_INFO
m.attr("__version__") = VERSION_INFO;
#else
m.attr("__version__") = "dev";
#endif
    xt::import_numpy();
    py_init_undirected_graph(m);
    py_init_embedding(m);
    py_init_regular_graph(m);
    py_init_tree_graph(m);
    py_init_graph_image(m);
    py_init_graph_weights(m);
    py_init_lca_fast(m);
    py_init_hierarchy_core(m);
    py_init_pink_io(m);
    py_init_accumulators(m);
    py_init_algo_tree(m);
}