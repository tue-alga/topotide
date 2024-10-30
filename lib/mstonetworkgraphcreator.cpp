#include "mstonetworkgraphcreator.h"

MsToNetworkGraphCreator::MsToNetworkGraphCreator(
        const std::shared_ptr<MsComplex>& msc,
        const std::shared_ptr<NetworkGraph>& networkGraph,
        std::function<void(int)> progressListener) :
    msc(msc),
    networkGraph(networkGraph),
    progressListener(progressListener) {
}

void
MsToNetworkGraphCreator::create() {

	signalProgress(0);

	for (int i = 0; i < msc->vertexCount(); i++) {
		MsComplex::Vertex v = msc->vertex(i);
		networkGraph->addVertex(v.data().p);
	}

	for (int i = 0; i < msc->halfEdgeCount(); i++) {
		signalProgress(100 * i / msc->halfEdgeCount());

		MsComplex::HalfEdge e = msc->halfEdge(i);

		// make sure we include edges only one time
		if (e.origin().data().type != VertexType::saddle) {
			continue;
		}

		std::vector<Point> path;
		msc->dcelPath(e).forAllVertices([&path](InputDcel::Vertex v) {
			path.push_back(v.data().p);
		});

		double delta = e.origin().data().type == VertexType::saddle ?
		                   e.data().m_delta :
		                   e.twin().data().m_delta;
		networkGraph->addEdge(e.origin().id(), e.destination().id(), path,
		                      delta);
	}

	signalProgress(100);
}

void MsToNetworkGraphCreator::signalProgress(int progress) {
	if (progressListener != nullptr) {
		progressListener(progress);
	}
}
