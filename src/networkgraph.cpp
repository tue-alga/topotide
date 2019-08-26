#include "networkgraph.h"

#include <algorithm>

NetworkGraph::Vertex::Vertex(int id, Point p) :
    id(id), p(p) {
}

NetworkGraph::NetworkGraph() = default;

NetworkGraph::Vertex& NetworkGraph::operator[](int i) {
	return m_verts[i];
}

const NetworkGraph::Vertex& NetworkGraph::operator[](int i) const {
	return m_verts[i];
}

int NetworkGraph::vertexCount() const {
	return m_verts.size();
}

int NetworkGraph::addVertex(Point p) {
	Vertex v(m_verts.size(), p);
	m_verts.push_back(v);
	return m_verts.size() - 1;
}

const NetworkGraph::Edge& NetworkGraph::edge(int i) const {
	return m_edges[i];
}

NetworkGraph::Edge& NetworkGraph::edge(int i) {
	return m_edges[i];
}

int NetworkGraph::edgeCount() const {
	return m_edges.size();
}

int NetworkGraph::addEdge(int from, int to, std::vector<Point> path,
                          double delta) {
	int edgeIndex = m_edges.size();
	Edge e(edgeIndex, from, to, path);
	e.delta = delta;
	m_edges.push_back(e);

	m_verts[from].incidentEdges.push_back(edgeIndex);
	m_verts[to].incidentEdges.push_back(edgeIndex);
	return edgeIndex;
}

void NetworkGraph::filterOnDelta(double threshold) {
	m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
	                             [threshold](Edge e) {
	                  return e.delta < threshold;
	              }), m_edges.end());
}
