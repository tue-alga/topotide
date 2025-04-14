#include "inputdcel.h"

#include <algorithm>
#include <limits>

#include <algorithm>
#include <limits>

void InputDcelVertex::output(std::ostream& out) {
	out << p;
}

InputDcel::InputDcel() = default;

InputDcel::InputDcel(const InputGraph& g) {

	// Add vertices for each vertex in the graph.
	for (int i = 0; i < g.vertexCount(); i++) {
		auto v = addVertex();
		assert(v.id() == i);
		v.data().p = g[i].p;
		v.data().boundaryStatus = g[i].boundaryStatus;
		v.data().permeableRegion = g[i].permeableRegion;
	}

	// Stores for each vertex the outgoing half-edges, in order of adj.
	// `heAdj[v][i]` stores the half-edge corresponding to `g[v].adj[i]`.
	std::vector<std::vector<int>> heAdj;
	heAdj.reserve(g.vertexCount());
	for (int v = 0; v < g.vertexCount(); v++) {
		heAdj.emplace_back(g[v].adj.size(), -1);
	}

	// For each vertex, create its incident edges.
	for (int v = 0; v < g.vertexCount(); v++) {
		for (int i = 0; i < g[v].adj.size(); i++) {
			const InputGraph::Adjacency& a = g[v].adj[i];

			// Ensure that we generate edges only in one direction, to avoid
			// duplicates.
			if (a.from < a.to) {
				HalfEdge e = addEdge(vertex(a.from), vertex(a.to));
				e.data().boundaryStatus = a.boundaryStatus;
				e.twin().data().boundaryStatus = a.boundaryStatus;
				e.data().permeableRegion = a.permeableRegion;
				e.twin().data().permeableRegion = a.permeableRegion;
				heAdj[v][i] = e.id();

				// Find v in `g[neighbor].adj`.
				std::optional<int> backIndex = g[a.to].findAdjacencyTo(a.from);
				assert(backIndex.has_value());
				heAdj[a.to][*backIndex] = e.id() + 1;
			}
		}
	}

	// Set the outgoing, next and previous pointers.
	for (int i = 0; i < vertexCount(); i++) {
		Vertex v = vertex(i);

		if (heAdj[i].size() > 0) {
			v.setOutgoing(halfEdge(heAdj[i][0]));
		}
		for (int j = 0; j < g[i].adj.size(); j++) {
			halfEdge(heAdj[i][j]).twin().setNext(halfEdge(heAdj[i][(j + 1) % heAdj[i].size()]));
		}
	}

	addFaces();

	// Mark the outer face. We know vertex 0 of the graph is on the boundary,
	// and its adjacencies are in counter-clockwise order starting from the
	// outer face. Hence, we simply take the half-edge from vertex 0 to its
	// first adjacency in the list; its incident face has to be the outer face.
	assert(g[0].boundaryStatus != BoundaryStatus::INTERIOR);
	m_outerFaceId = halfEdge(heAdj[0][0]).incidentFace().id();

	setEdgeAndFaceCoordinates();
}

void InputDcel::setEdgeAndFaceCoordinates() {
	for (size_t i = 0; i < halfEdgeCount(); i++) {
		HalfEdge e = halfEdge(i);
		e.data().p = (e.origin().data().p + e.destination().data().p) * 0.5;
		e.data().p.h = std::max(e.origin().data().p.h, e.destination().data().p.h);
	}
	for (size_t i = 0; i < faceCount(); i++) {
		Face f = face(i);
		if (f == outerFace()) {
			f.data().p = {-1, -1, -std::numeric_limits<double>::infinity()};
		} else {
			Point sum;
			double max = -std::numeric_limits<double>::infinity();
			int count = 0;
			f.forAllBoundaryVertices([&sum, &max, &count](Vertex v) {
				sum += v.data().p;
				max = std::max(max, v.data().p.h);
				count++;
			});
			f.data().p = sum * (1.0 / count);
			f.data().p.h = max;
		}
	}
}

