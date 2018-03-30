//
// Created by user on 3/12/18.
//


#pragma once

#include <functional>
#include <boost/graph/graph_concepts.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <vector>
#include <utility>

#include "xtensor/xarray.hpp"
#include "embedding.hpp"

namespace hg {


    namespace regular_graph_internal {

        using point_list_t = std::vector<xt::xarray<long>>;
        using point_list_iterator_t = point_list_t::iterator;

        //forward declaration
        template<typename embedding_t>
        struct regular_graph_adjacent_vertex_iterator;

        struct regular_graph_traversal_category :
                virtual public boost::incidence_graph_tag,
                virtual public boost::bidirectional_graph_tag,
                virtual public boost::adjacency_graph_tag,
                virtual public boost::vertex_list_graph_tag
        {
        };


        template<typename embedding_t>
        class regular_graph {


        public:
            // Graph associated types
            using vertex_descriptor = std::size_t;
            using directed_category = boost::undirected_tag;
            using edge_parallel_category = boost::disallow_parallel_edge_tag;
            using traversal_category = regular_graph_traversal_category;

            // VertexListGraph associated types
            using vertex_iterator = boost::counting_iterator<vertex_descriptor>;
            using vertices_size_type = std::size_t;

            //AdjacencyGraph associated types
            using adjacency_iterator = regular_graph_adjacent_vertex_iterator<embedding_t>;

            // IncidenceGraph associated types
            using edge_descriptor = std::pair<vertex_descriptor, vertex_descriptor>;
            using iterator_transform_function = std::function<edge_descriptor(vertex_descriptor)>;

            using out_edge_iterator = boost::transform_iterator<iterator_transform_function, adjacency_iterator>;
            using degree_size_type = std::size_t;

            //BidirectionalGraph associated types
            using in_edge_iterator = out_edge_iterator;


            vertices_size_type num_vertices() const {
                return embedding.size();
            }

            embedding_t embedding;
            point_list_t neighbours;

            regular_graph(embedding_t _embedding = {}, point_list_t _neighbours = {})
                    : embedding(_embedding), neighbours(_neighbours) {
            }


        };


        // Iterator
        template<typename embedding_t>
        struct regular_graph_adjacent_vertex_iterator :
                public boost::iterator_facade<regular_graph_adjacent_vertex_iterator<embedding_t>,
                        typename regular_graph<embedding_t>::vertex_descriptor,
                        boost::forward_traversal_tag,
                        typename regular_graph<embedding_t>::vertex_descriptor> {
        public:
            using graph_t = regular_graph<embedding_t>;
            using graph_vertex_t = typename graph_t::vertex_descriptor;


            regular_graph_adjacent_vertex_iterator() {}

            regular_graph_adjacent_vertex_iterator(graph_vertex_t _source,
                                                   embedding_t _embedding,
                                                   point_list_iterator_t _point_iterator,
                                                   point_list_iterator_t _point_iterator_end
            )
                    : source(_source), embedding(_embedding), point_iterator(_point_iterator),
                      point_iterator_end(_point_iterator_end) {

                source_coordinates = embedding.lin2grid(source);
                std::vector<long> neighbourc(source_coordinates.size());
                if (point_iterator != point_iterator_end) {
                    // TODO: optimize computation, avoid temporary
                    auto neighbour_relative = *point_iterator;
                    for (std::size_t i = 0; i < source_coordinates.size(); ++i) {
                        neighbourc[i] = neighbour_relative[i] + source_coordinates[i];
                    }

                    if (!embedding.contains(neighbourc)) {
                        increment();
                    } else {
                        neighbour = embedding.grid2lin(neighbourc);
                    }

                }
            }

        private:


            friend class boost::iterator_core_access;

            void increment() {
                bool flag;
                std::vector<long> neighbourc(source_coordinates.size());
                do {
                    point_iterator++;
                    if (point_iterator != point_iterator_end) {
                        // TODO: optimize computation, avoid temporary
                        auto neighbour_relative = *point_iterator;
                        for (std::size_t i = 0; i < source_coordinates.size(); ++i) {
                            neighbourc[i] = neighbour_relative[i] + source_coordinates[i];
                        }
                        flag = embedding.contains(neighbourc);
                    } else {
                        flag = true;
                    }
                } while (!flag);
                if (point_iterator != point_iterator_end) {
                    neighbour = embedding.grid2lin(neighbourc);
                }
            }

            bool equal(regular_graph_adjacent_vertex_iterator const &other) const {
                return this->point_iterator == other.point_iterator;
            }


            graph_vertex_t dereference() const {
                return neighbour;
            }

            graph_vertex_t source;
            graph_vertex_t neighbour;
            std::vector<long> source_coordinates;
            embedding_t embedding;
            point_list_iterator_t point_iterator;
            point_list_iterator_t point_iterator_end;

        };

    }

