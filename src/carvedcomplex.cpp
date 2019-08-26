#include "carvedcomplex.h"

CarvedComplex::CarvedComplex() = default;

CarvedComplex::CarvedComplex(MsComplex& msc) {

	// copy vertices
	for (int i = 0; i < msc.vertexCount(); i++) {
		MsComplex::Vertex v = msc.vertex(i);
		m_vertices.emplace_back();
		m_vertices[i].m_outgoing = v.outgoing().id();
		m_vertices[i].m_data.msVertex = v;
	}

	// copy half-edges
	for (int i = 0; i < msc.halfEdgeCount(); i++) {
		MsComplex::HalfEdge e = msc.halfEdge(i);
		m_halfEdges.emplace_back();
		m_halfEdges[i].m_next = e.next().id();
		m_halfEdges[i].m_previous = e.previous().id();
		m_halfEdges[i].m_twin = e.twin().id();
		m_halfEdges[i].m_origin = e.origin().id();
		m_halfEdges[i].m_incidentFace = e.incidentFace().id();
		m_halfEdges[i].m_data.msHalfEdge = e;
	}

	// copy faces
	for (int i = 0; i < msc.faceCount(); i++) {
		MsComplex::Face f = msc.face(i);
		m_faces.emplace_back();
		m_faces[i].m_boundary = f.boundary().id();
		m_faces[i].m_data.persistence = f.data().persistence;
		m_faces[i].m_data.lowestPathVertex = f.data().lowestPathVertex;
	}
}

void CarvedComplex::carveEdges(
        std::vector<CarvedComplex::HalfEdge> edges,
        CarvedComplex::Wedge start,
        CarvedComplex::Wedge end,
        const std::function<void(CarvedComplex::Vertex v,
                CarvedComplex::Vertex splitV)>& onSplit) {

	assert(start.isInitialized());
	assert(end.isInitialized());

	assert(start.face() == end.face());
	Face outerFace = start.face();

	// mark all vertices as closed, except for start and end
	start.vertex().forAllReachableVertices([](Vertex v, HalfEdge e) {
		v.data().openWedge = -1;
	});
	start.vertex().data().openWedge = start.outgoingHalfEdge().id();
	end.vertex().data().openWedge = end.outgoingHalfEdge().id();

	for (auto edge : edges) {
		carveEdge(edge, outerFace, onSplit);
	}
}

void CarvedComplex::carveEdge(
        CarvedComplex::HalfEdge edge,
        CarvedComplex::Face outerFace,
        const std::function<void(CarvedComplex::Vertex v,
                CarvedComplex::Vertex splitV)>& onSplit) {

	Vertex origin = edge.origin();
	Vertex destination = edge.destination();

	bool originIsOpen = origin.data().openWedge != -1;
	bool destinationIsOpen = destination.data().openWedge != -1;

	/*output(std::cout);
	std::cout << "Splitting edge "
			  << edge.id() << ": "
			  << origin.id()
			  << (openWedges[origin.id()].isInitialized() ?
					 " (open)" : " (closed)")
			  << " -> "
			  << destination.id()
			  << (openWedges[destination.id()].isInitialized() ?
					 " (open)" : " (closed)")
			  << std::endl;*/

	edge.split(outerFace);
	Wedge originWedge = wedge(edge.twin().next());
	Wedge destinationWedge = wedge(edge.twin());

	if (originIsOpen) {
		Wedge openWedge = wedge(halfEdge(origin.data().openWedge));
		HalfEdge in = openWedge.incomingHalfEdge();
		HalfEdge out = openWedge.outgoingHalfEdge();
		Vertex splitOrigin = origin.split(
		                         originWedge,
		                         wedge(halfEdge(origin.data().openWedge)));
		origin.data().openWedge = in.next().id();
		splitOrigin.data().openWedge = out.id();
		if (onSplit != nullptr) {
			onSplit(origin, splitOrigin);
		}
	} else {
		origin.data().openWedge = originWedge.outgoingHalfEdge().id();
	}

	if (destinationIsOpen) {
		Wedge openWedge = wedge(halfEdge(destination.data().openWedge));
		HalfEdge in = openWedge.incomingHalfEdge();
		HalfEdge out = openWedge.outgoingHalfEdge();
		Vertex splitDestination = destination.split(
		                         destinationWedge,
		                         wedge(halfEdge(destination.data().openWedge)));
		destination.data().openWedge = in.next().id();
		splitDestination.data().openWedge = out.id();
		if (onSplit != nullptr) {
			onSplit(destination, splitDestination);
		}
	} else {
		destination.data().openWedge = destinationWedge.outgoingHalfEdge().id();
	}
}
