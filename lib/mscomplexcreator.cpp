#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <variant>

#include "boundarystatus.h"
#include "inputdcel.h"
#include "mscomplexcreator.h"
#include "vertextype.h"

MsComplexCreator::MsComplexCreator(const std::shared_ptr<InputDcel>& dcel,
                                   const std::shared_ptr<MsComplex>& msc,
                                   std::function<void(int)> progressListener)
    : m_dcel(dcel), m_msc(msc), m_progressListener(progressListener) {}

void MsComplexCreator::create() {

	signalProgress(0);

	// Add one MS-vertex (the “boundary minimum”) that represents all permeable
	// boundary regions. The boundary minimum will be the target of all MS-edges
	// that flow through permeable boundary regions.
	MsComplex::Vertex boundaryMinimum = m_msc->addVertex();
	boundaryMinimum.data().p = {-1, -1, -std::numeric_limits<double>::infinity()};
	boundaryMinimum.data().type = VertexType::minimum;

	// Add an MS-vertex for each terrain minimum.
	for (int i = 0; i < m_dcel->vertexCount(); i++) {
		InputDcel::Vertex v = m_dcel->vertex(i);
		if (m_dcel->isCritical(v)) {
			MsComplex::Vertex newV = m_msc->addVertex();
			newV.data().p = v.data().p;
			newV.data().inputDcelSimplex = v;
			newV.data().type = VertexType::minimum;
			v.data().msVertex = newV.id();
		}
	}

	signalProgress(5);

	// Add an MS-vertex for each saddle.
	for (int i = 0; i < m_dcel->halfEdgeCount(); i++) {
		InputDcel::HalfEdge e = m_dcel->halfEdge(i);

		if (m_dcel->isCritical(e) && e.twin().data().msVertex == -1) {
			MsComplex::Vertex newV = m_msc->addVertex();
			newV.data().p = e.data().p;
			// Saddles get assigned the height of their highest endpoint.
			newV.data().p.h = std::max(e.origin().data().p.h, e.destination().data().p.h);
			newV.data().inputDcelSimplex = e;
			newV.data().type = VertexType::saddle;
			e.data().msVertex = newV.id();
			e.twin().data().msVertex = newV.id();
		}
	}

	signalProgress(10);

	// Add minimum → saddle half-edges. We do this by DFSing from each minimum,
	// following (inverse) vertex-edge pairs, so that we find reachable saddles
	// in counter-clockwise order around the minimum. (We do it this way around,
	// instead of simply walking down from saddles, as that way we wouldn't be
	// able to easily determine the counter-clockwise order of saddles. We need
	// that order to set next/previous pointers in the DCEL correctly. On the
	// other hand, next/previous pointers around saddles are trivial to set as
	// saddles have degree 2.)
	for (int i = 0; i < m_msc->vertexCount(); i++) {
		MsComplex::Vertex m = m_msc->vertex(i);
		if (m.data().type == VertexType::minimum && m != boundaryMinimum) {
			addEdgesFromMinimum(m);
		}
	}

	// Add boundary minimum → saddle half-edges. This works the same as “normal”
	// minimum → saddle half-edges, except the boundary minimum doesn't actually
	// exist in the InputDcel, so instead we walk over the boundary of the outer
	// face in counter-clockwise order and DFS from there. Again, we take care
	// to construct the correct order of saddles around the boundary minimum.
	addEdgesFromBoundaryMinimum(boundaryMinimum);

	signalProgress(30);

	// Add MS-faces.
	assert(m_msc->isValid(false));
	m_msc->addFaces();
	assert(m_msc->isValid(true));

	signalProgress(50);

	// For each MS-face, find its maximum and the set of InputDcel faces it
	// contains.
	for (int i = 0; i < m_msc->faceCount(); i++) {
		MsComplex::Face f = m_msc->face(i);
		setDcelFacesOfFace(f);
	}

	// Assert that the sum of numbers of InputDcel faces assigned to MS-faces is
	// equal to the total number of InputDcel faces (minus one: the outer face
	// is not assigned to any MS-face).
#ifndef NDEBUG
	int sum = 0;
	for (int i = 0; i < m_msc->faceCount(); i++) {
		sum += m_msc->face(i).data().faces.size();
	}
	std::cout << sum << " <-> " << m_dcel->faceCount() << std::endl;
	//assert(sum == m_dcel->faceCount() - 1);
#endif

	signalProgress(80);

	// Compute sand functions for each face.
	for (int i = 0; i < m_msc->faceCount(); i++) {
		MsComplex::Face f = m_msc->face(i);
		setSandFunctionOfFace(f);
	}

	signalProgress(100);
}

