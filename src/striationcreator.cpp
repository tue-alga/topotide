#include "striationcreator.h"

#include <ctime>
#include <stack>

#include "lowestpathtree.h"

StriationCreator::StriationCreator(MsComplex& msc, Striation* striation,
                                   Units units,
                                   bool alongLowestPath,
                                   std::function<void(int)> progressListener) :
        msc(msc), striation(striation), m_units(units),
        m_alongLowestPath(alongLowestPath),
        progressListener(progressListener) {
}

void StriationCreator::create() {

	signalProgress(0);

	// create a copy of the MS complex
	cc = CarvedComplex(msc);

	// find outer face
	outerFace = findOuterFace();
	assert(outerFace.isInitialized());

	// maintain number of carves done for progress information
	int carvedMaximumCount = 0;
	int maximumCount = cc.faceCount();

	std::stack<StriationPiece> pieces;
	pieces.push(StriationPiece(findSourceWedge(),
	                           findSinkWedge(),
	                           -1,
	                           true));

	while (!pieces.empty()) {

		// take source s and sink t from the queue
		StriationPiece piece = pieces.top();
		pieces.pop();
		CarvedComplex::Wedge s = piece.source;
		CarvedComplex::Wedge t = piece.sink;

#ifndef DISABLE_SLOW_ASSERTS
		assert(s.vertex().isReachable(t.vertex()));
#endif

		// search set S of canditate faces
		std::unordered_set<int> candidateFaces;
		if (m_alongLowestPath) {
			candidateFaces = findFacesAlongLowestPath(s.vertex(), t.vertex());
		} else {
			candidateFaces = findReachableFaceIds(s.vertex());
		}

		assert(!candidateFaces.empty());

		if (candidateFaces.size() == 1) {
			continue;
		}

		// find MS face in S with the highest persistence
		CarvedComplex::Face highestFace;
		for (auto i : candidateFaces) {
			CarvedComplex::Face f = cc.face(i);

			// ignore the outer face
			if (f.data().persistence ==
			        std::numeric_limits<double>::infinity()) {
				continue;
			}

			if (!highestFace.isInitialized() ||
			        f.data().persistence > highestFace.data().persistence) {
				highestFace = f;
			}
		}
		assert(highestFace.isInitialized());

		// compute lowest-path tree with source s
		CarveResult carve = carveAround(highestFace, s, t);

		// compute carve paths
		CarvedComplex::HalfEdge e;

		std::vector<int> topCarvePath;
		e = carve.topSourceWedge.outgoingHalfEdge();
		while (e.origin() != carve.topSinkWedge.vertex()) {
			topCarvePath.push_back(e.twin().data().msHalfEdge.id());
			e = e.next();
		}
		std::vector<int> bottomCarvePath;
		e = carve.bottomSourceWedge.incomingHalfEdge();
		while (e.destination() != carve.bottomSinkWedge.vertex()) {
			bottomCarvePath.push_back(e.twin().data().msHalfEdge.id());
			e = e.previous();
		}

		// compute vertices on face boundary
		int prefix = 0;
		while (prefix < topCarvePath.size() &&
		       prefix < bottomCarvePath.size() &&
		       cc.halfEdge(topCarvePath[prefix]).twin().data().msHalfEdge.id() ==
		               bottomCarvePath[prefix]) {
			prefix++;
		}
		int suffix = 0;
		while (suffix < topCarvePath.size() &&
		       suffix < bottomCarvePath.size() &&
		       cc.halfEdge(topCarvePath[topCarvePath.size() - suffix - 1]).
		                twin().data().msHalfEdge.id() ==
		       bottomCarvePath[bottomCarvePath.size() - suffix - 1]) {
			suffix++;
		}

		std::vector<int> topVertices;
		if (prefix == topCarvePath.size()) {
			topVertices.push_back(cc.halfEdge(
			          topCarvePath[prefix - 1]).origin().data().msVertex.id());
		} else {
			topVertices.push_back(cc.halfEdge(
			          topCarvePath[prefix]).destination().data().msVertex.id());
		}
		for (int i = prefix;
		     i <= static_cast<int>(topCarvePath.size()) - suffix - 1;
		     i++) {
			topVertices.push_back(cc.halfEdge(
			          topCarvePath[i]).origin().data().msVertex.id());
		}
		std::vector<int> bottomVertices;
		if (prefix == 0) {
			bottomVertices.push_back(cc.halfEdge(
			          bottomCarvePath[prefix]).origin().data().msVertex.id());
		} else {
			bottomVertices.push_back(cc.halfEdge(
			          bottomCarvePath[prefix - 1]).destination().data().
			                            msVertex.id());
		}
		for (int i = prefix;
		     i <= static_cast<int>(bottomCarvePath.size()) - suffix - 1;
		     i++) {
			bottomVertices.push_back(cc.halfEdge(
			          bottomCarvePath[i]).destination().data().msVertex.id());
		}

		// put middle source into striation
		int item = striation->addItem(
		               highestFace.id(), topCarvePath, bottomCarvePath,
		               topVertices, bottomVertices);
		if (piece.parent != -1) {
			Striation::Item& parent = striation->item(piece.parent);
			if (piece.isTop) {
				parent.m_topItem = item;
			} else {
				parent.m_bottomItem = item;
			}
		}

		// put top and bottom source / sink into queue
		pieces.push(StriationPiece(carve.topSourceWedge,
		                           carve.topSinkWedge,
		                           item,
		                           true));
		pieces.push(StriationPiece(carve.bottomSourceWedge,
		                           carve.bottomSinkWedge,
		                           item,
		                           false));

		// send progress information
		carvedMaximumCount++;
		signalProgress(100 * carvedMaximumCount / maximumCount);
	}

	signalProgress(100);
}

