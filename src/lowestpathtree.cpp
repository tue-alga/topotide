#include "lowestpathtree.h"

#include <algorithm>

#include "unionfind.h"

LowestPathTree::LowestPathTree(CarvedComplex* msc, CarvedComplex::Vertex source,
                               CarvedComplex::Vertex sink, Units units) :
                msc(msc), m_source(source), m_sink(sink) {

	// collect all saddles reachable from the source
	std::vector<CarvedComplex::Vertex> saddles;
	int vertexCount = 0;
	source.data().temporaryId = vertexCount;
	source.data().lptDirections.directionToSource = -1;
	source.data().lptDirections.directionToSink = -1;
	vertexCount++;
	source.forAllOutgoingEdges([](CarvedComplex::HalfEdge e) {
		e.data().inLpt = false;
	});
	source.forAllReachableVertices(
	            [&saddles, &vertexCount]
	            (CarvedComplex::Vertex v, CarvedComplex::HalfEdge e) {
		if (v.data().msVertex.data().type == VertexType::saddle) {
			saddles.push_back(v);
		}
		v.data().temporaryId = vertexCount;
		v.data().lptDirections.directionToSource = -1;
		v.data().lptDirections.directionToSink = -1;
		vertexCount++;
		v.forAllOutgoingEdges([](CarvedComplex::HalfEdge e) {
			e.data().inLpt = false;
		});
	});

	// sort them from low to high
	std::sort(saddles.begin(), saddles.end(), [](
	          CarvedComplex::Vertex v1, CarvedComplex::Vertex v2) {
		return v1.data().msVertex.data().p < v2.data().msVertex.data().p;
	});

	// create union-find structure for detecting cycles
	UnionFind uf(vertexCount);

	// for all these saddles:
	for (auto s : saddles) {
		// for all neighbors (from low to high):
		std::vector<CarvedComplex::HalfEdge> neighbors;
		s.forAllOutgoingEdges([&neighbors](CarvedComplex::HalfEdge e) {
			neighbors.push_back(e);
		});
		std::sort(neighbors.begin(), neighbors.end(), [&](
		          CarvedComplex::HalfEdge e1, CarvedComplex::HalfEdge e2) {
			Point p1 = e1.destination().data().msVertex.data().p;
			Point p2 = e2.destination().data().msVertex.data().p;
			double e1Steepness = p1.h / units.length(p1, s.data().msVertex.data().p);
			double e2Steepness = p2.h / units.length(p2, s.data().msVertex.data().p);
			if (e1Steepness != e2Steepness) {
				return e1Steepness < e2Steepness;
			}
			return p1.h < p2.h;
		});

		for (auto e : neighbors) {
			// if this does not include a cycle:
			if (uf.findSet(e.origin().data().temporaryId) !=
			                uf.findSet(e.destination().data().temporaryId)) {
				// include the edge to the neighbor in the lowest-path tree
				e.data().inLpt = true;
				e.twin().data().inLpt = true;
				uf.merge(e.origin().data().temporaryId,
				         e.destination().data().temporaryId);
			}
		}
	}

	// BFS to set pointers
	source.forAllReachableVertices([](CarvedComplex::HalfEdge e) -> bool {
		return e.data().inLpt;
	}, [](CarvedComplex::Vertex v, CarvedComplex::HalfEdge e) {
		v.data().lptDirections.directionToSource = e.twin().id();
	});

	sink.forAllReachableVertices([](CarvedComplex::HalfEdge e) -> bool {
		return e.data().inLpt;
	}, [](CarvedComplex::Vertex v, CarvedComplex::HalfEdge e) {
		v.data().lptDirections.directionToSink = e.twin().id();
	});
}

CarvedComplex::Path LowestPathTree::lowestPathToSource(
        CarvedComplex::Vertex from) {

#ifndef DISABLE_SLOW_ASSERTS
    assert(from.isReachable(m_source));
#endif

	CarvedComplex::Path result;

	CarvedComplex::Vertex v = from;
	while (v.data().lptDirections.directionToSource != -1) {
		CarvedComplex::HalfEdge e =
		        msc->halfEdge(v.data().lptDirections.directionToSource);
		result.addEdge(e);
		v = e.destination();
	}

	assert(v == m_source);

	return result;
}

CarvedComplex::Path LowestPathTree::lowestPathToSink(
        CarvedComplex::Vertex from) {

#ifndef DISABLE_SLOW_ASSERTS
    assert(from.isReachable(m_source));
#endif

	CarvedComplex::Path result;

	CarvedComplex::Vertex v = from;
	while (v.data().lptDirections.directionToSink != -1) {
		CarvedComplex::HalfEdge e =
		        msc->halfEdge(v.data().lptDirections.directionToSink);
		result.addEdge(e);
		v = e.destination();
	}

	assert(v == m_sink);

	return result;
}
