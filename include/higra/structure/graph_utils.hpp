//
// Created by user on 3/13/18.
//

#pragma once


namespace hg {

    using std::source;
    using std::target;
    using boost::out_edges;
    using boost::in_edges;
    using boost::in_degree;
    using boost::out_degree;
    using boost::degree;
    using boost::vertices;
    using boost::edges;
    using boost::add_vertex;
    using boost::add_edge;
    using boost::num_vertices;
    using boost::num_edges;
    using boost::adjacent_vertices;


    template<typename iterator_t>
    struct iterator_wrapper {
        iterator_t const first;
        iterator_t const last;

        iterator_wrapper(iterator_t &_first, iterator_t &_last) : first(_first), last(_last) {};

        iterator_wrapper(const std::pair<iterator_t, iterator_t> &p) : first(p.first), last(p.second) {};

        iterator_t begin() {
            return first;
        }

        iterator_t end() {
            return last;
        }

        iterator_t begin() const {
            return first;
        }

        iterator_t end() const {
            return last;
        }
    };

    template<typename graph_t>
    auto vertex_iterator(const graph_t &g) {
        using it_t = typename boost::graph_traits<graph_t>::vertex_iterator;
        return iterator_wrapper<it_t>(boost::vertices(g));
    }

    template<typename graph_t>
    auto edge_iterator(const graph_t &g) {
        using it_t = typename boost::graph_traits<graph_t>::edge_iterator;
        return iterator_wrapper<it_t>(boost::edges(g));
    }

    template<typename graph_t>
    auto out_edge_iterator(typename boost::graph_traits<graph_t>::vertex_descriptor v, const graph_t &g) {
        using it_t = typename boost::graph_traits<graph_t>::out_edge_iterator;
        return iterator_wrapper<it_t>(boost::out_edges(v, g));
    }

    template<typename graph_t>
    auto in_edge_iterator(typename boost::graph_traits<graph_t>::vertex_descriptor v, const graph_t &g) {
        using it_t = typename boost::graph_traits<graph_t>::in_edge_iterator;
        return iterator_wrapper<it_t>(boost::in_edges(v, g));
    }

    template<typename graph_t>
    auto adjacent_vertex_iterator(typename boost::graph_traits<graph_t>::vertex_descriptor v, const graph_t &g) {
        using it_t = typename boost::graph_traits<graph_t>::adjacency_iterator;
        return iterator_wrapper<it_t>(boost::adjacent_vertices(v, g));
    }

    template<typename graph_t>
    auto edge_index_iterator(const graph_t &g) {
        using it_t = typename graph_t::edge_index_iterator;
        return iterator_wrapper<it_t>(hg::edge_indexes(g));
    }

    template<typename graph_t>
    auto out_edge_index_iterator(typename graph_t::vertex_descriptor v, const graph_t &g) {
        using it_t = typename graph_t::out_edge_index_iterator;
        return iterator_wrapper<it_t>(hg::out_edge_indexes(v, g));
    }

    template<typename graph_t>
    auto in_edge_index_iterator(typename graph_t::vertex_descriptor v, const graph_t &g) {
        using it_t = typename graph_t::in_edge_index_iterator;
        return iterator_wrapper<it_t>(hg::in_edge_indexes(v, g));
    }


    template<typename graph_t>
    auto children_iterator(typename graph_t::vertex_descriptor v, const graph_t &g) {
        using it_t = typename graph_t::children_iterator;
        return iterator_wrapper<it_t>(hg::children(v, g));
    }
}