void InputDcel::computeGradientFlow() {

	// Vertex-edge pairings: pair each vertex with the outgoing half-edge to the
	// lowest neighbor (if it is lower than the vertex itself).
	for (int i = 0; i < vertexCount(); i++) {
		Vertex v = vertex(i);

		// Don't pair boundary vertices on a permeable region.
		if (v.data().boundaryStatus == BoundaryStatus::PERMEABLE) {
			continue;
		}

		InputDcel::HalfEdge pairedEdge;
		v.forAllOutgoingEdges([&pairedEdge](HalfEdge e) {
			if (!pairedEdge.isInitialized() ||
			    e.destination().data().p < pairedEdge.destination().data().p) {
				pairedEdge = e;
			}
		});
		if (pairedEdge.isInitialized() && pairedEdge.destination().data().p < v.data().p) {
			v.data().pairedWithEdge = pairedEdge.id();
			pairedEdge.data().pairedWithVertex = true;
		}
	}

	// Find the highest edge and the second-highest edge of each face.
	for (int i = 0; i < faceCount(); i++) {
		Face f = face(i);

		// First find the half-edge with the highest origin.
		InputDcel::HalfEdge highestEdge;
		f.forAllBoundaryEdges([&highestEdge](HalfEdge e) {
			if (!highestEdge.isInitialized() || e.origin().data().p > highestEdge.origin().data().p) {
				highestEdge = e;
			}
		});

		// Now the highest edge on the face boundary can be either that edge or
		// its predecessor, and the other one is the second-highest edge on the
		// face boundary.
		if (highestEdge.previous().origin().data().p > highestEdge.destination().data().p) {
			highestEdge.previous().data().highestOfFace = true;
			highestEdge.data().secondHighestOfFace = true;
		} else {
			highestEdge.data().highestOfFace = true;
			highestEdge.previous().data().secondHighestOfFace = true;
		}
	}

	auto highestBoundaryVertexNotInEdge = [](Face f, HalfEdge e) -> Vertex {
		Vertex result;
		f.forAllBoundaryVertices([&result, &e](Vertex v) {
			if (v == e.origin() || v == e.destination()) {
				return;
			}
			if (!result.isInitialized() ||
				v.data().p > result.data().p) {
				result = v;
			}
		});
		return result;
	};

	// Edge-face pairings: pair each edge with an incident face f if it is the
	// highest edge of f. If both incident faces satisfy this criterion, we
	// choose the lower face to pair the edge with. Here, “lower” means
	// lexicographically lower, i.e., we check the opposite edge of f and f',
	// and see which one has the lowest maximum.
	for (int i = 0; i < halfEdgeCount(); i++) {
		HalfEdge e = halfEdge(i);

		// Don't pair boundary edges on a permeable region.
		if (e.data().boundaryStatus == BoundaryStatus::PERMEABLE) {
			continue;
		}
		// Do pair boundary edges on an impermeable region, but not to the outer
		// face.
		if (e.data().boundaryStatus == BoundaryStatus::IMPERMEABLE &&
		    e.incidentFace() == outerFace()) {
			continue;
		}

		if (e.data().highestOfFace) {
			// Check if the incident face is lower than the opposite face. If
			// the opposite face is the outer face, skip the check and always
			// allow the pairing; after all, in that case we couldn't have
			// paired with the opposite face anyway.
			if (e.oppositeFace() == outerFace() ||
			    highestBoundaryVertexNotInEdge(e.incidentFace(), e).data().p <
			        highestBoundaryVertexNotInEdge(e.oppositeFace(), e).data().p) {
				Face f = e.incidentFace();
				f.data().pairedWithEdge = e.id();
				e.data().pairedWithFace = true;
				assert(!e.data().pairedWithVertex);
				assert(!e.twin().data().pairedWithVertex);
				assert(!e.twin().data().pairedWithFace);
			}
		}
	}

	// Secondary edge-face pairings: similar to ordinary edge-face pairings, but
	// now we allow each edge to pair with an incident face f if it is the
	// second-highest edge of f.
	for (int i = 0; i < halfEdgeCount(); i++) {
		HalfEdge e = halfEdge(i);

		// Don't pair boundary edges on a permeable region.
		if (e.data().boundaryStatus == BoundaryStatus::PERMEABLE) {
			continue;
		}
		// Do pair boundary edges on an impermeable region, but not to the outer
		// face.
		if (e.data().boundaryStatus == BoundaryStatus::IMPERMEABLE &&
		    e.incidentFace() == outerFace()) {
			continue;
		}

		// Explicitly check if this edge hasn't already been paired to something
		// else. (For ordinary edge-face pairings this follows from the
		// definition so we don't need to check it, but for secondary edge-face
		// pairs the explicit check is necessary to avoid potentially
		// double-pairing the edge.)
		if (e.data().pairedWithVertex || e.data().pairedWithFace ||
		    e.twin().data().pairedWithVertex || e.twin().data().pairedWithFace) {
			continue;
		}

		if (e.data().secondHighestOfFace && e.incidentFace().data().pairedWithEdge == -1) {
			// Check if the incident face is lower than the opposite face. If
			// the opposite face is the outer face, skip the check and always
			// allow the pairing; after all, in that case we couldn't have
			// paired with the opposite face anyway.
			if (e.oppositeFace() == outerFace() ||
			    highestBoundaryVertexNotInEdge(e.incidentFace(), e).data().p <
			        highestBoundaryVertexNotInEdge(e.oppositeFace(), e).data().p) {
				Face f = e.incidentFace();
				f.data().pairedWithEdge = e.id();
				e.data().pairedWithFace = true;
			}
		}
	}
}