void MsComplexCreator::addEdgesFromMinimum(MsComplex::Vertex m) {
	// Find the paths from each reachable saddle around this minimum, in
	// counter-clockwise order.
	assert(m.data().type == VertexType::minimum);
	assert(std::holds_alternative<InputDcel::Vertex>(m.data().inputDcelSimplex));
	std::vector<InputDcel::Path> order =
	    saddleOrder(std::get<InputDcel::Vertex>(m.data().inputDcelSimplex));

	// Add MS-edges representing these paths.
	addEdgesFromMinimum(m, order);
}

void MsComplexCreator::addEdgesFromMinimum(MsComplex::Vertex m, std::vector<InputDcel::Path> order) {
	// Add MS-edges representing the paths in the given order.
	std::vector<MsComplex::HalfEdge> addedEdges;
	for (const InputDcel::Path& path : order) {
		// Find the MS-vertex representing the path's origin saddle.
		assert(path.edges().front().data().msVertex != -1);
		MsComplex::Vertex s = m_msc->vertex(path.edges().front().data().msVertex);

		// Create the MS-edge.
		MsComplex::HalfEdge edge = m_msc->addEdge(m, s);
		addedEdges.push_back(edge);
		edge.twin().data().m_dcelPath = path;

		// Set the DCEL pointers. If the saddle's outgoing pointer wasn't set
		// yet, then apparently this is the first MS-edge we're connecting to
		// this saddle, so we set outgoing to the newly added MS-edge. If the
		// outgoing pointer was set, then apparently this is the second MS-edge
		// we're connecting to this saddle, so we set the next/previous pointers
		// of the newly added MS-edge to the (existing) outgoing MS-edge.
		if (!s.outgoing().isInitialized()) {
			s.setOutgoing(edge.twin());
		} else {
			MsComplex::HalfEdge other = s.outgoing();
			edge.setNext(other);
			other.twin().setNext(edge.twin());
		}
	}

	// Finally, set the DCEL pointers around m based on the counter-clockwise
	// order of the added MS-edges.
	for (int i = 0; i < addedEdges.size(); i++) {
		// edge = (m -> s)
		// edge.twin() = (s -> m)
		MsComplex::HalfEdge edge = addedEdges[i];
		// nextEdge = (m -> s')
		// nextEdge.twin() = (s' -> m)
		MsComplex::HalfEdge nextEdge = addedEdges[(i + 1) % addedEdges.size()];

		if (i == 0) {
			m.setOutgoing(edge);
		}

		edge.twin().setNext(nextEdge);
	}
}

void MsComplexCreator::addEdgesFromBoundaryMinimum(MsComplex::Vertex boundaryMinimum) {
	// Walk around the outer face in counter-clockwise order, and find paths
	// from each reachable saddle, in clockwise order. All of these paths
	// together result in one consistent order of saddles around the boundary
	// minimum.
	std::vector<InputDcel::Path> order;
	m_dcel->outerFace().forAllBoundaryVertices([this, &order, &boundaryMinimum](InputDcel::Vertex v) {
		assert(v.data().boundaryStatus != BoundaryStatus::INTERIOR);
		if (v.data().boundaryStatus == BoundaryStatus::PERMEABLE) {
			std::vector<InputDcel::Path> vertexOrder = saddleOrder(v);
			// saddleOrder() returns a counter-clockwise order, so we need to
			// reverse the order to make it clockwise.
			order.insert(order.end(), vertexOrder.rbegin(), vertexOrder.rend());
		}
	});

	// Because the boundary minimum is outside the boundary itself, walking in
	// counter-clockwise order around the boundary minimum corresponds to
	// walking in clockwise order around the boundary. As the order we have is
	// in counter-clockwise order around the boundary, we need to reverse it.
	std::reverse(order.begin(), order.end());

	// Add MS-edges representing these paths.
	addEdgesFromMinimum(boundaryMinimum, order);
}

