#include "mscomplexsimplifier.h"

#include <algorithm>

MsComplexSimplifier::MsComplexSimplifier(const std::shared_ptr<MsComplex>& msc,
                                         std::function<void(int)> progressListener) :
    msc(msc),
    mscCopy(*msc),
    progressListener(progressListener) {
}

void
MsComplexSimplifier::signalProgress(int progress) {
	if (progressListener != nullptr) {
		progressListener(progress);
	}
}

void
MsComplexSimplifier::simplify() {

	// sort saddle points on (ascending) height
	std::vector<MsComplex::Vertex> saddles;
	for (int i = 0; i < mscCopy.vertexCount(); i++) {
		MsComplex::Vertex v = mscCopy.vertex(i);
		if (v.data().type == VertexType::saddle) {
			saddles.push_back(v);
		}
	}
	std::sort(saddles.begin(), saddles.end(),
	          [](MsComplex::Vertex& v1, MsComplex::Vertex& v2) -> bool {
		return v1.data().p < v2.data().p;
	});

	// for all saddles from high to low
	for (int i = saddles.size() - 1; i >= 0; i--) {
		signalProgress(100 * (saddles.size() - i - 1) / saddles.size());

		MsComplex::Vertex saddle = saddles[i];

		std::pair<double, MsComplex::HalfEdge> significance =
		        computeSaddleSignificance(saddle);
		double delta = significance.first;
		MsComplex::HalfEdge heaviestSide = significance.second;
		msc->vertex(saddle.id()).data().m_heaviestSide =
		    heaviestSide.data().m_dcelPath.edges().front().id();

		// saddles should have degree 2
		assert(saddle.outgoing().nextOutgoing().nextOutgoing() ==
		       saddle.outgoing());

		// set edge weights
		msc->halfEdge(saddle.outgoing().id()).data().m_delta = delta;
		msc->halfEdge(saddle.outgoing().id()).twin().data().m_delta = delta;
		msc->halfEdge(saddle.outgoing().nextOutgoing().id()).
		        data().m_delta = delta;
		msc->halfEdge(saddle.outgoing().nextOutgoing().id()).
		        twin().data().m_delta = delta;

		if (saddle.outgoing().incidentFace() != saddle.outgoing().nextOutgoing().incidentFace()) {
			// actually remove saddle, and merge faces
			// add sand functions around the saddle together
			PiecewiseLinearFunction f;
			f = f.add(heaviestSide.incidentFace().data().volumeAbove);
			f = f.add(heaviestSide.nextOutgoing().incidentFace().data().volumeAbove);
			f.prune(saddle.data().p.h);

			// assign added sand function to largest face
			heaviestSide.incidentFace().data().volumeAbove = f;

			// remove the saddle and its adjacent edges, maintaining the data
			// (and sand function) of the largest face
			heaviestSide.remove();
		}
	}

	// remove all degree-1 vertices (iteratively)
	bool removedVertices;
	do {
		removedVertices = false;
		for (int i = 0; i < msc->vertexCount(); i++) {
			MsComplex::Vertex v = msc->vertex(i);
			if (v.isRemoved()) {
				continue;
			}
			if (v.data().p.h == -std::numeric_limits<double>::infinity()) {
				continue;
			}
			std::vector<MsComplex::HalfEdge> edges;
			v.forAllOutgoingEdges([&edges](MsComplex::HalfEdge e) {
				edges.push_back(e);
			});
			if (edges.size() == 1) {
				if (edges[0].data().m_delta > 0) {
					edges[0].data().m_delta = 0;
					edges[0].twin().data().m_delta = 0;
					removedVertices = true;
				}
				continue;
			}
			std::sort(edges.begin(), edges.end(),
			          [](MsComplex::HalfEdge e1, MsComplex::HalfEdge e2) {
				return e1.data().m_delta > e2.data().m_delta;
			});
			if (edges[0].data().m_delta > edges[1].data().m_delta) {
				edges[0].data().m_delta = edges[1].data().m_delta;
				edges[0].twin().data().m_delta = edges[1].data().m_delta;
				removedVertices = true;
			}
		}
	} while (removedVertices);
}

std::pair<double, MsComplex::HalfEdge>
MsComplexSimplifier::computeSaddleSignificance(MsComplex::Vertex saddle) {
	//if (saddle.data().isBoundarySaddle) {
	//	return {0, saddle.outgoing()};
	//}

	assert(saddle.isInitialized());
	assert(saddle.data().type == VertexType::saddle);

	double saddleHeight = saddle.data().p.h;
	MsComplex::HalfEdge largestFace;
	
	MsComplex::HalfEdge e1 = saddle.outgoing();
	MsComplex::HalfEdge e2 = e1.nextOutgoing();
	double volume1 = e1.incidentFace().data().volumeAbove(saddleHeight);
	if (std::isnan(volume1)) {
		volume1 = std::numeric_limits<double>::infinity();
	}
	double volume2 = e2.incidentFace().data().volumeAbove(saddleHeight);
	if (std::isnan(volume2)) {
		volume2 = std::numeric_limits<double>::infinity();
	}

	if (volume1 > volume2) {
		return {volume2, e1};
	} else {
		return {volume1, e2};
	}
}
