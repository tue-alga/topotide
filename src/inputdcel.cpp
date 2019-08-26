#include "inputdcel.h"

InputDcel::InputDcel() = default;

InputDcel::InputDcel(const InputGraph& g) {

	// add vertices
	for (int i = 0; i < g.vertexCount(); i++) {
		auto v = addVertex();
		assert(v.id() == i);
		v.data().p = g[i].p;
		v.data().type = g.vertexType(i);
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

	// set outgoing, next and previous pointers and steepest-descent marks
	for (int i = 0; i < vertexCount(); i++) {
		auto v = vertex(i);

		if (heAdj[i].size() > 0) {
			v.setOutgoing(halfEdge(heAdj[i][0]));
		}
		for (int j = 0; j < g[i].adj.size(); j++) {
			halfEdge(heAdj[i][j]).twin().setNext(halfEdge(heAdj[i][
												 (j + 1) % heAdj[i].size()]));
		}

		auto wedges = g.getWedges(i).descending;
		for (auto wedge : wedges) {
			halfEdge(heAdj[i][wedge]).data().wedgeSteepest = true;
		}

		int steepestDescent = g.steepestDescentFrom(i);
		if (steepestDescent != -1) {
			v.data().steepestDescentEdge = heAdj[i][steepestDescent];
			halfEdge(heAdj[i][steepestDescent]).data().steepest = true;
		}
	}

	addFaces();
}

void InputDcel::splitMonkeySaddles() {

	for (int i = 0; i < vertexCount(); i++) {
		Vertex v = vertex(i);

		if (v.data().type != VertexType::saddle) {
			continue;
		}

		assert(v.data().steepestDescentEdge != -1);
		HalfEdge startEdge = halfEdge(v.data().steepestDescentEdge);
		HalfEdge edge = startEdge;

		while (isDescending(edge)) {
			edge = edge.nextOutgoing();
		}
		// now edge is the first ascending edge
		while (isAscending(edge)) {
			edge = edge.nextOutgoing();
		}
		// now edge is the first descending edge again
		while (isDescending(edge) && edge != startEdge) {
			edge = edge.nextOutgoing();
		}
		if (edge == startEdge) {
			// not even a saddle!
			continue;
		}
		// now edge is the first ascending edge
		// that is, the edge we need to split along, assuming that this
		// actually is a monkey saddle...
		HalfEdge splitEdge = edge;
		while (isAscending(edge)) {
			edge = edge.nextOutgoing();
		}
		// now edge is the first descending edge again
		while (isDescending(edge) && edge != startEdge) {
			edge = edge.nextOutgoing();
		}
		if (edge == startEdge) {
			// not a monkey saddle!
			continue;
		}

		// now we are having a monkey saddle
		Vertex vs = v.split(startEdge, splitEdge);

		// update the steepest-descent pointer of vs to point to the new edge
		vs.data().steepestDescentEdge = vs.outgoing().id();

		// if the twin of our split-edge was wedge-steepest, the split caused
		// us to duplicate the wedge-steepest marker, so we remove it from the
		// original edge
		splitEdge.twin().data().wedgeSteepest = false;
	}
}

InputDcel::Path InputDcel::steepestDescentPath(HalfEdge startingEdge) {

	InputDcel::Path result;

	HalfEdge edge = startingEdge;
	result.addEdge(edge);

	while (edge.destination().data().type != VertexType::minimum) {
		edge = halfEdge(edge.destination().data().steepestDescentEdge);
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
