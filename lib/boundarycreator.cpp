#include "boundarycreator.h"
#include "heightmap.h"
#include "path.h"
#include <cmath>
#include <optional>

BoundaryCreator::BoundaryCreator(HeightMap heightMap)
    : m_heightMap(heightMap) {
	m_inputDcel = InputDcel(InputGraph(heightMap));
}

void BoundaryCreator::setSeed(HeightMap::Coordinate seed) {
	using Coordinate = HeightMap::Coordinate;
	using Vertex = InputDcel::Vertex;
	using HalfEdge = InputDcel::HalfEdge;
	using Face = InputDcel::Face;

	Vertex startVertex = m_inputDcel.vertexAt(seed.m_x, seed.m_y);
	if (!startVertex.isInitialized()) {
		m_path = std::nullopt;
		return;
	}
	Face startFace = startVertex.incidentFace();  // TODO pick suitable one
	if (!std::isfinite(startFace.data().p.h)) {
		m_path = std::nullopt;
		return;
	}

	// keep track of reachable faces
	// (keep 1 row/column boundary around the grid to avoid having to handle
	// corner cases where we run out of the grid)
	std::vector<std::vector<bool>> reached(m_heightMap.width() + 2,
	                                       std::vector<bool>(m_heightMap.height() + 2, false));
	int x = static_cast<int>(startFace.data().p.x);
	int y = static_cast<int>(startFace.data().p.y);
	reached[x + 1][y + 1] = true;  // + 1 because of the boundary

	startFace.forAllReachableFaces(
	    [&reached](HalfEdge e) -> bool {
		    Face f = e.oppositeFace();
		    int x = static_cast<int>(f.data().p.x);
		    int y = static_cast<int>(f.data().p.y);
		    if (reached[x + 1][y + 1]) {
				return false;
			}
		    bool faceHasNodata = false;
		    f.forAllBoundaryVertices([&faceHasNodata](Vertex v) {
			    if (!std::isfinite(v.data().p.h)) {
				    faceHasNodata = true;
			    }
		    });
		    return !faceHasNodata;
	    },
	    [&reached](Face f, HalfEdge) {
		    int x = static_cast<int>(f.data().p.x);
		    int y = static_cast<int>(f.data().p.y);
		    reached[x + 1][y + 1] = true;
	    });

	// find leftmost, topmost reached face
	int startX = -1;
	int startY = -1;
	for (int x = 1; x < reached.size() - 1; x++) {
		for (int y = 1; y < reached[0].size() - 1; y++) {
			if (reached[x][y]) {
				startX = x;
				startY = y;
				goto found;
			}
		}
	}
	assert(false);  // nothing reached?
	found:

	m_path = Path();
	int dxs[] = {0, 1, 0, -1};
	int dys[] = {-1, 0, 1, 0};
	int cxs[] = {-1, -1, 0, 0};
	int cys[] = {0, -1, -1, 0};
	Coordinate p(startX, startY);
	m_path->addPoint(Coordinate{p.m_x - 1, p.m_y - 1});
	int direction = 0;
	do {
		// try left, straight, right, U-turn
		assert(reached[p.m_x][p.m_y]);
		for (int i = direction + 3; i <= direction + 6; i++) {
			int dx = dxs[i % 4];
			int dy = dys[i % 4];
			if (i > direction + 3) {
				m_path->addPoint(Coordinate{p.m_x + cxs[i % 4], p.m_y + cys[i % 4]});
			}
			if (reached[p.m_x + dx][p.m_y + dy]) {
				p.m_x += dx;
				p.m_y += dy;
				direction = i % 4;
				break;
			}
		}
	} while (p.m_x != startX || p.m_y != startY);
	m_path->addPoint(Coordinate{startX - 1, startY - 1});
}

std::optional<Path> BoundaryCreator::getPath() const {
	return m_path;
}
