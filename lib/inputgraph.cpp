#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <queue>

#include "inputgraph.h"

InputGraph::Vertex::Vertex(int id) : id(id) {}

void InputGraph::Vertex::addAdjacency(int to, bool disambiguation) {
	adj.emplace_back(id, to, disambiguation);
}

InputGraph::InputGraph(Units units) :
    m_units(units) {
}

InputGraph::InputGraph(const HeightMap& heightMap, Units units) :
    InputGraph(heightMap, Boundary(heightMap), units) {}

InputGraph::InputGraph(const HeightMap& heightMap,
                       Boundary boundary,
                       Units units) :
    m_units(units) {

	boundary = boundary.rasterize();

	// first add the global source, sink and maximum
	int globalSource = addVertex();
	(*this)[globalSource].p.x = -1;
	(*this)[globalSource].p.y = heightMap.height() / 2;
	(*this)[globalSource].p.h = -std::numeric_limits<double>::infinity();

	int globalSink = addVertex();
	(*this)[globalSink].p.x = heightMap.width();
	(*this)[globalSink].p.y = heightMap.height() / 2;
	(*this)[globalSink].p.h = -std::numeric_limits<double>::infinity();

	int globalMaximum = addVertex();
	(*this)[globalMaximum].p.x = heightMap.width() / 2;
	(*this)[globalMaximum].p.y = -1;
	(*this)[globalMaximum].p.h = std::numeric_limits<double>::infinity();

	// save mapping from HeightMap coordinates to vertex IDs
	// vertexMap[x][y] is the index of the InputGraph vertex representing this
	// HeightMap coordinate
	std::vector<std::vector<int>> vertexMap(
	        heightMap.width(), std::vector<int>(heightMap.height(), -1));

	struct Edge {
		HeightMap::Coordinate source;
		int direction;

		Edge(HeightMap::Coordinate s, int d) :
		    source(s), direction(d) {}

		Edge(HeightMap::Coordinate p1, HeightMap::Coordinate p2) :
		    source(p1), direction(directionBetween(p1, p2)) {}

		HeightMap::Coordinate destination() {
			int dx[] = {1, 0, -1, 0};
			int dy[] = {0, -1, 0, 1};
			return {source.m_x + dx[direction],
			        source.m_y + dy[direction]};
		}

		static int directionBetween(HeightMap::Coordinate p1,
		                            HeightMap::Coordinate p2) {
			int dx[] = {1, 0, -1, 0};
			int dy[] = {0, -1, 0, 1};
			for (int i = 0; i < 4; i++) {
				if (p2.m_x == p1.m_x + dx[i] && p2.m_y == p1.m_y + dy[i]) {
					return i;
				}
			}
			return -1;
		}
	};

	// which edges are boundary edges?
	// boundaryEdges[x][y][d] = edge (x, y) in direction d (see Edge)
	std::vector<std::vector<std::array<bool, 4>>> topEdges(
	            heightMap.width(),
	            std::vector<std::array<bool, 4>>(heightMap.height(),
	                                             std::array<bool, 4>{}));
	std::vector<std::vector<std::array<bool, 4>>> bottomEdges(
	            heightMap.width(),
	            std::vector<std::array<bool, 4>>(heightMap.height(),
	                                             std::array<bool, 4>{}));
	std::vector<std::vector<std::array<bool, 4>>> sourceEdges(
	            heightMap.width(),
	            std::vector<std::array<bool, 4>>(heightMap.height(),
	                                             std::array<bool, 4>{}));
	std::vector<std::vector<std::array<bool, 4>>> sinkEdges(
	            heightMap.width(),
	            std::vector<std::array<bool, 4>>(heightMap.height(),
	                                             std::array<bool, 4>{}));
	for (int i = 0; i < boundary.m_top.m_points.size() - 1; i++) {
		HeightMap::Coordinate p1 = boundary.m_top.m_points[i];
		HeightMap::Coordinate p2 = boundary.m_top.m_points[i + 1];
		topEdges[p1.m_x][p1.m_y][Edge::directionBetween(p1, p2)] = true;
		topEdges[p2.m_x][p2.m_y][Edge::directionBetween(p2, p1)] = true;
	}
	for (int i = 0; i < boundary.m_bottom.m_points.size() - 1; i++) {
		HeightMap::Coordinate p1 = boundary.m_bottom.m_points[i];
		HeightMap::Coordinate p2 = boundary.m_bottom.m_points[i + 1];
		bottomEdges[p1.m_x][p1.m_y][Edge::directionBetween(p1, p2)] = true;
		bottomEdges[p2.m_x][p2.m_y][Edge::directionBetween(p2, p1)] = true;
	}
	for (int i = 0; i < boundary.m_source.m_points.size() - 1; i++) {
		HeightMap::Coordinate p1 = boundary.m_source.m_points[i];
		HeightMap::Coordinate p2 = boundary.m_source.m_points[i + 1];
		sourceEdges[p1.m_x][p1.m_y][Edge::directionBetween(p1, p2)] = true;
		sourceEdges[p2.m_x][p2.m_y][Edge::directionBetween(p2, p1)] = true;
	}
	for (int i = 0; i < boundary.m_sink.m_points.size() - 1; i++) {
		HeightMap::Coordinate p1 = boundary.m_sink.m_points[i];
		HeightMap::Coordinate p2 = boundary.m_sink.m_points[i + 1];
		sinkEdges[p1.m_x][p1.m_y][Edge::directionBetween(p1, p2)] = true;
		sinkEdges[p2.m_x][p2.m_y][Edge::directionBetween(p2, p1)] = true;
	}

	std::vector<std::vector<bool>> marked(
	        heightMap.width(), std::vector<bool>(heightMap.height(), false));

	// main part: search through the area between the boundary edges
	std::queue<Edge> edges;
	Edge startEdge(boundary.m_bottom.m_points[1], boundary.m_bottom.m_points[0]);
	if (!boundary.isClockwise()) {
		startEdge = Edge(boundary.m_bottom.m_points[0], boundary.m_bottom.m_points[1]);
	}
	edges.push(startEdge);
	int vertex = vertexMap[startEdge.source.m_x][startEdge.source.m_y];
	if (vertex == -1) {
		vertex = addVertex();
		(*this)[vertex].p.x = startEdge.source.m_x;
		(*this)[vertex].p.y = startEdge.source.m_y;
		(*this)[vertex].p.h = heightMap.elevationAt(startEdge.source.m_x,
		                                            startEdge.source.m_y);
		vertexMap[startEdge.source.m_x][startEdge.source.m_y] = vertex;
	}
	marked[startEdge.source.m_x][startEdge.source.m_y] = true;

	while (!edges.empty()) {

		// take first edge
		Edge edge = edges.front();
		edges.pop();
		//std::cout << "Graph vertex " << edge.source.m_x << " " << edge.source.m_y << " (direction " << edge.direction << ")" << std::endl;

		int origin = vertexMap[edge.source.m_x][edge.source.m_y];
		assert(origin != -1);

		// check all edges in counter-clockwise order
		int startDirection = edge.direction;
		bool inside = true;

		for (int i = 0; i < 4; i++) {
			int direction = (i + startDirection) % 4;

			//std::cout << "    checking direction " << direction << std::endl;

			// ignore edges that go out of bounds
			HeightMap::Coordinate destination =
			        Edge(edge.source, direction).destination();
			if (!heightMap.isInBounds(destination)) {
				continue;
			}

			bool onTop =
			        topEdges[edge.source.m_x][edge.source.m_y][direction];
			bool onBottom =
			        bottomEdges[edge.source.m_x][edge.source.m_y][direction];
			bool onSource =
			        sourceEdges[edge.source.m_x][edge.source.m_y][direction];
			bool onSink =
			        sinkEdges[edge.source.m_x][edge.source.m_y][direction];

			bool originIsOnlySource = boundary.m_source.m_points.size() == 1 &&
			        boundary.m_source.m_points[0].m_x == (*this)[origin].p.x &&
			        boundary.m_source.m_points[0].m_y == (*this)[origin].p.y;
			bool originIsOnlySink = boundary.m_sink.m_points.size() == 1 &&
			        boundary.m_sink.m_points[0].m_x == (*this)[origin].p.x &&
			        boundary.m_sink.m_points[0].m_y == (*this)[origin].p.y;

			if (i != 0 && (onTop || onBottom || onSource || onSink)) {
				inside = !inside;
			}

			/*if (inside) {
				std::cout << "    we are currently inside" << std::endl;
			} else {
				std::cout << "    we are currently outside" << std::endl;
			}
			std::cout << "    edges: " << onTop << " " << onBottom << " " << onSource
			          << " " << onSink << std::endl;*/

			if (inside) {
				if (onTop || onBottom) {
					if (originIsOnlySource) {
						//std::cout << "    special edge to global source" << std::endl;
						(*this)[origin].addAdjacency(globalSource);
					} else if (originIsOnlySink) {
						//std::cout << "    special edge to global sink" << std::endl;
						(*this)[origin].addAdjacency(globalSink);
					}
					//std::cout << "    edge to global maximum" << std::endl;
					(*this)[origin].addAdjacency(globalMaximum, onBottom);
				} else if (onSource) {
					//std::cout << "    edge to global source" << std::endl;
					(*this)[origin].addAdjacency(globalSource);
				} else if (onSink) {
					//std::cout << "    edge to global sink" << std::endl;
					(*this)[origin].addAdjacency(globalSink);
				}
			}
			if (onTop || onBottom || onSource || onSink || inside) {
				//std::cout << "    edge to " << destination.m_x << " " << destination.m_y << std::endl;

				// add destination vertex if it doesn't exist yet
				int vertex = vertexMap[destination.m_x][destination.m_y];
				if (vertex == -1) {
					vertex = addVertex();
					(*this)[vertex].p.x = destination.m_x;
					(*this)[vertex].p.y = destination.m_y;
					(*this)[vertex].p.h = heightMap.elevationAt(destination.m_x,
					                                            destination.m_y);
					vertexMap[destination.m_x][destination.m_y] = vertex;
				}

				(*this)[origin].addAdjacency(vertex);
			}
			if (!inside) {
				if (onTop || onBottom) {
					//std::cout << "    edge to global maximum" << std::endl;
					(*this)[origin].addAdjacency(globalMaximum, onBottom);
				} else if (onSource) {
					//std::cout << "    edge to global source" << std::endl;
					(*this)[origin].addAdjacency(globalSource);
				} else if (onSink) {
					//std::cout << "    edge to global sink" << std::endl;
					(*this)[origin].addAdjacency(globalSink);
				}
			}

			if (inside) {
				if (!marked[destination.m_x][destination.m_y]) {
					marked[destination.m_x][destination.m_y] = true;
					edges.push(Edge(destination, (direction + 1) % 4));
				}
			}
		}

		// remove double adjacencies
		std::vector<Adjacency>& adj = (*this)[origin].adj;
		for (int i = 1; i < adj.size(); i++) {
			if (adj[i] == adj[i - 1]) {
				adj.erase(adj.begin() + i);
				i--;
			}
		}
		if (adj.front() == adj.back()) {
			adj.erase(adj.end() - 1);
		}
	}

	// connect globalSource to all source vertices
	for (auto c = boundary.m_source.m_points.begin(); c < boundary.m_source.m_points.end(); c++) {
		assert(vertexMap[(*c).m_x][(*c).m_y] != -1);
		(*this)[globalSource].addAdjacency(vertexMap[(*c).m_x][(*c).m_y]);
	}
	(*this)[globalSource].addAdjacency(globalMaximum);

	// connect globalSink to all sink vertices
	for (auto c = boundary.m_sink.m_points.begin(); c < boundary.m_sink.m_points.end(); c++) {
		assert(vertexMap[(*c).m_x][(*c).m_y] != -1);
		(*this)[globalSink].addAdjacency(vertexMap[(*c).m_x][(*c).m_y]);
	}
	(*this)[globalSink].addAdjacency(globalMaximum);

	// connect globalMaximum to all boundary vertices
	(*this)[globalMaximum].addAdjacency(globalSource);
	for (auto c = boundary.m_top.m_points.begin(); c < boundary.m_top.m_points.end(); c++) {
		assert(vertexMap[(*c).m_x][(*c).m_y] != -1);
		(*this)[globalMaximum].addAdjacency(vertexMap[(*c).m_x][(*c).m_y],
		        false);
	}
	(*this)[globalMaximum].addAdjacency(globalSink);
	for (auto c = boundary.m_bottom.m_points.begin(); c < boundary.m_bottom.m_points.end(); c++) {
		assert(vertexMap[(*c).m_x][(*c).m_y] != -1);
		(*this)[globalMaximum].addAdjacency(vertexMap[(*c).m_x][(*c).m_y],
		        true);
	}
}

