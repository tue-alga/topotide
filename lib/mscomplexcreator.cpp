#include <cmath>
#include <variant>

#include "inputdcel.h"
#include "mscomplexcreator.h"
#include "vertextype.h"

MsComplexCreator::MsComplexCreator(const std::shared_ptr<InputDcel>& dcel, const std::shared_ptr<MsComplex>& msc,
                                   std::function<void(int)> progressListener) :
        dcel(dcel), msc(msc), progressListener(progressListener) {
}

void MsComplexCreator::create() {

	signalProgress(0);

	// add minima and saddles as vertices
	for (int i = 0; i < dcel->vertexCount(); i++) {
		InputDcel::Vertex v = dcel->vertex(i);

		if (dcel->isCritical(v)) {
			MsComplex::Vertex newV = msc->addVertex();
			newV.data().p = v.data().p;
			newV.data().inputDcelSimplex = v;
			newV.data().type = VertexType::minimum;
			v.data().msVertex = newV.id();
		}
	}

	signalProgress(5);

	for (int i = 0; i < dcel->halfEdgeCount(); i++) {
		InputDcel::HalfEdge e = dcel->halfEdge(i);

		if (dcel->isCritical(e) && e.twin().data().msVertex == -1) {
			MsComplex::Vertex newV = msc->addVertex();
			newV.data().p = e.data().p;
			// saddles get assigned the height of their highest endpoint
			newV.data().p.h = std::max(e.origin().data().p.h, e.destination().data().p.h);
			newV.data().inputDcelSimplex = e;
			newV.data().type = VertexType::saddle;
			e.data().msVertex = newV.id();
			e.twin().data().msVertex = newV.id();
		}
	}

	signalProgress(10);

	// add minimum -> saddle half-edges
	for (int i = 0; i < msc->vertexCount(); i++) {
		MsComplex::Vertex m = msc->vertex(i);
		if (m.data().type == VertexType::minimum) {
			addEdgesFromMinimum(m);
		}
	}

	signalProgress(30);

	// add faces (automagically! :D)
	assert(msc->isValid(false));
	msc->addFaces();
	assert(msc->isValid(true));

	signalProgress(50);

	// for each face, set the triangles and the maximum
	for (int i = 0; i < msc->faceCount(); i++) {
		MsComplex::Face f = msc->face(i);
		setDcelFacesOfFace(f);
	}

	// assert that the total number of triangles is correct
#ifndef NDEBUG
	int sum = 0;
	for (int i = 0; i < msc->faceCount(); i++) {
		sum += msc->face(i).data().faces.size();
	}
	assert(sum == dcel->faceCount());
#endif

	signalProgress(80);

	// compute persistence values
	computePersistence();

	signalProgress(90);

	// compute sand functions for each face
	for (int i = 0; i < msc->faceCount(); i++) {
		MsComplex::Face f = msc->face(i);
		setSandFunctionOfFace(f);
	}

	signalProgress(100);
}

