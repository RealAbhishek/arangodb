//
// Created by roman on 25.02.22.
//

#include "Graph02.h"
#include "velocypack/Value.h"

using namespace arangodb;
using namespace arangodb::pregel3;
using namespace arangodb::velocypack;

template<class V, class E>
std::pair<size_t, bool> Graph<V, E>::addEdge(size_t from, size_t to) {
  TRI_ASSERT(from < numVertices());
  TRI_ASSERT(to < numVertices());
  size_t idx;
  // amortized O(1)
  while (!holes.empty() && holes.back() > edges.size() - 1) {
    holes.pop_back();
  }
  if (!holes.empty()) {
    idx = holes.back();
    holes.pop_back();
  } else {
    idx = edges.size();
  }
  auto [it, success] = edges.emplace(std::make_pair(idx, Edge(from, to, idx)));
  if (success) {
    auto e = it->second;
    vertices[from].outEdges.insert(std::make_pair(to, &e));
    vertices[to].outEdges.insert(std::make_pair(from, &e));
    return {e.idx, true};
  }
  return {0, false};
}

template<class V, class E>
void Graph<V, E>::removeEdge(E* e) {
  size_t idx = e->idx;
  vertices[e->to].outEdges.erase(idx);
  vertices[e->from].inEdges.erase(idx);
  if (idx < edges.size() - 1) {
    holes.push_back(idx);
  }
  edges.erase(idx);
}

template struct arangodb::pregel3::Graph<MinCutVertex, MinCutEdge>;