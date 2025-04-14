#include <algorithm>
#include <cassert>
#include <cmath>
#include <queue>

#include "inputgraph.h"
#include "boundarystatus.h"

InputGraph::Vertex::Vertex(int id) : id(id) {}

std::optional<int> InputGraph::Vertex::findAdjacencyTo(int to) const {
	auto it = std::find(adj.begin(), adj.end(), InputGraph::Adjacency{id, to});
	if (it == adj.end()) {
		return std::nullopt;
	} else {
		return std::distance(adj.begin(), it);
	}
}

void InputGraph::Vertex::addAdjacencyAfter(int to) {
	adj.emplace_back(id, to);
}

void InputGraph::Vertex::addAdjacencyBefore(int to) {
	adj.insert(adj.begin(), Adjacency{id, to});
}

static constexpr int directionDx[] = {1, 0, -1, 0};
static constexpr int directionDy[] = {0, -1, 0, 1};

/// Returns the coordinate obtained by going one step in the given
/// direction from the starting coordinate `c`.
HeightMap::Coordinate applyDirection(HeightMap::Coordinate c, int direction) {
	return {c.m_x + directionDx[direction], c.m_y + directionDy[direction]};
}

/// Returns the direction between the two given coordinates.
int directionBetween(HeightMap::Coordinate c1, HeightMap::Coordinate c2) {
	for (int i = 0; i < 4; i++) {
		if (c2.m_x == c1.m_x + directionDx[i] && c2.m_y == c1.m_y + directionDy[i]) {
			return i;
		}
	}
	return -1;
}

InputGraph::InputGraph() {}

InputGraph::InputGraph(const HeightMap& heightMap) :
    InputGraph(heightMap, Boundary(heightMap)) {}