void MsComplexCreator::addEdgesFromMinimum(MsComplex::Vertex m) {

	assert(m.data().type == VertexType::minimum);
	assert(std::holds_alternative<InputDcel::Vertex>(m.data().inputDcelSimplex));
	std::vector<InputDcel::Path> order =
	    saddleOrder(std::get<InputDcel::Vertex>(m.data().inputDcelSimplex));

	// add edges
	std::vector<MsComplex::HalfEdge> addedEdges;
	for (const InputDcel::Path& path : order) {
		// the saddle
		assert(path.edges().front().data().msVertex != -1);
		MsComplex::Vertex s = msc->vertex(path.edges().front().data().msVertex);
		MsComplex::HalfEdge edge = msc->addEdge(m, s);
		addedEdges.push_back(edge);
		edge.twin().data().m_dcelPath = path;

		if (!s.outgoing().isInitialized()) {
			// this is the first MS-edge from s
			s.setOutgoing(edge.twin());
		} else {
			// this is the second MS-edge from s: set next / previous pointers around s
			MsComplex::HalfEdge other = s.outgoing();
			edge.setNext(other);
			other.twin().setNext(edge.twin());
		}
	}

	// set next / previous pointers around m
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

	if (dcel->isCritical(edge)) {
		order.push_back(dcel->gradientPath(edge));
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

	// step 1: find the maximum of f
	MsComplex::HalfEdge e = f.boundary();
	if (e.origin().data().type == VertexType::minimum) {
		e = e.next();
	}
	assert(e.origin().data().type == VertexType::saddle);
	const InputDcel::HalfEdge saddleEdge = e.data().m_dcelPath.edges()[0];
	// start from the face adjacent to the saddle edge
	InputDcel::Face face = saddleEdge.incidentFace();
	while (face.data().pairedWithEdge != -1) {
		// walk upwards following edge-face gradient pairs
		face = dcel->halfEdge(face.data().pairedWithEdge).twin().incidentFace();
	}
	// when no edge-face gradient pair exists, we found the maximum
	f.data().maximum = face;

	// step 2: from the maximum, collect all the faces of f
	f.data().faces.push_back(face);
	face.forAllReachableFaces([](InputDcel::HalfEdge e) {
		return e.twin().data().pairedWithFace;
	}, [&f](InputDcel::Face foundFace, InputDcel::HalfEdge e) {
		f.data().faces.push_back(foundFace);
	});
}

void MsComplexCreator::computePersistence() {

	/*// make list of saddles, sorted from high to low
	std::vector<MsComplex::Vertex> saddles;
	for (int i = 0; i < msc->vertexCount(); i++) {
		if (msc->vertex(i).data().type == VertexType::saddle) {
			saddles.push_back(msc->vertex(i));
		}
	}
	std::sort(saddles.begin(), saddles.end(),
	          [](MsComplex::Vertex v1, MsComplex::Vertex v2) {
		return v1.data().p > v2.data().p;
	});

	// for every saddle we are going to "merge" all faces around it
	// initialize a union-find structure to maintain what faces are merged
	UnionFind uf(msc->faceCount());

	for (auto s : saddles) {
		// set of neighboring faces, taking "merged" faces into account
		std::set<int> neighboringFaces;

		// ID of the highest-maximum face within neighboringFaces
		MsComplex::Face highestMaxFace;

		// check all neighboring faces
		s.forAllOutgoingEdges([&](MsComplex::HalfEdge e) {
			MsComplex::Face incident = e.incidentFace();
			MsComplex::Face f = msc->face(uf.findSet(incident.id()));
			InputDcel::Vertex m = f.data().maximum;
			neighboringFaces.insert(f.id());
			if (!highestMaxFace.isInitialized() ||
			        m.data().p > highestMaxFace.data().maximum.data().p) {
				highestMaxFace = f;
			}
		});
		assert(highestMaxFace.isInitialized());

		// merge all non-highest-maximum faces into the highest-maximum face
		if (neighboringFaces.size() > 1) {
			for (auto i : neighboringFaces) {
				MsComplex::Face f = msc->face(i);
				if (f != highestMaxFace) {
					uf.merge(highestMaxFace.id(), f.id());
					InputDcel::Vertex m = f.data().maximum;
					f.data().persistence = m.data().p.h - s.data().p.h;
				}
			}
		}
	}*/
}

void MsComplexCreator::setSandFunctionOfFace(MsComplex::Face f) {
	std::queue<PiecewiseCubicFunction> queue;
	for (auto face : f.data().faces) {
		face.forAllBoundaryVertices([&queue](InputDcel::Vertex v) {
			queue.push(PiecewiseCubicFunction{v.data().p});
		});
	}
	if (queue.empty()) {
		f.data().volumeAbove = PiecewiseCubicFunction();
		return;
	}

	while (queue.size() > 1) {
		PiecewiseCubicFunction f1 = queue.front();
		queue.pop();
		PiecewiseCubicFunction f2 = queue.front();
		queue.pop();
		queue.push(f1.add(f2));
	}

	f.data().volumeAbove = queue.front();
}

void MsComplexCreator::signalProgress(int progress) {
	if (progressListener != nullptr) {
		progressListener(progress);
	}
}