    template<typename embedding_t>
    using regular_graph = regular_graph_internal::regular_graph<embedding_t>;

    using regular_grid_graph = regular_graph<hg::embedding_grid>;

    template<typename embedding_t>
    using regular_graph_out_edge_iterator = typename regular_graph_internal::regular_graph<embedding_t>::out_edge_iterator;

    template<typename embedding_t>
    using regular_graph_adjacent_vertex_iterator = typename regular_graph_internal::regular_graph<embedding_t>::adjacency_iterator;


}


namespace boost {
    template<typename embedding_t>
    struct graph_traits<hg::regular_graph<embedding_t>> {
        using G = hg::regular_graph<embedding_t>;

        using vertex_descriptor = typename G::vertex_descriptor;
        using edge_descriptor = typename G::edge_descriptor;
        using out_edge_iterator = typename G::out_edge_iterator;

        using directed_category = typename G::directed_category;
        using edge_parallel_category = typename G::edge_parallel_category;
        using traversal_category = typename G::traversal_category;

        using degree_size_type = typename G::degree_size_type;

        using in_edge_iterator = typename G::in_edge_iterator;
        using vertex_iterator = typename G::vertex_iterator;
        using vertices_size_type = typename G::vertices_size_type;
        using adjacency_iterator = typename G::adjacency_iterator;
    };

    template<typename embedding_t>
    std::pair<typename hg::regular_graph<embedding_t>::out_edge_iterator, typename hg::regular_graph<embedding_t>::out_edge_iterator>
    out_edges(typename hg::regular_graph<embedding_t>::vertex_descriptor u, hg::regular_graph<embedding_t> &g) {
        return std::make_pair(
                hg::regular_graph_out_edge_iterator<embedding_t>(
                        hg::regular_graph_adjacent_vertex_iterator<embedding_t>(
                                u,
                                g.embedding,
                                g.neighbours.begin(),
                                g.neighbours.end()),
                        [u](typename hg::regular_graph<embedding_t>::vertex_descriptor v) {
                            return std::make_pair(u, v);
                        }),
                hg::regular_graph_out_edge_iterator<embedding_t>(
                        hg::regular_graph_adjacent_vertex_iterator<embedding_t>(
                                u,
                                g.embedding,
                                g.neighbours.end(),
                                g.neighbours.end()),
                        [u](typename hg::regular_graph<embedding_t>::vertex_descriptor v) {
                            return std::make_pair(u, v);
                        })
        );
    }