bool InputDcel::isCritical(Vertex vertex) const {
	return vertex.data().pairedWithEdge == -1 &&
	       vertex.data().boundaryStatus != BoundaryStatus::PERMEABLE;
}

bool InputDcel::isCritical(HalfEdge edge) const {
	return !edge.data().pairedWithFace && !edge.twin().data().pairedWithFace &&
	       !edge.data().pairedWithVertex && !edge.twin().data().pairedWithVertex &&
	       edge.data().boundaryStatus != BoundaryStatus::PERMEABLE;
}

bool InputDcel::isCritical(Face face) const {
	return face.data().pairedWithEdge == -1 && face.id() != m_outerFaceId;
}

InputDcel::Path InputDcel::gradientPath(HalfEdge startingEdge) {

	InputDcel::Path result;

	HalfEdge edge = startingEdge;
	result.addEdge(edge);

	while (edge.destination().data().pairedWithEdge != -1) {
		edge = halfEdge(edge.destination().data().pairedWithEdge);
		result.addEdge(edge);
	}

	return result;
}

bool InputDcel::isDescending(HalfEdge edge) {
	return edge.origin().data().p > edge.destination().data().p;
}

bool InputDcel::isAscending(HalfEdge edge) {
	return !isDescending(edge);
}

PiecewiseLinearFunction InputDcel::volumeAbove(InputDcel::Face face) {
	PiecewiseLinearFunction result;
	face.forAllBoundaryVertices([&result](InputDcel::Vertex v) {
		result = result.add(PiecewiseLinearFunction{v.data().p});
	});
	return result;
}

InputDcel::Vertex InputDcel::vertexAt(double x, double y) {
	for (int i = 0; i < vertexCount(); i++) {
		if (vertex(i).data().p.x == x && vertex(i).data().p.y == y
		        && vertex(i).data().p.h <
		                    std::numeric_limits<double>::infinity()) {
			return vertex(i);
		}
	}
	return {};
}
void InputDcel::pair(Vertex v, HalfEdge e) {
	assert(e.origin() == v);
	assert(v.data().pairedWithEdge == -1 || v.data().pairedWithEdge == e.id());
	assert(!e.twin().data().pairedWithVertex);
	assert(!e.data().pairedWithFace);
	assert(!e.twin().data().pairedWithFace);

	v.data().pairedWithEdge = e.id();
	e.data().pairedWithVertex = true;
}

void InputDcel::pair(HalfEdge e, Face f) {
	assert(e.incidentFace() == f);
	assert(f.data().pairedWithEdge == -1 || f.data().pairedWithEdge == e.id());
	assert(!e.twin().data().pairedWithFace);
	assert(!e.data().pairedWithVertex);
	assert(!e.twin().data().pairedWithVertex);

	f.data().pairedWithEdge = e.id();
	e.data().pairedWithFace = true;
}

void InputDcel::unpair(Vertex v, HalfEdge e) {
	assert(e.origin() == v);
	assert(v.data().pairedWithEdge == e.id());
	assert(e.data().pairedWithVertex);

	v.data().pairedWithEdge = -1;
	e.data().pairedWithVertex = false;
}

void InputDcel::unpair(HalfEdge e, Face f) {
	assert(e.incidentFace() == f);
	assert(f.data().pairedWithEdge == e.id());
	assert(e.data().pairedWithFace);

	f.data().pairedWithEdge = -1;
	e.data().pairedWithFace = false;
}

bool InputDcel::isBlueLeaf(Vertex v) const {
	assert(v.isInitialized());
	int count = 0;
	v.forAllOutgoingEdges([&count](HalfEdge e) {
		if (e.data().pairedWithVertex || e.twin().data().pairedWithVertex) {
			count++;
		}
	});
	return count == 1;
}

bool InputDcel::isRedLeaf(Face f) const {
	assert(f.isInitialized());
	int count = 0;
	f.forAllBoundaryEdges([&count](HalfEdge e) {
		if (e.data().pairedWithFace || e.twin().data().pairedWithFace) {
			count++;
		}
	});
	return count == 1;
}

InputDcel::Face InputDcel::outerFace() {
	return face(m_outerFaceId);
}
