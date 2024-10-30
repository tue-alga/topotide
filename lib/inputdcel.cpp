#include "inputdcel.h"

InputDcel::InputDcel() = default;

InputDcel::InputDcel(const InputGraph& g) {

	// add vertices
	for (int i = 0; i < g.vertexCount(); i++) {
		auto v = addVertex();
		assert(v.id() == i);
		v.data().p = g[i].p;
	}

	// stores for each vertex the outgoing half-edges, in order of adj
	// heAdj[v][i] stores the half-edge corresponding to g[v].adj[i]
	std::vector<std::vector<int>> heAdj;
	heAdj.reserve(g.vertexCount());
	for (int v = 0; v < g.vertexCount(); v++) {
		heAdj.emplace_back(g[v].adj.size(), -1);
	}

	// for each vertex, create its outgoing edges
	for (int v = 0; v < g.vertexCount(); v++) {
		for (int i = 0; i < g[v].adj.size(); i++) {
			auto a = g[v].adj[i];

			if (a.from < a.to) { // only in one direction
				auto e = addEdge(vertex(a.from), vertex(a.to));
				heAdj[v][i] = e.id();

				// find v in g[neighbor].adj
				int backIndex = std::distance(g[a.to].adj.begin(),
				        std::find(g[a.to].adj.begin(), g[a.to].adj.end(),
				        InputGraph::Adjacency(a.to, a.from,
				                              a.disambiguation)));
				heAdj[a.to][backIndex] = e.id() + 1;
			}
		}
	}

	// set outgoing, next and previous pointers
	for (int i = 0; i < vertexCount(); i++) {
		auto v = vertex(i);

		if (heAdj[i].size() > 0) {
			v.setOutgoing(halfEdge(heAdj[i][0]));
		}
		for (int j = 0; j < g[i].adj.size(); j++) {
			halfEdge(heAdj[i][j]).twin().setNext(halfEdge(heAdj[i][
												 (j + 1) % heAdj[i].size()]));
		}
	}

	addFaces();

	// set highest-of-face marks
	for (int i = 0; i < faceCount(); i++) {
		Face f = face(i);

		// first compute the half-edge with the highest origin
		InputDcel::HalfEdge highestEdge;
		f.forAllBoundaryEdges([&highestEdge](HalfEdge e) {
			if (!highestEdge.isInitialized() || e.origin().data().p > highestEdge.origin().data().p) {
				highestEdge = e;
			}
		});

		// now the highest edge on the face boundary can be either that edge,
		// or its predecessor
		if (highestEdge.previous().origin().data().p > highestEdge.destination().data().p) {
			highestEdge.previous().data().highestOfFace = true;
			highestEdge.data().secondHighestOfFace = true;
		} else {
			highestEdge.data().highestOfFace = true;
			highestEdge.previous().data().secondHighestOfFace = true;
		}
	}

	// compute gradient pairing

	// vertex-edge
	for (int i = 0; i < vertexCount(); i++) {
		Vertex v = vertex(i);
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

	// edge-face
	for (int i = 0; i < halfEdgeCount(); i++) {
		HalfEdge e = halfEdge(i);
		if (e.data().highestOfFace) {
			// find the highest vertices that are not in e, in the incident and
			// the opposite face of e, that are not in e
			Vertex incidentHighest = highestBoundaryVertexNotInEdge(e.incidentFace(), e);
			Vertex oppositeHighest = highestBoundaryVertexNotInEdge(e.oppositeFace(), e);
			if (incidentHighest.data().p < oppositeHighest.data().p) {
				Face f = e.incidentFace();
				f.data().pairedWithEdge = e.id();
				e.data().pairedWithFace = true;
				assert(!e.data().pairedWithVertex);
				assert(!e.twin().data().pairedWithVertex);
				assert(!e.twin().data().pairedWithFace);
			}
		}
	}

	// secondary edge-face pairs
	for (int i = 0; i < halfEdgeCount(); i++) {
		HalfEdge e = halfEdge(i);
		if (e.data().pairedWithVertex || e.data().pairedWithFace ||
		    e.twin().data().pairedWithVertex || e.twin().data().pairedWithFace) {
			continue;
		}
		if (e.data().secondHighestOfFace && e.incidentFace().data().pairedWithEdge == -1) {
			Vertex incidentHighest = highestBoundaryVertexNotInEdge(e.incidentFace(), e);
			Vertex oppositeHighest = highestBoundaryVertexNotInEdge(e.oppositeFace(), e);
			if (incidentHighest.data().p < oppositeHighest.data().p) {
				Face f = e.incidentFace();
				f.data().pairedWithEdge = e.id();
				e.data().pairedWithFace = true;
			}
		}
	}

	// set edge and face center coordinates
	for (size_t i = 0; i < halfEdgeCount(); i++) {
		HalfEdge e = halfEdge(i);
		e.data().p = (e.origin().data().p + e.destination().data().p) * 0.5;
	}
	for (size_t i = 0; i < faceCount(); i++) {
		Face f = face(i);
		Point sum;
		int count = 0;
		f.forAllBoundaryVertices([&sum, &count](Vertex v) {
			sum += v.data().p;
			count++;
		});
		f.data().p = sum * (1.0 / count);
	}
}

bool InputDcel::isCritical(Vertex vertex) const {
	return vertex.data().pairedWithEdge == -1;
}

bool InputDcel::isCritical(HalfEdge edge) const {
	return !edge.data().pairedWithFace && !edge.twin().data().pairedWithFace &&
	       !edge.data().pairedWithVertex && !edge.twin().data().pairedWithVertex;
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

PiecewiseCubicFunction InputDcel::volumeAboveFunction(InputDcel::Face face) {

	std::vector<Point> points;
	face.forAllBoundaryEdges([&points](HalfEdge e) {
		points.push_back(e.origin().data().p);
	});
	assert(points.size() == 3);

	// sort points on height (with SoS)
	std::sort(points.begin(), points.end());

	Point p1 = points[0];
	Point p2 = points[1];
	Point p3 = points[2];

	return PiecewiseCubicFunction(p1, p2, p3);
}

PiecewiseCubicFunction InputDcel::volumeBelowFunction(InputDcel::Face face) {

	std::vector<Point> points;
	face.forAllBoundaryEdges([&points](HalfEdge e) {
		points.push_back(e.origin().data().p);
	});
	assert(points.size() == 3);
	Point p1 = points[0];
	Point p2 = points[1];
	Point p3 = points[2];

	// the volume below function is
	// f(h) = h * area + volumeAbove(h) - volumeAbove(0)
	double area = std::abs(PiecewiseCubicFunction::area(p1, p2, p3));
	PiecewiseCubicFunction term1(CubicFunction(0, area, 0, 0));
	PiecewiseCubicFunction term2 = volumeAboveFunction(face);
	PiecewiseCubicFunction term3(CubicFunction(term2(0), 0, 0, 0));
	return term1.add(term2).subtract(term3);
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