InputGraph::InputGraph(const HeightMap& heightMap, Boundary boundary) {
	boundary = boundary.rasterize();
	m_vertexMap =
	    std::vector<std::vector<int>>(heightMap.width(), std::vector<int>(heightMap.height(), -1));

	// Preparation: keep track of which vertices are on the boundary, and in
	// which directions its two boundary edges go.
	std::vector<std::vector<bool>> vertexOnBoundary(heightMap.width(),
	                                                std::vector<bool>(heightMap.height(), false));
	std::vector<std::vector<int>> incomingBoundaryEdge(heightMap.width(),
	                                                   std::vector<int>(heightMap.height(), -1));
	std::vector<std::vector<int>> outgoingBoundaryEdge(heightMap.width(),
	                                                   std::vector<int>(heightMap.height(), -1));
	for (int i = 0; i < boundary.path().m_points.size() - 1; i++) {
		HeightMap::Coordinate p1 = boundary.path().m_points[i];
		HeightMap::Coordinate p2 = boundary.path().m_points[i + 1];
		vertexOnBoundary[p1.m_x][p1.m_y] = true;
		incomingBoundaryEdge[p2.m_x][p2.m_y] = directionBetween(p2, p1);
		outgoingBoundaryEdge[p1.m_x][p1.m_y] = directionBetween(p1, p2);
	}

	// Do a BFS through the area between the boundary edges to find all vertices
	// and edges that lie on the boundary or inside it.
	std::vector<std::vector<bool>> visited(
	        heightMap.width(), std::vector<bool>(heightMap.height(), false));
	std::queue<HeightMap::Coordinate> queue;
	HeightMap::Coordinate start = boundary.path().start();
	queue.push(boundary.path().start());

	// Insert the first vertex.
	m_vertexMap[start.m_x][start.m_y] =
	    addVertex(Point{static_cast<double>(start.m_x), static_cast<double>(start.m_y),
	                    heightMap.elevationAt(start.m_x, start.m_y)});

	while (!queue.empty()) {
		HeightMap::Coordinate coordinate = queue.front();
		queue.pop();
		if (visited[coordinate.m_x][coordinate.m_y]) {
			continue;
		}
		visited[coordinate.m_x][coordinate.m_y] = true;
		int vertexId = m_vertexMap[coordinate.m_x][coordinate.m_y];
		assert(vertexId != -1);

		// If the source vertex is on the inside, we don't care in which order
		// we consider its incident edges. However, if the source vertex is on
		// the boundary, we want to consider its incident edges starting from
		// the incident (incoming) boundary edge b.
		int startDirection = 0;
		if (vertexOnBoundary[coordinate.m_x][coordinate.m_y]) {
			startDirection = incomingBoundaryEdge[coordinate.m_x][coordinate.m_y];
		}

		// Now consider the incident edges, starting from the start edge we just
		// determined.
		for (int i = 0; i < 4; i++) {
			int direction = (i + startDirection) % 4;

			// Ignore edges that go out of bounds.
			HeightMap::Coordinate target = applyDirection(coordinate, direction);
			if (!heightMap.isInBounds(target)) {
				continue;
			}

			// Add the edge to the graph (adding the destination vertex if it
			// doesn't exist yet).
			if (m_vertexMap[target.m_x][target.m_y] == -1) {
				m_vertexMap[target.m_x][target.m_y] = addVertex(Point{
					static_cast<double>(target.m_x), static_cast<double>(target.m_y),
					heightMap.elevationAt(target.m_x, target.m_y)});
			}
			(*this)[vertexId].addAdjacencyAfter(m_vertexMap[target.m_x][target.m_y]);

			// Add the target vertex to the queue.
			queue.push(target);

			// If the edge we just added was the incoming boundary edge, then
			// this was the last edge on the inside, hence we should stop.
			if (incomingBoundaryEdge[target.m_x][target.m_y] == (direction + 2) % 4) {
				break;
			}
		}
	}

	// Mark adjacencies on the boundary as being on an impermeable or a
	// permeable section of said boundary.
	for (int i = 0; i < boundary.path().length(); i++) {
		HeightMap::Coordinate c1 = boundary.path().m_points[i];
		HeightMap::Coordinate c2 = boundary.path().m_points[(i + 1) % boundary.path().length()];
		markVertex(c1, BoundaryStatus::IMPERMEABLE);
		markVertex(c2, BoundaryStatus::IMPERMEABLE);
		markEdge(c1, c2, BoundaryStatus::IMPERMEABLE);
	}
	for (int regionId = 0; regionId < boundary.permeableRegions().size(); regionId++) {
		const Boundary::Region& region  = boundary.permeableRegions()[regionId];
		for (int i = region.m_start; i != region.m_end; i = (i + 1) % boundary.path().length()) {
			HeightMap::Coordinate c1 = boundary.path().m_points[i];
			HeightMap::Coordinate c2 = boundary.path().m_points[(i + 1) % boundary.path().length()];
			markVertex(c1, BoundaryStatus::PERMEABLE, i);
			markVertex(c2, BoundaryStatus::PERMEABLE, i);
			markEdge(c1, c2, BoundaryStatus::PERMEABLE, i);
		}
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

void InputGraph::markVertex(HeightMap::Coordinate c, BoundaryStatus status,
                            std::optional<int> permeableRegion) {
	int v = m_vertexMap[c.m_x][c.m_y];
	(*this)[v].boundaryStatus = status;
	(*this)[v].permeableRegion = permeableRegion;
}

void InputGraph::markEdge(HeightMap::Coordinate c1, HeightMap::Coordinate c2, BoundaryStatus status,
              std::optional<int> permeableRegion) {
	int v = m_vertexMap[c1.m_x][c1.m_y];
	int v2 = m_vertexMap[c2.m_x][c2.m_y];

	std::optional<int> adjIndex = (*this)[v].findAdjacencyTo(v2);
	assert(adjIndex.has_value());
	(*this)[v].adj[*adjIndex].boundaryStatus = status;
	(*this)[v].adj[*adjIndex].permeableRegion = permeableRegion;

	adjIndex = (*this)[v2].findAdjacencyTo(v);
	assert(adjIndex.has_value());
	(*this)[v2].adj[*adjIndex].boundaryStatus = status;
	(*this)[v2].adj[*adjIndex].permeableRegion = permeableRegion;
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

int InputGraph::addVertex(Point p) {
	int index = addVertex();
	(*this)[index].p = p;
	return index;
}

int InputGraph::edgeCount() const {
	int count = 0;
	for (int i = 0; i < m_verts.size(); i++) {
		count += m_verts[i].adj.size();
	}
	return count / 2;
}

void InputGraph::clearAllEdges() {
	for (auto v : m_verts) {
		v.adj.clear();
	}
}

bool InputGraph::isAscending(const InputGraph::Adjacency& a) const {
	return (*this)[a.from] < (*this)[a.to];
}

bool InputGraph::containsNodata() const {
	for (auto v : m_verts) {
		if (std::isnan(v.p.h)) {
			return true;
		}
	}
	return false;
}
