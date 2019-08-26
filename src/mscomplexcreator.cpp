#include <set>

#include "mscomplexcreator.h"
#include "unionfind.h"

MsComplexCreator::MsComplexCreator(InputDcel& dcel, MsComplex* msc,
                                   std::function<void(int)> progressListener) :
        dcel(dcel), msc(msc), progressListener(progressListener) {
}

void MsComplexCreator::create() {

	signalProgress(0);

	// add minima and saddles as vertices
	for (int i = 0; i < dcel.vertexCount(); i++) {
		InputDcel::Vertex v = dcel.vertex(i);

		VertexType type = v.data().type;

		if (type == VertexType::minimum || type == VertexType::saddle) {
			MsComplex::Vertex newV = msc->addVertex();
			newV.data().p = v.data().p;
			newV.data().inputDcelVertex = v;
			newV.data().type = type;
			v.data().msVertex = newV.id();
		}
	}

	signalProgress(10);

	// add minimum -> saddle half-edges
	for (int i = 0; i < msc->vertexCount(); i++) {
		MsComplex::Vertex m = msc->vertex(i);
		if (m.data().type == VertexType::minimum) {
			addEdgesFromMinimum(dcel, m);
		}
	}

	signalProgress(30);

	// add saddle -> minimum edges
	for (int i = 0; i < msc->vertexCount(); i++) {
		MsComplex::Vertex s = msc->vertex(i);
		if (s.data().type == VertexType::saddle) {
			addEdgeOrderAroundSaddle(s);
		}
	}

	signalProgress(50);

	// add faces (automagically! :D)
	msc->addFaces();

	signalProgress(60);

	// for each DCEL edge on a MS edge, set the incidentMsFace
	for (int i = 0; i < msc->vertexCount(); i++) {
		MsComplex::Vertex m = msc->vertex(i);
		if (m.data().type == VertexType::minimum) {
			setDcelMsFacesAroundMinimum(m);
		}
	}

	signalProgress(70);

	// for each face, set the triangles and the maximum
	for (int i = 0; i < msc->faceCount(); i++) {
		MsComplex::Face f = msc->face(i);
		setDcelFacesOfFace(dcel, f);
	}

	// assert that the total number of triangles is correct
#ifndef NDEBUG
	int sum = 0;
	for (int i = 0; i < msc->faceCount(); i++) {
		sum += msc->face(i).data().triangles.size();
	}
	assert(sum == dcel.faceCount());
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

void MsComplexCreator::addEdgesFromMinimum(InputDcel& dcel,
                                           MsComplex::Vertex m) {

	std::vector<InputDcel::Path>
	        order = saddleOrder(dcel, m.data().inputDcelVertex);

	// add edges
	std::vector<MsComplex::HalfEdge> addedEdges;
	for (const InputDcel::Path& path : order) {
		// the outgoing edge towards the minimum
		assert(path.edges()[0].data().msHalfEdge == -1);

		/*for (InputDcel::HalfEdge e : path.edges()) {
			InputDcel::Vertex v = e.origin();
		}*/

		// the saddle
		MsComplex::Vertex s = msc->vertex(path.origin().data().msVertex);

		MsComplex::HalfEdge edge = msc->addEdge(m, s);
		addedEdges.push_back(edge);
		path.edges()[0].data().msHalfEdge = edge.twin().id();
		edge.twin().data().m_dcelPath = path;
	}

	// set next / previous pointers
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

void MsComplexCreator::addEdgeOrderAroundSaddle(MsComplex::Vertex s) {

	InputDcel::Vertex sDcel = s.data().inputDcelVertex;

	// search for initial wedge-steepest-descent edge
	InputDcel::HalfEdge startEdge = sDcel.outgoing();
	while (!startEdge.data().wedgeSteepest) {
		startEdge = startEdge.nextOutgoing();
	}
	assert(startEdge.data().msHalfEdge != -1);
	MsComplex::HalfEdge previousEdgeToMinimum =
	        msc->halfEdge(startEdge.data().msHalfEdge);
	s.setOutgoing(previousEdgeToMinimum);

	do {
		startEdge = startEdge.nextOutgoing();
	} while (!startEdge.data().wedgeSteepest);

	// iterate over all edges around sDcel and set the previous and
	// next pointers of the MS edges based on this order

	sDcel.forAllOutgoingEdges(startEdge, [this, &previousEdgeToMinimum]
	                            (InputDcel::HalfEdge edge) {
		if (edge.data().wedgeSteepest) {
			assert(edge.data().msHalfEdge != -1);
			MsComplex::HalfEdge edgeToMinimum =
			        msc->halfEdge(edge.data().msHalfEdge);
			previousEdgeToMinimum.twin().setNext(edgeToMinimum);
			previousEdgeToMinimum = edgeToMinimum;
		}
	});
}

std::vector<InputDcel::Path>
            MsComplexCreator::saddleOrder(InputDcel& dcel, InputDcel::Vertex m) {
	std::vector<InputDcel::Path> order;

	// for all wedge-steepest incoming edges of m, recurse
	InputDcel::HalfEdge edge = m.outgoing();  // arbitrary outgoing edge
	InputDcel::HalfEdge endEdge = edge;
	do {
		if (edge.twin().data().wedgeSteepest) {
			saddleOrderRecursive(dcel, edge.destination(), edge.twin(), order);
		}
		edge = edge.nextOutgoing();
	} while (edge != endEdge);

	return order;
}

void MsComplexCreator::saddleOrderRecursive(
        InputDcel& dcel, InputDcel::Vertex v,
        InputDcel::HalfEdge wedgeSteepestDescentEdge,
        std::vector<InputDcel::Path>& order) {

	assert(wedgeSteepestDescentEdge.data().wedgeSteepest);
	assert(wedgeSteepestDescentEdge.origin() == v);
	assert(wedgeSteepestDescentEdge.destination().data().p.h
	       <= v.data().p.h); // TODO better check with SoS?

	// determine edge to start with
	InputDcel::HalfEdge edge = wedgeSteepestDescentEdge;
	InputDcel::HalfEdge endEdge = edge;

	if (v.data().type == VertexType::saddle &&
	            !edge.data().steepest) {
		order.push_back(dcel.steepestDescentPath(wedgeSteepestDescentEdge));

	} else {
#ifndef NDEBUG
		bool reportedThisSaddle = false;
#endif
		while ((edge = edge.nextOutgoing()) != endEdge) {
			if (edge.twin().data().wedgeSteepest) {
				saddleOrderRecursive(dcel, edge.destination(),
				                     edge.twin(), order);
			} else if (edge.data().wedgeSteepest) {
				assert(edge.origin().data().type == VertexType::saddle);
				assert(!reportedThisSaddle);  // we have a monkey saddle
#ifndef NDEBUG
				reportedThisSaddle = true;
#endif
				InputDcel::Path path =
				        dcel.steepestDescentPath(wedgeSteepestDescentEdge);
				order.push_back(path);
			}
		}
	}
}

void MsComplexCreator::setDcelMsFacesAroundMinimum(MsComplex::Vertex m) {

	m.forAllOutgoingEdges([](MsComplex::HalfEdge edge) {

		MsComplex::HalfEdge next = edge.nextOutgoing();
		InputDcel::Path& p1 = edge.twin().data().m_dcelPath;
		InputDcel::Path& p2 = next.twin().data().m_dcelPath;

		// find common end to skip
		int j = 0;
		while (j < p1.length() && j < p2.length() &&
		       p1.edges()[p1.length() - 1 - j] ==
		                     p2.edges()[p2.length() - 1 - j]) {
			j++;
		}

		// for the remainder of the paths, set the msIncidentFaces
		for (int k = 0; k < p1.length() - j; k++) {
			// m -> s edges from different minima do not share DCEL edges;
			// hence, every msIncidentFace should only be set once
			assert(p1.edges()[k].data().incidentMsFace == -1);
			p1.edges()[k].data().incidentMsFace =
			        edge.oppositeFace().id();
		}
		for (int k = 0; k < p2.length() - j; k++) {
			// same here
			assert(p2.edges()[k].twin().data().incidentMsFace == -1);
			p2.edges()[k].twin().data().incidentMsFace =
			        next.incidentFace().id();
		}
	});
}

void MsComplexCreator::setDcelFacesOfFace(InputDcel& dcel, MsComplex::Face f) {

	MsComplex::HalfEdge e = f.boundary();
	MsComplex::Vertex saddle = e.origin().data().type == VertexType::saddle ?
	            e.origin() :
	            e.destination();
	assert(saddle.data().type == VertexType::saddle);

	// find outgoing edge of `saddle` with incidentMsFace = f
	InputDcel::HalfEdge edge = saddle.data().inputDcelVertex.outgoing();
	auto startEdge = edge;
	do {
		if (edge.data().incidentMsFace == f.id()) {
			break;
		}
		edge = edge.nextOutgoing();
	} while (edge != startEdge);

	// there always needs to be an outgoing edge with incidentMsFace = f, except
	// in the special case that there is only one face in the MS complex:
	// in that case there is only one MS face bordering the saddle, and then it
	// does not matter which one we pick
	assert(msc->faceCount() == 1 || edge.data().incidentMsFace == f.id());

	InputDcel::Face startFace = edge.incidentFace();
	std::vector<InputDcel::Face>& tris = f.data().triangles;

	tris.push_back(startFace);
	startFace.data().msFace = f.id();

	for (int i = 0; i < tris.size(); i++) {
		InputDcel::Face face = tris[i];

		face.forAllBoundaryEdges([&](InputDcel::HalfEdge e) {

			if (e.data().incidentMsFace != -1) {
				// sanity check: we should still be in the same face
				assert(e.data().incidentMsFace == f.id());
			} else {
				if (e.oppositeFace().data().msFace != -1) {
					// sanity check: we should still be in the same face
					assert(e.oppositeFace().data().msFace == f.id());
				} else {
					// found a triangle that we had not yet found before
					tris.push_back(e.oppositeFace());
					e.oppositeFace().data().msFace = f.id();

					face.forAllBoundaryEdges([&f](InputDcel::HalfEdge e) {
						if (!f.data().maximum.isInitialized() ||
						        e.origin().data().p >
						            f.data().maximum.data().p) {
							f.data().maximum = e.origin();
						}
					});
				}
			}
		});
	}

	assert(f.data().maximum.isInitialized());

	InputDcel::Path path = dcel.steepestDescentPath(dcel.halfEdge(
	                            f.data().maximum.data().steepestDescentEdge));
	f.data().lowestPathVertex = path.destination().data().msVertex;
}

void MsComplexCreator::computePersistence() {

	// make list of saddles, sorted from high to low
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
	}
}

void MsComplexCreator::setSandFunctionOfFace(MsComplex::Face f) {

	std::queue<PiecewiseCubicFunction> queue;
	double nInf = -std::numeric_limits<double>::infinity();
	for (auto tri : f.data().triangles) {
		if (tri.boundary().origin().data().p.h == nInf
		        || tri.boundary().next().origin().data().p.h == nInf
		        || tri.boundary().next().next().origin().data().p.h == nInf
		        || tri.boundary().next().next().next() != tri.boundary()) {
			continue;
		}
		queue.push(dcel.volumeAboveFunction(tri));
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