CarvedComplex::Wedge StriationCreator::findSourceWedge() {
	for (int i = 0; i < cc.vertexCount(); i++) {
		CarvedComplex::Vertex v = cc.vertex(i);
		if (v.data().msVertex.data().inputDcelVertex.id() == 0) {  // 0 = source vertex
			return cc.wedge(v, outerFace);
		}
	}
	return {};
}

CarvedComplex::Wedge StriationCreator::findSinkWedge() {
	for (int i = 0; i < cc.vertexCount(); i++) {
		CarvedComplex::Vertex v = cc.vertex(i);
		if (v.data().msVertex.data().inputDcelVertex.id() == 1) {  // 1 = sink vertex
			return cc.wedge(v, outerFace);
		}
	}
	return {};
}

std::unordered_set<int>
StriationCreator::findReachableFaceIds(CarvedComplex::Vertex v) {

	std::unordered_set<int> result;

	v.forAllReachableVertices([&result](CarvedComplex::Vertex v2,
	                          CarvedComplex::HalfEdge e) {
		v2.forAllIncidentFaces([&result](CarvedComplex::Face f) {
			result.insert(f.id());
		});
	});

	return result;
}

std::unordered_set<int>
StriationCreator::findFacesAlongLowestPath(CarvedComplex::Vertex source,
                                           CarvedComplex::Vertex sink) {
	std::unordered_set<int> result;

	LowestPathTree lpt(&cc, source, sink, m_units);
	CarvedComplex::Path lowestPath = lpt.lowestPathToSink(source);

	for (CarvedComplex::HalfEdge lpEdge : lowestPath.edges()) {
		result.insert(lpEdge.incidentFace().id());
		result.insert(lpEdge.twin().incidentFace().id());
	}

	return result;
}