void InputGraph::addNeighbor(const HeightMap& heightMap, int v,
		int source, int sink, int globalMaximum,
		int nx, int ny) {

	if (heightMap.isInBounds(nx, ny)) {
		(*this)[v].addAdjacency(heightMap.height() * nx + ny + 3);
	} else if ((ny == -1 || ny == heightMap.height()) &&
				nx == (*this)[v].p.x) {
		// neighbor above or below the river: edge to global maximum
		(*this)[v].addAdjacency(globalMaximum);
	} else if (nx == -1 && ny == (*this)[v].p.y) {
		// neighbor to the left of the river: insert edge to source
		(*this)[v].addAdjacency(source);
	} else if (nx == heightMap.width() && ny == (*this)[v].p.y) {
		// neighbor to the right of the river: insert edge to sink
		(*this)[v].addAdjacency(sink);
	}
}

std::vector<HeightMap::Coordinate> neighborsOf(HeightMap::Coordinate v) {

	int x = v.m_x;
	int y = v.m_y;

	std::vector<HeightMap::Coordinate> result;
	result.emplace_back(x + 1, y);
	result.emplace_back(x, y - 1);
	result.emplace_back(x - 1, y);
	result.emplace_back(x, y + 1);

	return result;
}

InputGraph::Vertex& InputGraph::operator[](int i) {
	return m_verts[i];
}

