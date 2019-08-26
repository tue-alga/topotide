#include "networkgraphcreator.h"

#include <stack>

NetworkGraphCreator::NetworkGraphCreator(
        MsComplex& msc,
        Network& network,
        NetworkGraph* networkGraph,
        bool simplify,
        std::function<void(int)> progressListener) :
    msc(msc),
    network(network),
    networkGraph(networkGraph),
    simplify(simplify),
    marked(msc.halfEdgeCount(), false),
    progressListener(progressListener) {
}

void NetworkGraphCreator::create() {

	signalProgress(0);

	if (network.paths().empty()) {
		return;
	}

	// first mark all MS-edges that are in some network path
	for (const Network::Path& path : network.paths()) {
		for (Network::PathEdge edge : path.edges()) {
			marked[edge.edge.id()] = true;
		}
	}

	// now do a DFS over all marked edges, starting with the source,
	// skipping over degree-2 vertices
	MsComplex::Vertex source = network.paths()[0].edges()[0].edge.origin();

	// stores which MS-complex vertices have been visited already
	std::vector<bool> visited(msc.vertexCount(), false);

	// stores the mapping from MS-complex vertices to graph vertices
	std::vector<int> graphVertices(msc.vertexCount(), -1);

	MsComplex::Vertex v = source;
	visited[v.id()] = true;
	graphVertices[v.id()] = networkGraph->addVertex(v.data().p);

	std::stack<MsComplex::Vertex> stack;
	stack.push(v);

	while (!stack.empty()) {
		MsComplex::Vertex v = stack.top();
		stack.pop();
		v.forAllOutgoingEdges(
		            [&](MsComplex::HalfEdge outgoing) {
			if (!marked[outgoing.id()]) {
				return;
			}
			int vGraph = graphVertices[v.id()];
			std::vector<MsComplex::HalfEdge> edges =
			        nextInterestingVertex(outgoing);
			MsComplex::Vertex vNew = edges.back().destination();
			int vNewGraph = graphVertices[vNew.id()];
			if (vNewGraph == -1) {
				vNewGraph = networkGraph->addVertex(vNew.data().p);
				graphVertices[vNew.id()] = vNewGraph;
			}

			// construct path
			std::vector<Point> path;
			for (MsComplex::HalfEdge edge : edges) {
				InputDcel::Path dcelPath = msc.dcelPath(edge);
				dcelPath.forAllVertices([&path](InputDcel::Vertex v) {
					path.push_back(v.data().p);
				});
			}
			/*qDebug() << v.id() << v.data().p.x << v.data().p.y
					 << vNew.id() << vNew.data().p.x << vNew.data().p.y;
			qDebug() << "   " << edges.size();*/
			assert(!path.empty());
			path.push_back(vNew.data().p);

			networkGraph->addEdge(vGraph, vNewGraph, path);
			if (!visited[vNew.id()]) {
				visited[vNew.id()] = true;
				stack.push(vNew);
			}
		});
	}

	signalProgress(100);
}

std::vector<MsComplex::HalfEdge> NetworkGraphCreator::nextInterestingVertex(
        MsComplex::HalfEdge edge) {
	std::vector<MsComplex::HalfEdge> result;
	result.push_back(edge);

	while (isBoring(edge.destination())) {
		edge = otherMarkedOutgoingEdge(edge.twin());
		assert(edge.isInitialized());
		result.push_back(edge);
	}
	return result;
}

bool NetworkGraphCreator::isBoring(MsComplex::Vertex v) {

	if (!simplify) {
		return false;
	}

	std::vector<MsComplex::HalfEdge> in;
	std::vector<MsComplex::HalfEdge> out;

	v.forAllIncomingEdges([this, &in](MsComplex::HalfEdge e) {
		if (marked[e.id()]) {
			in.push_back(e);
		}
	});

	v.forAllOutgoingEdges([this, &out](MsComplex::HalfEdge e) {
		if (marked[e.id()]) {
			out.push_back(e);
		}
	});

	return (in.size() == 1 && out.size() == 1 &&
	            !marked[in[0].twin().id()]) ||
	        (in.size() == 2 && out.size() == 2 &&
	            marked[in[0].twin().id()] &&
	            marked[in[1].twin().id()]);
}

MsComplex::HalfEdge NetworkGraphCreator::otherMarkedOutgoingEdge(
        MsComplex::HalfEdge e) {
	MsComplex::HalfEdge result;
	e.origin().forAllOutgoingEdges([this, &e, &result]
	                               (MsComplex::HalfEdge other) {
		if (e != other && marked[other.id()]) {
			result = other;
		}
	});
	return result;
}

void NetworkGraphCreator::signalProgress(int progress) {
	if (progressListener != nullptr) {
		progressListener(progress);
	}
}
