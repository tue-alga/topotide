#include "gradientfieldsimplifier.h"

#include <algorithm>

GradientFieldSimplifier::GradientFieldSimplifier(std::shared_ptr<InputDcel>& dcel,
                                                 const std::shared_ptr<MsComplex>& msComplex,
                                                 double delta,
                                                 std::function<void(int)> progressListener)
    : m_dcel(dcel), m_msComplex(msComplex), m_delta(delta), m_progressListener(progressListener) {}

void
GradientFieldSimplifier::signalProgress(int progress) {
	if (m_progressListener != nullptr) {
		m_progressListener(progress);
	}
}

void
GradientFieldSimplifier::simplify() {
	std::cout << "Simplifying gradient field" << std::endl;

	// Sort the saddles on height and process them in turn from high to low.
	std::vector<MsComplex::Vertex> saddles;
	for (int i = 0; i < m_msComplex->vertexCount(); i++) {
		MsComplex::Vertex v = m_msComplex->vertex(i);
		if (v.data().type == VertexType::saddle) {
			saddles.push_back(v);
		}
	}
	std::sort(saddles.begin(), saddles.end(),
	          [](MsComplex::Vertex& v1, MsComplex::Vertex& v2) -> bool {
		return v1.data().p < v2.data().p;
	});

	for (int i = saddles.size() - 1; i >= 0; i--) {
		MsComplex::Vertex v = saddles[i];

		assert(v.data().m_heaviestSide != -1);
		double delta = v.outgoing().data().m_delta;

		// If the saddle's δ is too small, then we need to eliminate the saddle
		// by pairing it to the face in the direction of the light maximum.
		if (delta < m_delta) {
			InputDcel::HalfEdge s = m_dcel->halfEdge(v.data().m_heaviestSide);
			InputDcel::Face heavyMaximum = findMaximumFromSaddle(s);
			InputDcel::Face lightMaximum = findMaximumFromSaddle(s.twin());

			// If this is a non-splitting saddle, don't do anything.
			if ((!heavyMaximum.isInitialized() && !lightMaximum.isInitialized()) ||
			    (heavyMaximum.isInitialized() && lightMaximum.isInitialized() &&
			     heavyMaximum == lightMaximum)) {
				continue;
			}

			std::cout << "Simplifying saddle " << v.data().p << ", delta "
						<< v.outgoing().data().m_delta << ", heaviest side "
						<< m_dcel->halfEdge(v.data().m_heaviestSide).origin().data().p << " -> "
						<< m_dcel->halfEdge(v.data().m_heaviestSide).destination().data().p
						<< std::endl;

			// Traverse edge-face pairs, swapping them one by one until we reach
			// a maximum.
			while (s.twin().incidentFace().data().pairedWithEdge != -1) {
				std::cerr << "s: " << s.origin().data().p << " -> " << s.destination().data().p
							<< std::endl;
				if (s.twin().incidentFace() == m_dcel->outerFace()) {
					std::cerr << "reached outer face! stopping" << std::endl;
					break;
				}
				InputDcel::HalfEdge nextS =
					m_dcel->halfEdge(s.twin().incidentFace().data().pairedWithEdge);
				std::cerr << "nextS: " << nextS.origin().data().p << " -> "
							<< nextS.destination().data().p << std::endl;
				std::cerr << "paired? " << nextS.data().pairedWithFace << std::endl;
				if (!nextS.data().pairedWithFace) {
					std::cerr << "OEPS" << std::endl;
					return;
				}
				m_dcel->unpair(nextS, s.twin().incidentFace());
				m_dcel->pair(s.twin(), s.twin().incidentFace());
				s = nextS;
			}
			if (s.twin().incidentFace() != m_dcel->outerFace()) {
				m_dcel->pair(s.twin(), s.twin().incidentFace());
			}
		}
	}
}

InputDcel::Face GradientFieldSimplifier::findMaximumFromSaddle(InputDcel::HalfEdge s) {
	while (s.incidentFace() != m_dcel->outerFace() && !m_dcel->isCritical(s.incidentFace())) {
		s = m_dcel->halfEdge(s.incidentFace().data().pairedWithEdge).twin();
	}
	if (s.incidentFace() == m_dcel->outerFace()) {
		return {};
	} else {
		return s.incidentFace();
	}
}