const InputGraph::Vertex& InputGraph::operator[](int i) const {
	return m_verts[i];
}

int InputGraph::vertexCount() const {
	return m_verts.size();
}

int InputGraph::addVertex() {
	Vertex v(m_verts.size());
	m_verts.push_back(v);
	return m_verts.size() - 1;
}

void InputGraph::clearAllEdges() {
	for (auto v : m_verts) {
		v.adj.clear();
	}
}

bool InputGraph::isAscending(const InputGraph::Adjacency& a) const {
	return (*this)[a.from] < (*this)[a.to];
}

int InputGraph::steepestDescentFrom(int i) const {
	int steepest = -1;

	const std::vector<InputGraph::Adjacency>& adj = (*this).m_verts[i].adj;
	for (int j = 0; j < adj.size(); j++) {
		if (!isAscending(adj[j])) {
			if (steepest == -1 || compareSteepness(adj[j], adj[steepest])) {
				steepest = j;
			}
		}
	}

	return steepest;
}

bool InputGraph::compareSteepness(const Adjacency& a1, const Adjacency& a2) const {
	double h1 = (*this)[a1.to].p.h - (*this)[a1.from].p.h;
	double h2 = (*this)[a2.to].p.h - (*this)[a2.from].p.h;
	double l1 = adjacencyLength(a1);
	double l2 = adjacencyLength(a2);

	// break ties by operator<
	if (h1 * l2 == h2 * l1) {
		return a1.to < a2.to;
	}

	return h1 * l2 < h2 * l1;
}

double InputGraph::adjacencyLength(const Adjacency& a) const {
	return m_units.length((*this)[a.from].p, (*this)[a.to].p);
}
