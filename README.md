# RecursiveGraphCollections

A C++23 module with a C-compatible interface (`extern "C"`) designed for managing collections of named, recursively constructed simple undirected graphs[cite: 2, 3]. Developed as part of advanced programming studies at the University of Warsaw (MIMUW).

## Features

- **C-C++ Interoperability**: Seamlessly integrates C++ backend logic with C programs, exposing a clean procedural API wrapped inside the `cxx` namespace when compiled in C++[cite: 2, 3].
- **Advanced STL Containers**: Leverages standard C++ library containers (`std::unordered_map`, `std::map`, `std::set`, `std::multiset`, `std::tuple`) for efficient graph storage, lexicographical ordering, and multi-level tracking without custom heavy data structures[cite: 2].
- **Recursive Graph Operations**: Implements complex edge-replacement (`rgc_replace_edge`) mechanics where a selected edge in a recipient graph is replaced by a donor graph, maintaining hierarchical edge levels up to `LEVEL_MAX`[cite: 2].
- **Robust State Management**: Utilizes the *Construct-on-First-Use Idiom* to safely manage global collections and avoid static initialization order issues[cite: 2].

## API Overview

The module provides a comprehensive set of functions to manage:
- **Collections**: Creation and deletion of isolated graph collections identified by `long` IDs (`rgc_new_collection`, `rgc_delete_collection`)[cite: 2, 3].
- **Graphs**: Creation, deletion, naming validation, and size/level queries (`rgc_new_graph`, `rgc_graph_level`, etc.)[cite: 2, 3].
- **Nodes & Edges**: Dynamic addition/removal of vertices and edges with automatic numbering and level constraints (`rgc_graph_add_edge`, `rgc_replace_edge`, `rgc_edge_level`)[cite: 2, 3].

## Compilation

The project can be compiled using modern C++ and C compilers (C++23 / C23 standards). The maximum edge level (`LEVEL_MAX`) can be configured at compile time (default is 4):

```bash
g++ -c -Wall -Wextra -O2 -std=c++23 -DLEVEL_MAX=16 recursive_graph_collections.cpp -o recursive_graph_collections.o
