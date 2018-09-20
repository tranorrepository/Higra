/***************************************************************************
* Copyright ESIEE Paris (2018)                                             *
*                                                                          *
* Contributor(s) : Benjamin Perret                                         *
*                                                                          *
* Distributed under the terms of the CECILL-B License.                     *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#pragma once

#include "xtensor/xsort.hpp"

#include "../graph.hpp"
#include "../structure/details/light_axis_view.hpp"
#include "../accumulator/accumulator.hpp"

namespace hg {

    /**
     * Result of the region adjacency graph (rag) construction algorithm
     */
    struct region_adjacency_graph {
        /**
         * The region adjacency graph
         */
        ugraph rag;

        /**
         * An array indicating for each vertex of the original graph, the corresponding vertex of the rag
         */
        array_1d <index_t> vertex_map;

        /**
        * An array indicating for each edge of the original graph, the corresponding edge of the rag.
         * An edge with no corresponding edge in the rag (edge within a region) is indicated with the value invalid_index.
        */
        array_1d <index_t> edge_map;
    };

    /**
     * Construct a region adjacency graph from a vertex labeled graph in linear time.
     * @tparam graph_t
     * @tparam T
     * @param graph
     * @param xvertex_labels
     * @return see struct region_adjacency_graph
     */
    template<typename graph_t, typename T>
    auto
    make_region_adjacency_graph(const graph_t &graph, const xt::xexpression<T> &xvertex_labels) {
        HG_TRACE();
        static_assert(std::is_integral<typename T::value_type>::value, "Labels must have an integral type.");
        auto &vertex_labels = xvertex_labels.derived_cast();
        hg_assert(vertex_labels.dimension() == 1, "Vertex labels must be scalar numbers.");
        hg_assert(vertex_labels.size() == num_vertices(graph),
                  "Vertex labels size does not match graph number of vertices.");

        ugraph rag;

        array_1d <index_t> vertex_map({num_vertices(graph)}, invalid_index);
        array_1d <index_t> edge_map({num_edges(graph)}, invalid_index);

        index_t num_regions = 0;
        index_t num_edges = 0;

        std::vector<long> canonical_edge_indexes;

        stackv <index_t> s;

        auto explore_component =
                [&s, &graph, &vertex_labels, &rag, &vertex_map, &edge_map, &num_regions, &num_edges, &canonical_edge_indexes]
                        (index_t start_vertex) {

                    auto label_region = vertex_labels[start_vertex];
                    s.push(start_vertex);
                    vertex_map[start_vertex] = num_regions;
                    add_vertex(rag);
                    auto lowest_edge = num_edges;
                    while (!s.empty()) {
                        auto v = s.top();
                        s.pop();

                        for (auto ei: out_edge_index_iterator(v, graph)) {
                            auto e = edge(ei, graph);
                            auto adjv = other_vertex(e, v, graph);
                            if (vertex_labels[adjv] == label_region) {
                                if (vertex_map[adjv] == invalid_index) {
                                    vertex_map[adjv] = num_regions;
                                    s.push(adjv);
                                    canonical_edge_indexes.push_back(-1);
                                }
                            } else {
                                if (vertex_map[adjv] != invalid_index) {
                                    auto num_region_adjacent = vertex_map[adjv];
                                    if (canonical_edge_indexes[num_region_adjacent] < lowest_edge) {
                                        add_edge(num_region_adjacent, num_regions, rag);
                                        edge_map[ei] = num_edges;
                                        canonical_edge_indexes[num_region_adjacent] = num_edges;
                                        num_edges++;
                                    } else {
                                        edge_map[ei] = canonical_edge_indexes[num_region_adjacent];
                                    }
                                }
                            }
                        }
                    }
                    num_regions++;
                };


        for (auto v: vertex_iterator(graph)) {
            if (vertex_map[v] != invalid_index)
                continue;
            explore_component(v);

        }

        return region_adjacency_graph{std::move(rag), std::move(vertex_map), std::move(edge_map)};
    }

    namespace rag_internal {
        template<bool vectorial, typename T>
        auto
        rag_back_project_weights(const array_1d <index_t> &rag_map, const xt::xexpression<T> &xrag_weights) {
            HG_TRACE();
            auto &rag_weights = xrag_weights.derived_cast();

            index_t numv = rag_map.size();
            std::vector<size_t> shape;
            shape.push_back(numv);
            shape.insert(shape.end(), rag_weights.shape().begin() + 1, rag_weights.shape().end());
            array_nd<typename T::value_type> weights = xt::zeros<typename T::value_type>(shape);


            auto input_view = make_light_axis_view<vectorial>(rag_weights);
            auto output_view = make_light_axis_view<vectorial>(weights);

            for (index_t i = 0; i < numv; ++i) {
                if (rag_map.data()[i] != invalid_index) {
                    output_view.set_position(i);
                    input_view.set_position(rag_map.data()[i]);
                    output_view = input_view;
                }
            }

            return weights;
        }

        template<bool vectorial,
                typename T,
                typename accumulator_t,
                typename output_t = typename T::value_type>
        auto
        rag_accumulate(const array_1d <index_t> &rag_map,
                       const xt::xexpression<T> &xweights,
                       const accumulator_t &accumulator) {
            HG_TRACE();
            auto &weights = xweights.derived_cast();
            hg_assert(weights.shape()[0] == rag_map.size(), "Weights dimension does not match rag map dimension.");

            index_t size = xt::amax(rag_map)() + 1;
            auto data_shape = std::vector<size_t>(weights.shape().begin() + 1, weights.shape().end());
            auto output_shape = accumulator_t::get_output_shape(data_shape);
            output_shape.insert(output_shape.begin(), size);
            array_nd<typename T::value_type> rag_weights = array_nd<typename T::value_type>::from_shape(output_shape);

            auto input_view = make_light_axis_view<vectorial>(weights);
            auto output_view = make_light_axis_view<vectorial>(rag_weights);

            std::vector<decltype(accumulator.template make_accumulator<vectorial>(output_view))> accs;
            accs.reserve(size);


            for (index_t i = 0; i < size; ++i) {
                output_view.set_position(i);
                accs.push_back(accumulator.template make_accumulator<vectorial>(output_view));
                accs[i].initialize();
            }

            index_t map_size = rag_map.size();
            for (index_t i = 0; i < map_size; ++i) {
                if (rag_map.data()[i] != invalid_index) {
                    input_view.set_position(i);
                    accs[rag_map.data()[i]].accumulate(input_view.begin());
                }
            }

            for (auto &acc: accs) {
                acc.finalize();
            }

            return rag_weights;
        }
    }

    /**
     * Projects weights on the rag (vertices or edges) onto the original graph.
     * @tparam T
     * @param rag_map rag vertex_map or rag edge_map (see struct region_adjacency_graph)
     * @param xrag_weights node or edge weights of the rag (depending of the provided rag_map)
     * @return original graph (vertices or edges) weights
     */
    template<typename T>
    auto
    rag_back_project_weights(const array_1d <index_t> &rag_map, const xt::xexpression<T> &xrag_weights) {
        if (xrag_weights.derived_cast().dimension() == 1) {
            return rag_internal::rag_back_project_weights<false>(rag_map, xrag_weights);
        } else {
            return rag_internal::rag_back_project_weights<true>(rag_map, xrag_weights);
        }

    }

    /**
     * Accumulate original graph (vertices or edges) weights onto the rag (vertices or edges)
     * @tparam T
     * @tparam accumulator_t
     * @tparam output_t
     * @param rag_map rag vertex_map or rag edge_map (see struct region_adjacency_graph)
     * @param xweights node or edge weights of the original graph (depending of the provided rag_map)
     * @param accumulator
     * @return rag (vertices or edges) weights
     */
    template<typename T, typename accumulator_t, typename output_t = typename T::value_type>
    auto rag_accumulate(const array_1d <index_t> &rag_map,
                        const xt::xexpression<T> &xweights,
                        const accumulator_t &accumulator) {
        if (xweights.derived_cast().dimension() == 1) {
            return rag_internal::rag_accumulate<false, T, accumulator_t, output_t>(rag_map, xweights, accumulator);
        } else {
            return rag_internal::rag_accumulate<true, T, accumulator_t, output_t>(rag_map, xweights, accumulator);
        }
    };

    /**
     * Given two labelisations, a fine and a coarse one, of a same set of elements.
     * Find for each label (ie. region) of the fine labelisation, the label of the region in the
     * coarse labelisation that maximises the intersection with the "fine" region.
     *
     * Pre-condition:
     *  range(xlabelisation_fine) = [0..num_regions_fine[
     *  range(xlabelisation_coarse) = [0..num_regions_coarse[
     *
     * @tparam T1
     * @tparam T2
     * @param xlabelisation_fine
     * @param num_regions_fine
     * @param xlabelisation_coarse
     * @param num_regions_coarse
     * @return a 1d array of size num_regions_fine
     */
    template<typename T1, typename T2>
    auto project_fine_to_coarse_labelisation
            (const xt::xexpression<T1> &xlabelisation_fine,
             size_t num_regions_fine,
             const xt::xexpression<T2> &xlabelisation_coarse,
             size_t num_regions_coarse) {

        auto &labelisation_fine = xlabelisation_fine.derived_cast();
        auto &labelisation_coarse = xlabelisation_coarse.derived_cast();

        static_assert(std::is_integral<typename T1::value_type>::value,
                      "Labelisation must have integral value type.");
        static_assert(std::is_integral<typename T2::value_type>::value,
                      "Labelisation must have integral value type.");
        hg_assert(labelisation_fine.dimension() == 1 && labelisation_coarse.dimension() == 1,
                  "Labelisation must be a 1d array.");
        hg_assert(labelisation_fine.size() == labelisation_coarse.size(),
                  "Labelisations must have the same size.");

        array_2d <size_t> intersections = xt::zeros<size_t>({num_regions_fine, num_regions_coarse});

        for (index_t i = 0; i < labelisation_fine.size(); i++) {
            intersections(labelisation_fine(i), labelisation_coarse(i))++;
        }

        return xt::eval(xt::argmax(intersections, 1));
    }

    /**
     * Given two region adjacency graphs, a fine and a coarse one, of a same set of elements.
     * Find for each region of the fine rag, the region of the
     * coarse rag that maximises the intersection with the "fine" region.
     *
     * @param fine_rag
     * @param coarse_rag
     * @return a 1d array of size num_vertices(fine_rag.rag)
     */
    inline
    auto project_fine_to_coarse_rag
            (const region_adjacency_graph &fine_rag,
             const region_adjacency_graph &coarse_rag) {
        return project_fine_to_coarse_labelisation(fine_rag.vertex_map,
                                                   num_vertices(fine_rag.rag),
                                                   coarse_rag.vertex_map,
                                                   num_vertices(coarse_rag.rag));
    }
}
