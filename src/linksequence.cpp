#include "linksequence.h"

#include <algorithm>
#include <cassert>
#include <iostream>

LinkSequence::LinkSequence(const NetworkGraph& graph) {

	// mark all vertices that we have seen so far
	std::vector<bool> visitedVertex(graph.vertexCount(), false);
	visitedVertex[0] = true;  // the source
	visitedVertex[1] = true;  // the sink

	std::vector<bool> visitedEdge(graph.edgeCount(), false);

	std::vector<NetworkGraph::Edge> edges;
	for (int i = 0; i < graph.edgeCount(); i++) {
		edges.push_back(graph.edge(i));
	}
	std::sort(edges.begin(), edges.end(),
	          [](NetworkGraph::Edge& e1, NetworkGraph::Edge& e2) {
		return e1.delta > e2.delta;
	});

	for (NetworkGraph::Edge& e : edges) {
		if (visitedEdge[e.id]) {
			continue;
		}
		if (!visitedVertex[e.from] && !visitedVertex[e.to]) {
			continue;  // TODO: if there is more than one edge with the same δ-value, this can drop one of them. So if this happens we need to handle it separately
		}

		// found the begin point of a new link
		int vId = visitedVertex[e.from] ? e.from : e.to;
		NetworkGraph::Vertex v = graph[vId];
		Link link;
		link.delta = e.delta;
		link.path.push_back(v.p);
		bool end = false;
		while (!end) {
			end = true;
			for (int incidentEdgeId : v.incidentEdges) {
				NetworkGraph::Edge incidentEdge = graph.edge(incidentEdgeId);
				if (!visitedEdge[incidentEdge.id]
				        && incidentEdge.delta == e.delta) {
					visitedEdge[incidentEdge.id] = true;
					end = false;
					appendEdgeToLink(link, graph, incidentEdge);
					vId = otherEndOf(incidentEdge, vId);
					visitedVertex[vId] = true;
					v = graph[vId];
					e = incidentEdge;
					break;
				}
			}
		}

		m_links.push_back(link);
	}
}

int LinkSequence::linkCount() {
	return m_links.size();
}

LinkSequence::Link& LinkSequence::link(int id) {
	return m_links[id];
}

void LinkSequence::appendEdgeToLink(Link& link,
                                    const NetworkGraph& graph,
                                    const NetworkGraph::Edge& e) {
	assert(link.path.size() > 0);

	// should we append the path non-reversed or reversed?
	Point lastOfLink = link.path[link.path.size() - 1];
	if (lastOfLink == graph[e.from].p) {
		// non-reversed
		for (int i = 1; i < e.path.size(); i++) {
			link.path.push_back(e.path[i]);
		}
	} else {
		// reversed
		for (int i = e.path.size() - 2; i >= 0; i--) {
			link.path.push_back(e.path[i]);
		}
	}
}

int LinkSequence::otherEndOf(const NetworkGraph::Edge& e, int oneEnd) {
	assert(oneEnd == e.from || oneEnd == e.to);
	return oneEnd == e.from ? e.to : e.from;
}