    template<typename embedding_t>
    std::pair<typename hg::regular_graph<embedding_t>::out_edge_iterator, typename hg::regular_graph<embedding_t>::out_edge_iterator>
    in_edges(typename hg::regular_graph<embedding_t>::vertex_descriptor u, hg::regular_graph<embedding_t> &g) {
        return std::make_pair(
                hg::regular_graph_out_edge_iterator<embedding_t>(
                        hg::regular_graph_adjacent_vertex_iterator<embedding_t>(
                                u,
                                g.embedding,
                                g.neighbours.begin(),
                                g.neighbours.end()),
                        [u](typename hg::regular_graph<embedding_t>::vertex_descriptor v) {
                            return std::make_pair(v, u);
                        }),
                hg::regular_graph_out_edge_iterator<embedding_t>(
                        hg::regular_graph_adjacent_vertex_iterator<embedding_t>(
                                u,
                                g.embedding,
                                g.neighbours.end(),
                                g.neighbours.end()),
                        [u](typename hg::regular_graph<embedding_t>::vertex_descriptor v) {
                            return std::make_pair(v, u);
                        })
        );
    }


    template<typename embedding_t>
    typename hg::regular_graph<embedding_t>::degree_size_type
    out_degree(
            typename hg::regular_graph<embedding_t>::vertex_descriptor &v,
            hg::regular_graph<embedding_t> &g) {
        typename hg::regular_graph<embedding_t>::degree_size_type count = 0;
        typename hg::regular_graph<embedding_t>::out_edge_iterator out_i, out_end;
        typename hg::regular_graph<embedding_t>::edge_descriptor e;
        for (boost::tie(out_i, out_end) = out_edges(v, g); out_i != out_end; ++out_i) {
            count++;
        }

        return count;
    }

    template<typename embedding_t>
    typename hg::regular_graph<embedding_t>::degree_size_type
    in_degree(
            typename hg::regular_graph<embedding_t>::vertex_descriptor &v,
            hg::regular_graph<embedding_t> &g) {
        return out_degree(v, g);
    }

    template<typename embedding_t>
    typename hg::regular_graph<embedding_t>::vertex_descriptor
    source(
            typename hg::regular_graph<embedding_t>::edge_descriptor &e,
            hg::regular_graph<embedding_t> &) {
        return e.first;
    }

    template<typename embedding_t>
    typename hg::regular_graph<embedding_t>::vertex_descriptor
    target(
            typename hg::regular_graph<embedding_t>::edge_descriptor &e,
            hg::regular_graph<embedding_t> &) {
        return e.second;
    }

    template<typename embedding_t>
    typename hg::regular_graph<embedding_t>::degree_size_type
    degree(
            typename hg::regular_graph<embedding_t>::vertex_descriptor &v,
            hg::regular_graph<embedding_t> &g) {
        return out_degree(v, g);
    }

    template<typename embedding_t>
    typename hg::regular_graph<embedding_t>::vertices_size_type
    num_vertices(const hg::regular_graph<embedding_t> &g) {
        return g.num_vertices();
    };

    template<typename embedding_t>
    std::pair<typename hg::regular_graph<embedding_t>::vertex_iterator, typename hg::regular_graph<embedding_t>::vertex_iterator>
    vertices(const hg::regular_graph<embedding_t> &g) {
        using vertex_iterator = typename hg::regular_graph<embedding_t>::vertex_iterator;
        return std::pair<vertex_iterator, vertex_iterator>(
                vertex_iterator(0),                 // The first iterator position
                vertex_iterator(num_vertices(g))); // The last iterator position
    }

    template<typename embedding_t>
    std::pair<typename hg::regular_graph<embedding_t>::adjacency_iterator, typename hg::regular_graph<embedding_t>::adjacency_iterator>
    adjacent_vertices(typename hg::regular_graph<embedding_t>::vertex_descriptor u, hg::regular_graph<embedding_t> &g) {
        return std::make_pair<typename hg::regular_graph<embedding_t>::adjacency_iterator, typename hg::regular_graph<embedding_t>::adjacency_iterator>(
                hg::regular_graph_adjacent_vertex_iterator<embedding_t>(u, g.embedding, g.neighbours.begin(),
                                                                        g.neighbours.end()),
                hg::regular_graph_adjacent_vertex_iterator<embedding_t>(u, g.embedding, g.neighbours.end(),
                                                                        g.neighbours.end()));
    };
}