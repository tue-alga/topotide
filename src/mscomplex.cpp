#include "mscomplex.h"

InputDcel::Path
MsComplex::dcelPath(MsComplex::HalfEdge e) {
	if (e.origin().data().type == VertexType::minimum) {
		return e.twin().data().m_dcelPath.reversed();
	} else {
		return e.data().m_dcelPath;
	}
}

MsComplex::MsComplex() = default;