std::vector<InputDcel::Path> MsComplexCreator::saddleOrder(InputDcel::Vertex m) {
	std::vector<InputDcel::Path> order;

	InputDcel::HalfEdge edge = m.outgoing();  // arbitrary outgoing edge
	InputDcel::HalfEdge endEdge = edge;
	do {
		saddleOrderRecursive(edge, order);
		edge = edge.nextOutgoing();
	} while (edge != endEdge);

	return order;
}

void MsComplexCreator::saddleOrderRecursive(InputDcel::HalfEdge edge,
                                            std::vector<InputDcel::Path>& order) {

	edge = edge.twin();

	if (m_dcel->isCritical(edge)) {
		order.push_back(m_dcel->gradientPath(edge));
		return;
	}

	if (!edge.data().pairedWithVertex) {
		return;
	}

	InputDcel::HalfEdge endEdge = edge;
	while ((edge = edge.nextOutgoing()) != endEdge) {
		saddleOrderRecursive(edge, order);
	}
}

void MsComplexCreator::setDcelFacesOfFace(MsComplex::Face f) {

	f.data().maximum = findFaceMaximum(f);

	// From the maximum, collect all the faces of f
	if (f.data().maximum != m_dcel->outerFace()) {
		f.data().faces.push_back(f.data().maximum);
		f.data().maximum.forAllReachableFaces([](InputDcel::HalfEdge e) {
			return e.twin().data().pairedWithFace;
		}, [&f](InputDcel::Face foundFace, InputDcel::HalfEdge) {
			f.data().faces.push_back(foundFace);
		});
	}
}

InputDcel::Face MsComplexCreator::findFaceMaximum(MsComplex::Face f) {
	// Find an arbitrary saddle-to-minimum edge of the MS-face.
	MsComplex::HalfEdge e = f.boundary();
	if (e.origin().data().type == VertexType::minimum) {
		e = e.next();
	}
	assert(e.origin().data().type == VertexType::saddle);

	// Start from the InputDcel face adjacent to the saddle edge.
	const InputDcel::HalfEdge saddleEdge = e.data().m_dcelPath.edges()[0];
	InputDcel::Face face = saddleEdge.incidentFace();

	// Walk upwards following edge-face gradient pairs.
	while (face.data().pairedWithEdge != -1) {
		face = m_dcel->halfEdge(face.data().pairedWithEdge).twin().incidentFace();
	}

	// When no edge-face gradient pair exists, we found the maximum.
	return face;
}

void MsComplexCreator::setSandFunctionOfFace(MsComplex::Face f) {
	if (f.data().maximum == m_dcel->outerFace()) {
		f.data().volumeAbove =
		    PiecewiseLinearFunction(LinearFunction{std::numeric_limits<double>::infinity()});
	} else {
		std::queue<PiecewiseLinearFunction> queue;
		for (auto face : f.data().faces) {
			face.forAllBoundaryVertices([&queue](InputDcel::Vertex v) {
				queue.push(PiecewiseLinearFunction{v.data().p});
			});
		}
		if (queue.empty()) {
			f.data().volumeAbove = PiecewiseLinearFunction();
			return;
		}

		while (queue.size() > 1) {
			PiecewiseLinearFunction f1 = queue.front();
			queue.pop();
			PiecewiseLinearFunction f2 = queue.front();
			queue.pop();
			queue.push(f1.add(f2));
		}

		f.data().volumeAbove = queue.front();
	}
}

void MsComplexCreator::signalProgress(int progress) {
	if (m_progressListener != nullptr) {
		m_progressListener(progress);
	}
}
