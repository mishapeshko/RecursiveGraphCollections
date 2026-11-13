#include "recursive_graph_collections.h"
#include <algorithm> // Used for max function.
#include <cctype>    // Used for isalnum function.
#include <climits>   // Used for INT_MAX and LONG_MAX constants.
#include <cstddef>   // Used for size_t type.
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility> // Used for move function.

// The maximal level of an edge is LEVEL_MAX - 1. The value can be changed by
// the -D option.
#ifndef LEVEL_MAX
#define LEVEL_MAX 4
#endif

namespace {
// Identifier of a collection.
using id_collection_t = long;
// Number of a vertex.
using vertex_t = int;
// Level of an edge or of a whole graph.
using level_t = int;
// Name of a graph.
using name_t = std::string;

// Adjacency list of a vertex: its neighbours with levels of the edges to them.
using adj_list_t = std::map<vertex_t, level_t>;
// Adjacency lists of all vertices of a graph.
using adj_lists_t = std::map<vertex_t, adj_list_t>;
// Graph is the tuple <adjacency lists, level of the graph, numbers of deleted
// vertices, number of edges, levels of all edges>. The levels of all edges are
// kept in a multiset, so the level of the graph can be found after deleting an
// edge.
using graph_t = std::tuple<adj_lists_t, level_t, std::set<vertex_t>, long,
                           std::multiset<level_t>>;
// Graphs of one collection, ordered lexicographically by their names.
using collection_t = std::map<name_t, graph_t>;
// Collection of graphs.
using collections_t = std::unordered_map<id_collection_t, collection_t>;

// Indexes of the fields of the graph_t tuple.
int const IDX_ADJ_LISTS = 0;
int const IDX_MAX_LEVEL = 1;
int const IDX_DELETED = 2;
int const IDX_EDGES_COUNT = 3;
int const IDX_LEVELS = 4;

// Value used when there is no collection identifier left.
const long NO_COLLECTION_ID = -1;

// All collections found by their identifiers.
// We use the Construct On First Use Idiom to avoid any initialization issues.
collections_t &collections() {
    static collections_t ret;
    return ret;
}

graph_t *get_graph(long id, char const *name) {
    auto it = collections().find(id);
    if (it == collections().end() || name == nullptr) {
        return nullptr;
    }

    name_t tmp(name);
    auto it_to_graph = it->second.find(tmp);
    if (it_to_graph == it->second.end()) {
        return nullptr;
    }
    graph_t &graph = it_to_graph->second;
    return &graph;
}

adj_list_t *get_adj_list(long id, char const *name, vertex_t x) {
    graph_t *graph = get_graph(id, name);
    if (!graph) {
        return nullptr;
    }

    auto it = std::get<IDX_ADJ_LISTS>(*graph).find(x);
    if (it == std::get<IDX_ADJ_LISTS>(*graph).end()) {
        return nullptr;
    }
    return &it->second;
}

// Deletes the edge from the given graph, assuming it is present in it.
void erase_edge(graph_t &graph, vertex_t x, vertex_t y) {
    adj_lists_t &adj_lists = std::get<IDX_ADJ_LISTS>(graph);
    level_t level = adj_lists[x][y];

    // Delete the edge from adjacency lists of both of its ends.
    adj_lists[x].erase(y);
    adj_lists[y].erase(x);
    // Delete the level of the edge from the multiset, update maximum. The
    // multiset can become empty, so rbegin() must not be dereferenced blindly.
    std::multiset<level_t> &levels = std::get<IDX_LEVELS>(graph);
    levels.extract(level);
    std::get<IDX_MAX_LEVEL>(graph) = levels.empty() ? 0 : *levels.rbegin();
    // Decrease the number of edges.
    std::get<IDX_EDGES_COUNT>(graph)--;
}

// Checks if the character can be used in a name of a graph.
bool is_valid(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// Returns true, when the name is invalid: it is empty or contains a character,
// which is not a letter, a digit or an underscore.
bool check_name(char const *name) {
    if (name[0] == '\0') {
        return true;
    }
    int idx = 0;
    while (name[idx] != '\0') {
        if (!is_valid(name[idx])) {
            return true;
        }
        idx++;
    }
    return false;
}
} // namespace

// Don't mangle names in our interface!
extern "C" {

// Creates a new empty collection and returns its identifier. Returns
// NO_COLLECTION_ID, when all identifiers were already used.
long rgc_new_collection() {
    // The next identifier to give or NO_COLLECTION_ID, when all of them were
    // used.
    static id_collection_t last_free = 0;
    if (last_free == NO_COLLECTION_ID) {
        return NO_COLLECTION_ID;
    }
    long result = last_free;
    collection_t new_collection;
    collections()[result] = new_collection;
    // There is no identifier after LONG_MAX.
    if (last_free == LONG_MAX) {
        last_free = NO_COLLECTION_ID;
    } else {
        last_free++;
    }
    return result;
}

// Deletes a collection by its id.
void rgc_delete_collection(long id) {
    collections().erase(id);
}

// Creates a graph with n vertices and no edges in the collection. Returns true,
// when the graph was created.
bool rgc_new_graph(long id, char const *name, int n) {
    if (name == nullptr) {
        return false;
    }
    bool error = check_name(name);
    if (error) {
        return false;
    }
    if (n < 0) {
        return false;
    }
    auto iter1 = collections().find(id);
    if (iter1 != collections().end()) {
        auto &inner_map = iter1->second;
        // The name has to be unique in the collection.
        auto iter2 = inner_map.find(name);
        if (iter2 != inner_map.end()) {
            return false;
        }
        graph_t new_graph;
        std::set<vertex_t> deleted;
        std::multiset<level_t> levels;
        adj_lists_t adj_lists;
        // Vertices are numbered from 0 to n - 1.
        for (vertex_t vertex_number = 0; vertex_number < n; vertex_number++) {
            adj_list_t adj_list;
            adj_lists[vertex_number] = adj_list;
        }
        std::get<IDX_DELETED>(new_graph) = deleted;
        std::get<IDX_EDGES_COUNT>(new_graph) = 0;
        std::get<IDX_LEVELS>(new_graph) = levels;
        std::get<IDX_MAX_LEVEL>(new_graph) = 0;
        std::get<IDX_ADJ_LISTS>(new_graph) = adj_lists;
        // The collection keeps a copy of the name.
        inner_map.emplace(name, new_graph);
        return true;
    }
    return false;
}

// Deletes a graph by its name, in a given collection.
void rgc_delete_graph(long id, char const *name) {
    auto it = collections().find(id);
    if (it == collections().end() || name == nullptr) {
        return;
    }

    name_t tmp(name);
    it->second.erase(tmp);
}

// Adds an isolated vertex with the smallest free number to the graph and
// returns the number. Returns -1, when the graph does not exist or has INT_MAX
// vertices.
int rgc_graph_new_node(long id, char const *name) {
    graph_t *graph = get_graph(id, name);
    if (!graph) {
        return -1;
    }

    std::size_t number_of_vertices = std::get<IDX_ADJ_LISTS>(*graph).size();
    if (number_of_vertices < INT_MAX) {
        vertex_t vertex_number;
        std::set<vertex_t> &deleted = std::get<IDX_DELETED>(*graph);
        // If some vertex was deleted, its number is the smallest free
        // one.
        if (deleted.empty()) {
            vertex_number = number_of_vertices;
        } else {
            vertex_number = *deleted.begin();
            deleted.erase(vertex_number);
        }
        adj_list_t adj_list;
        std::get<IDX_ADJ_LISTS>(*graph)[vertex_number] = adj_list;
        return vertex_number;
    }
    return -1;
}

// Deletes a node by its number, in a given graph. Removes all adjacent edges.
bool rgc_graph_delete_node(long id, char const *name, int x) {
    graph_t *graph = get_graph(id, name);
    if (!graph) {
        return false;
    }

    adj_lists_t &adj_lists = std::get<IDX_ADJ_LISTS>(*graph);
    auto it_to_adj = adj_lists.find(x);
    if (it_to_adj == adj_lists.end()) {
        return false;
    }
    // Copy the neighbours, because erase_edge modifies the adjacency lists.
    adj_list_t neighbours = it_to_adj->second;
    for (auto const &edge : neighbours) {
        erase_edge(*graph, x, edge.first);
    }

    // Mark node number as free and delete it from adjacency map.
    std::get<IDX_DELETED>(*graph).insert(x);
    adj_lists.erase(x);
    return true;
}

// Adds the edge {x, y} with the level 0 to the graph. Returns true, when the
// edge was added, so when both vertices exist, are different and were not
// connected.
bool rgc_graph_add_edge(long id, char const *name, int x, int y) {
    if (x == y) {
        return false;
    }
    graph_t *graph = get_graph(id, name);
    if (!graph) {
        return false;
    }

    adj_lists_t &adj_lists = std::get<IDX_ADJ_LISTS>(*graph);
    auto iter3 = adj_lists.find(x);
    auto iter4 = adj_lists.find(y);
    if (iter3 != adj_lists.end() && iter4 != adj_lists.end()) {
        adj_list_t *adj_list_x = get_adj_list(id, name, x);
        adj_list_t *adj_list_y = get_adj_list(id, name, y);
        if ((*adj_list_x).find(y) == adj_list_x->end()) {
            (*adj_list_x)[y] = 0;
            (*adj_list_y)[x] = 0;
            std::get<IDX_LEVELS>(*graph).insert(0);
            std::get<IDX_EDGES_COUNT>(*graph)++;
            return true;
        }
        return false;
    }
    return false;
}

// Deletes an edge {x, y} in a given graph.
bool rgc_graph_delete_edge(long id, char const *name, int x, int y) {
    graph_t *graph = get_graph(id, name);
    if (!graph) {
        return false;
    }

    adj_lists_t &adj_lists = std::get<IDX_ADJ_LISTS>(*graph);
    auto it_to_adj = adj_lists.find(x);
    if (it_to_adj == adj_lists.end() || !it_to_adj->second.count(y)) {
        return false;
    }

    erase_edge(*graph, x, y);
    return true;
}

// Returns the name of the first graph in the collection or nullptr, when the
// collection does not exist or is empty. The pointer points to the name stored
// in the collection, so it is valid until the graph is deleted.
char const *rgc_first_graph(long id) {
    auto iter1 = collections().find(id);
    if (iter1 != collections().end()) {
        collection_t &collection = iter1->second;
        if (!collection.empty()) {
            name_t const &graph_name = collection.begin()->first;
            char const *graph_name_const = graph_name.c_str();
            return graph_name_const;
        }
        return nullptr;
    }
    return nullptr;
}

// Returns the name of the next (in the lexicographical order) graph in the
// collection. The pointer points to the name stored in the collection, so it is
// valid until the graph is deleted.
char const *rgc_next_graph(long id, char const *name) {
    auto it = collections().find(id);
    if (it == collections().end() || name == nullptr) {
        return nullptr;
    }

    name_t tmp(name);
    auto it_to_graph = it->second.upper_bound(tmp);
    return (it_to_graph == it->second.end()) ? nullptr
                                             : it_to_graph->first.c_str();
}

// Returns the smallest number of a vertex of the graph or -1, when there is no
// such vertex.
int rgc_first_node(long id, char const *name) {
    graph_t *graph = get_graph(id, name);
    if (!graph) {
        return -1;
    }
    adj_lists_t &adj_lists = std::get<IDX_ADJ_LISTS>(*graph);
    if (!adj_lists.empty()) {
        vertex_t first_node = adj_lists.begin()->first;
        return first_node;
    }
    return -1;
}

// Returns the number of the next vertex in the graph or -1, when there is no
// such vertex.
int rgc_next_node(long id, char const *name, int x) {
    graph_t *graph = get_graph(id, name);
    if (!graph) {
        return -1;
    }

    auto const &adj_map = std::get<IDX_ADJ_LISTS>(*graph);
    auto it_to_vert = adj_map.upper_bound(x);
    return (it_to_vert == adj_map.end()) ? -1 : it_to_vert->first;
}

// Returns the smallest neighbour of the vertex x or -1, when the graph or the
// vertex does not exist or the vertex has no neighbours.
int rgc_first_edge(long id, char const *name, int x) {
    adj_list_t *adj_list_x = get_adj_list(id, name, x);
    if (!adj_list_x) {
        return -1;
    }
    if (!adj_list_x->empty()) {
        vertex_t smallest_neighbour = adj_list_x->begin()->first;
        return smallest_neighbour;
    }
    return -1;
}

// Returns the smallest neighbour of the vertex x after y or -1, when y was the
// largest neighbour.
int rgc_next_edge(long id, char const *name, int x, int y) {
    adj_list_t *adj_list = get_adj_list(id, name, x);
    if (!adj_list) {
        return -1;
    }

    auto it_to_neigh = adj_list->upper_bound(y);
    return (it_to_neigh == adj_list->end()) ? -1 : it_to_neigh->first;
}

// Creates the graph result_name by replacing the edge {w_1, w_2} of the
// recipient with the donor, whose vertices v_1 and v_2 are identified with w_1
// and w_2. Returns true, when the graph was created.
bool rgc_replace_edge(long id, char const *result_name,
                      char const *recipient_name, int w_1, int w_2,
                      char const *donor_name, int v_1, int v_2) {
    if (result_name == nullptr || recipient_name == nullptr ||
        donor_name == nullptr) {
        return false;
    }
    auto iter1 = collections().find(id);
    if (iter1 == collections().end()) {
        return false;
    }
    bool error = check_name(result_name);
    if (error) {
        return false;
    }
    auto &collection_map = iter1->second;
    if (collection_map.find(result_name) != collection_map.end()) {
        return false;
    }
    graph_t *recipient_graph = get_graph(id, recipient_name);
    graph_t *donor_graph = get_graph(id, donor_name);
    if (!recipient_graph || !donor_graph) {
        return false;
    }
    // The recipient has to contain the edge {w_1, w_2}.
    adj_list_t *adj_list_w_1 = get_adj_list(id, recipient_name, w_1);
    if (!adj_list_w_1) {
        return false;
    }

    auto iter3 = adj_list_w_1->find(w_2);
    if (iter3 == adj_list_w_1->end()) {
        return false;
    }
    // The donor has to contain two different vertices v_1 and v_2.
    adj_lists_t &adj_lists_donor = std::get<IDX_ADJ_LISTS>(*donor_graph);
    auto iter4 = adj_lists_donor.find(v_1);
    auto iter5 = adj_lists_donor.find(v_2);
    if (v_1 == v_2 || iter4 == adj_lists_donor.end() ||
        iter5 == adj_lists_donor.end()) {
        return false;
    }
    // The result has all vertices of the recipient and k - 2 new vertices of
    // the donor.
    adj_lists_t &adj_lists_recipient =
        std::get<IDX_ADJ_LISTS>(*recipient_graph);
    std::size_t k = adj_lists_donor.size();
    std::size_t result_size = adj_lists_recipient.size() + k - 2;
    if (result_size > INT_MAX) {
        return false;
    }
    // New edges have the level greater by one than the donor and the replaced
    // edge.
    level_t donor_level = std::get<IDX_MAX_LEVEL>(*donor_graph);
    level_t deleted_level = iter3->second;
    level_t new_level = std::max(donor_level, deleted_level) + 1;
    if (new_level >= LEVEL_MAX) {
        return false;
    }
    // Copy the recipient graph.
    graph_t result_graph = *recipient_graph;
    adj_lists_t &adj_lists_result = std::get<IDX_ADJ_LISTS>(result_graph);
    std::multiset<level_t> &levels_result = std::get<IDX_LEVELS>(result_graph);
    // Deleting the replaced edge from the result. Only one occurrence of its
    // level is erased.
    adj_lists_result[w_1].erase(w_2);
    adj_lists_result[w_2].erase(w_1);
    levels_result.extract(deleted_level);
    std::get<IDX_EDGES_COUNT>(result_graph)--;
    // Numbers of the vertices deleted from the result can be used again.
    std::set<vertex_t> &deleted = std::get<IDX_DELETED>(result_graph);
    std::unordered_map<vertex_t, vertex_t> vertices_donor_result_mapping;
    auto iter_pom = adj_lists_donor.begin();
    // Each vertex of the donor except v_1 and v_2 gets the smallest free
    // number.
    for (std::size_t number = 0; number < k - 2; number++) {
        vertex_t next_vertex_number;
        if (!deleted.empty()) {
            next_vertex_number = *deleted.begin();
            deleted.erase(next_vertex_number);
        } else {
            next_vertex_number = adj_lists_result.size();
        }
        // The vertices v_1 and v_2 are identified with w_1 and w_2, so they are
        // skipped.
        while (iter_pom->first == v_1 || iter_pom->first == v_2) {
            ++iter_pom;
        }
        vertices_donor_result_mapping[iter_pom->first] = next_vertex_number;
        ++iter_pom;
        adj_list_t adj_list_new;
        adj_lists_result[next_vertex_number] = adj_list_new;
    }
    vertices_donor_result_mapping[v_1] = w_1;
    vertices_donor_result_mapping[v_2] = w_2;
    // Moving the edges of the donor to the result.
    iter_pom = adj_lists_donor.begin();
    for (std::size_t number = 0; number < k; number++) {
        vertex_t w_k = vertices_donor_result_mapping[iter_pom->first];
        adj_list_t &adj_list_w_k = adj_lists_result[w_k];
        adj_list_t &adj_list_v_k = iter_pom->second;
        for (auto iter = adj_list_v_k.begin(); iter != adj_list_v_k.end();
             iter++) {
            // Each edge is in two adjacency lists, so it is moved only once.
            if (iter_pom->first > iter->first) {
                continue;
            }
            vertex_t neighbour_w_k = vertices_donor_result_mapping[iter->first];
            adj_list_t &adj_list_neighbour_w_k =
                adj_lists_result[neighbour_w_k];
            adj_list_w_k[neighbour_w_k] = new_level;
            adj_list_neighbour_w_k[w_k] = new_level;
            std::get<IDX_EDGES_COUNT>(result_graph)++;
            levels_result.insert(new_level);
        }
        ++iter_pom;
    }
    // The level of the result is the greatest level of its edges or 0, if it
    // has no edges.
    std::get<IDX_MAX_LEVEL>(result_graph) =
        levels_result.empty() ? 0 : *levels_result.rbegin();
    collection_map.emplace(result_name, std::move(result_graph));
    return true;
}

// Returns the level of the given edge in the graph.
ssize_t rgc_edge_level(long id, char const *name, int x, int y) {
    adj_list_t *adj_list = get_adj_list(id, name, x);
    if (!adj_list) {
        return -1;
    }

    auto it_to_neigh = adj_list->find(y);
    return (it_to_neigh == adj_list->end()) ? -1 : it_to_neigh->second;
}

// Returns the level of the graph or -1, when the graph does not exist.
ssize_t rgc_graph_level(long id, char const *name) {
    graph_t *graph = get_graph(id, name);
    if (!graph) {
        return -1;
    }
    return std::get<IDX_MAX_LEVEL>(*graph);
}

// Returns the maximal possible edge level.
ssize_t rgc_max_level() {
    return LEVEL_MAX - 1;
}

// Returns the number of vertices of the graph or -1, when the graph does not
// exist.
ssize_t rgc_count_nodes(long id, char const *name) {
    graph_t *graph = get_graph(id, name);
    if (!graph) {
        return -1;
    }
    return std::get<IDX_ADJ_LISTS>(*graph).size();
}

// Returns the number of edges in the graph.
ssize_t rgc_count_edges(long id, char const *name) {
    graph_t *graph = get_graph(id, name);
    if (!graph) {
        return -1;
    }

    return std::get<IDX_EDGES_COUNT>(*graph);
}

// Returns the number of graphs in the collection or -1, when the collection
// does not exist.
ssize_t rgc_size(long id) {
    auto iter1 = collections().find(id);
    if (iter1 != collections().end()) {
        return iter1->second.size();
    }
    return -1;
}
} // extern "C"
