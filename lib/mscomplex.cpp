#include "mscomplex.h"
#include "vertextype.h"

void MsVertex::output(std::ostream& out) {
	out << p;
	if (type == VertexType::minimum) {
		out << " (minimum)";
		if (std::get<InputDcel::Vertex>(inputDcelSimplex).isInitialized()) {
			out << " → DCEL vertex " << std::get<InputDcel::Vertex>(inputDcelSimplex).id();
		}
	} else if (type == VertexType::saddle) {
		out << " (saddle)";
		if (std::get<InputDcel::HalfEdge>(inputDcelSimplex).isInitialized()) {
			out << " → DCEL half-edge " << std::get<InputDcel::HalfEdge>(inputDcelSimplex).id();
		}
	} else if (type == VertexType::maximum) {
		out << " (maximum)";
		if (std::get<InputDcel::Face>(inputDcelSimplex).isInitialized()) {
			out << " → DCEL face " << std::get<InputDcel::Face>(inputDcelSimplex).id();
		}
	}
	if (isBoundarySaddle) {
		out << " (boundary)";
	}
}

void MsHalfEdge::output(std::ostream& out) {
	if (m_dcelPath.length() > 1) {
		out << "path from " << m_dcelPath.edges()[0].destination().data().p;
	}
}

void MsFace::output(std::ostream& out) {
	out << "(" << faces.size() << " faces";
	if (maximum.isInitialized()) {
		out << ", maximum " << maximum.data().p;
	}
	out << ")";
}

InputDcel::Path
MsComplex::dcelPath(MsComplex::HalfEdge e) {
	if (e.origin().data().type == VertexType::minimum) {
		return e.twin().data().m_dcelPath.reversed();
	} else {
		return e.data().m_dcelPath;
	}
}

MsComplex::MsComplex() = default;