StriationCreator::CarveResult StriationCreator::carveAround(
        CarvedComplex::Face f, CarvedComplex::Wedge source, CarvedComplex::Wedge sink) {

	CarveResult result;

	std::unordered_set<int> carveEdgeIds =
	        findCarveEdgeIdsAround(f, source.vertex(), sink.vertex());
	assert(!carveEdgeIds.empty());

	// find the bottom-most (first) outgoing and top-most (last) incoming
	// half-edge from the source that is in the carving path;
	// those edges stay in the bottom / top part of the MS complex after
	// carving
	CarvedComplex::HalfEdge topSourceEdge;
	CarvedComplex::HalfEdge bottomSourceEdge;
	source.vertex().forAllOutgoingEdges(source.outgoingHalfEdge(),
	                                    [&](CarvedComplex::HalfEdge e) {
		if (carveEdgeIds.find(e.id()) != carveEdgeIds.end() ||
		    carveEdgeIds.find(e.twin().id()) != carveEdgeIds.end()) {
			if (!bottomSourceEdge.isInitialized()) {
				bottomSourceEdge = e;
			}
			topSourceEdge = e.twin();
		}
	});
	assert(topSourceEdge.isInitialized());
	assert(bottomSourceEdge.isInitialized());

	// same for the sink (but note that the top-most edge is first here)
	CarvedComplex::HalfEdge topSinkEdge;
	CarvedComplex::HalfEdge bottomSinkEdge;
	sink.vertex().forAllOutgoingEdges(sink.outgoingHalfEdge(),
	                                    [&](CarvedComplex::HalfEdge e) {
		if (carveEdgeIds.find(e.id()) != carveEdgeIds.end() ||
		    carveEdgeIds.find(e.twin().id()) != carveEdgeIds.end()) {
			if (!topSinkEdge.isInitialized()) {
				topSinkEdge = e;
			}
			bottomSinkEdge = e.twin();
		}
	});
	assert(topSinkEdge.isInitialized());
	assert(bottomSinkEdge.isInitialized());

	std::vector<CarvedComplex::HalfEdge> carveEdges;
	carveEdges.reserve(carveEdgeIds.size());
	for (auto i : carveEdgeIds) {
		carveEdges.push_back(cc.halfEdge(i));
	}

	cc.carveEdges(carveEdges, source, sink,
	               [](CarvedComplex::Vertex v, CarvedComplex::Vertex vSplit) {
		vSplit.forAllOutgoingEdges([v, vSplit](CarvedComplex::HalfEdge e) {
			CarvedComplex::Face f = e.incidentFace();
			if (f.data().lowestPathVertex == v.id()) {
				f.data().lowestPathVertex = vSplit.id();
			}
		});
	});

	result.topSourceWedge =
	        cc.wedge(topSourceEdge.destination(), outerFace);
	result.bottomSourceWedge =
	        cc.wedge(bottomSourceEdge.origin(), outerFace);
	result.topSinkWedge =
	        cc.wedge(topSinkEdge.origin(), outerFace);
	result.bottomSinkWedge =
	        cc.wedge(bottomSinkEdge.destination(), outerFace);

	return result;
}

std::unordered_set<int> StriationCreator::findCarveEdgeIdsAround(
        CarvedComplex::Face f,
        CarvedComplex::Vertex source,
        CarvedComplex::Vertex sink) {

	LowestPathTree lpt(&cc, source, sink, m_units);
	CarvedComplex::Path pathToSource =
	        lpt.lowestPathToSource(cc.vertex(f.data().lowestPathVertex));
	CarvedComplex::Path pathToSink =
	        lpt.lowestPathToSink(cc.vertex(f.data().lowestPathVertex));

	std::unordered_set<int> carveEdgeIds;
	for (int i = pathToSource.edges().size() - 1; i >= 0; i--) {
		CarvedComplex::HalfEdge e = pathToSource.edges()[i];
		if (e.destination().incidentToFace(f)) {
			break;
		}
		insertEdge(carveEdgeIds, e);
	}
	for (int i = pathToSink.edges().size() - 1; i >= 0; i--) {
		CarvedComplex::HalfEdge e = pathToSink.edges()[i];
		if (e.destination().incidentToFace(f)) {
			break;
		}
		insertEdge(carveEdgeIds, e);
	}
	f.forAllBoundaryEdges([&](CarvedComplex::HalfEdge e) {
		insertEdge(carveEdgeIds, e);
	});

	return carveEdgeIds;
}

void StriationCreator::insertEdge(std::unordered_set<int>& edgeSet,
                                  CarvedComplex::HalfEdge e) {
	int id = e.id();
	int twinId = e.twin().id();
	int smallestId = std::min(id, twinId);
	edgeSet.insert(smallestId);
}

CarvedComplex::Face StriationCreator::findOuterFace() {
	for (int i = 0; i < cc.faceCount(); i++) {
		CarvedComplex::Face f = cc.face(i);
		if (f.data().persistence ==
		        std::numeric_limits<double>::infinity()) {
			return f;
		}
	}

	return {};
}

void StriationCreator::signalProgress(int progress) {
	if (progressListener != nullptr) {
		progressListener(progress);
	}
}
