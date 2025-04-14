#include "fingerfinder.h"

FingerFinder::FingerFinder(std::shared_ptr<InputDcel>& dcel,
                                                 const std::shared_ptr<MsComplex>& msComplex,
                                                 double delta,
                                                 std::function<void(int)> progressListener)
    : m_dcel(dcel), m_msComplex(msComplex), m_delta(delta), m_progressListener(progressListener) {}

void
FingerFinder::signalProgress(int progress) {
	if (m_progressListener != nullptr) {
		m_progressListener(progress);
	}
}

std::vector<InputDcel::Path> FingerFinder::findFingers() {
	// compute volume functions for each edge
	for (size_t i = 0; i < m_dcel->faceCount(); i++) {
		InputDcel::Face face = m_dcel->face(i);
		if (m_dcel->isCritical(face)) {
			computeVolumesForRedTree(face);
		}
	}

	// find significant leaves
	for (size_t i = 0; i < m_dcel->faceCount(); i++) {
		InputDcel::Face face = m_dcel->face(i);
		if (m_dcel->isRedLeaf(face)) {
			auto topEdgeResult = computeTopEdge(face, m_delta);
			face.data().topEdge = topEdgeResult.first.id();
			face.data().pathToTopEdge = topEdgeResult.second;
			face.data().flankingHeight =
			    m_dcel->halfEdge(face.data().topEdge).data().volumeAbove.heightForVolume(m_delta);
			double maximumHeight = maximumVertexHeight(face);
			double volume = m_dcel->halfEdge(face.data().topEdge).data().volumeAbove(maximumHeight);
			if (volume >= m_delta) {
				face.data().isSignificant = true;
				face.data().spurFaces.push_back(topEdgeResult.first.incidentFace().id());
				topEdgeResult.first.incidentFace().forAllReachableFaces(
					[topEdge = topEdgeResult.first](InputDcel::HalfEdge e) {
						if (e == topEdge) {
							return false;
						}
						return e.data().pairedWithFace || e.twin().data().pairedWithFace;
					},
					[&outcropFaces = face.data().spurFaces](InputDcel::Face f, InputDcel::HalfEdge) {
						outcropFaces.push_back(f.id());
					});
				computeSpurBoundary(topEdgeResult.first, face.data().outcropBoundary);
				face.data().outcropBoundary.push_back(topEdgeResult.first.origin().id());
			}
		}
	}

	// find fingers in each red tree
	std::cout << "start" << std::endl;
	std::vector<InputDcel::Path> fingers;
	/*for (size_t i = 0; i < m_dcel->faceCount(); i++) {
		InputDcel::Face face = m_dcel->face(i);
		if (m_dcel->isCritical(face)) {
			// find spur order
			std::vector<InputDcel::Face> leaves = findSignificantLeafOrder(face);
			std::cout << "leaf order size " << leaves.size() << std::endl;
			if (leaves.empty()) {
				continue;
			}
			// for each spur, put fingers around it
			for (int i = 0; i < leaves.size(); i++) {
				InputDcel::Path path;
				path.addEdge(m_dcel->halfEdge(face.data().topEdge));
				fingers.push_back(path);
			}
		}
	}*/
	for (size_t i = 0; i < m_dcel->faceCount(); i++) {
		InputDcel::Face face = m_dcel->face(i);
		if (m_dcel->isRedLeaf(face) && face.data().isSignificant) {
			double flankingHeight = face.data().flankingHeight;
			std::vector<int> boundary = face.data().outcropBoundary;
			int startIndex = -1;
			for (int i = 0; i < boundary.size(); i++) {
				if (m_dcel->vertex(boundary[i]).data().p.h < flankingHeight) {
					startIndex = i;
					break;
				}
			}
			int endIndex = -1;
			for (int i = boundary.size() - 1; i >= 0; i--) {
				if (m_dcel->vertex(boundary[i]).data().p.h < flankingHeight) {
					endIndex = i;
					break;
				}
			}
			if (startIndex == -1 || endIndex == -1) {
				continue;
			}
			InputDcel::Path finger;
			for (int i = startIndex; i < endIndex; i++) {
				finger.addEdge(
				    m_dcel->vertex(boundary[i]).outgoingTo(m_dcel->vertex(boundary[i + 1])));
			}
			fingers.push_back(finger);
		}
	}

	std::cout << "FOUND " << fingers.size() << " FINGERS" << std::endl;
	return fingers;
}

double FingerFinder::maximumVertexHeight(InputDcel::Face face) {
	double maximumHeight = -std::numeric_limits<double>::infinity();
	face.forAllBoundaryVertices([&maximumHeight](InputDcel::Vertex v) {
		maximumHeight = std::max(maximumHeight, v.data().p.h);
	});
	return maximumHeight;
}

void FingerFinder::computeVolumesForRedTree(InputDcel::Face face) {
	std::queue<InputDcel::HalfEdge> queue;
	//std::cout << "red tree " << face.data().p << std::endl;
	
	// for each red leaf v: insert (u, v) into queue
	face.forAllReachableFaces([](InputDcel::HalfEdge e) {
			return e.twin().data().pairedWithFace;
		}, [this, &queue](InputDcel::Face f, InputDcel::HalfEdge e) {
		if (m_dcel->isRedLeaf(f)) {
			queue.push(e.twin());
		}
	});

	std::vector<bool> visited(m_dcel->halfEdgeCount(), false);
	while (!queue.empty()) {
		InputDcel::HalfEdge edge = queue.front();
		queue.pop();
		if (visited[edge.id()]) {
			continue;
		}
		visited[edge.id()] = true;
		//std::cout << "    " << edge.oppositeFace().data().p << " -> " << edge.incidentFace().data().p << std::endl;
		edge.data().volumeAbove = volumeAbove(edge);
		size_t unvisitedCount = 0;
		InputDcel::HalfEdge unvisited;
		edge.oppositeFace().forAllBoundaryEdges(
		    [&visited, &unvisitedCount, &unvisited](InputDcel::HalfEdge boundary) {
			    if ((boundary.data().pairedWithFace || boundary.twin().data().pairedWithFace) &&
			        !visited[boundary.twin().id()]) {
				    unvisitedCount++;
				    unvisited = boundary.twin();
			    }
		    });
		//std::cout << "        unvisited count " << unvisitedCount << std::endl;
		if (unvisitedCount == 0) {
			edge.oppositeFace().forAllBoundaryEdges([&queue](InputDcel::HalfEdge boundary) {
				queue.push(boundary);
				//std::cout << "        inserting " << boundary.oppositeFace().data().p << " -> "
				//          << boundary.incidentFace().data().p << std::endl;
			});
		} else if (unvisitedCount == 1) {
			queue.push(unvisited.twin());
			//std::cout << "        inserting " << unvisited.twin().oppositeFace().data().p << " -> "
			//          << unvisited.twin().incidentFace().data().p << std::endl;
		}
	}

	size_t vCount = 0;
	for (size_t i = 0; i < visited.size(); i++) {
		if (visited[i]) {
			vCount++;
		}
	}
	//std::cout << "visited: " << vCount << std::endl;
}

PiecewiseLinearFunction FingerFinder::volumeAbove(InputDcel::HalfEdge edge) {
	InputDcel::Face face = edge.incidentFace();
	PiecewiseLinearFunction result = m_dcel->volumeAbove(face);
	face.forAllBoundaryEdges([this, edge, &result](InputDcel::HalfEdge e) {
		if (e == edge) {
			return;
		}
		if (e.data().pairedWithFace || e.twin().data().pairedWithFace) {
			PiecewiseLinearFunction descendantVolume = e.twin().data().volumeAbove;
			result = result.add(descendantVolume);
		}
	});
	double cutOffHeight = std::max(edge.origin().data().p.h, edge.destination().data().p.h);
	result.setToZeroAbove(cutOffHeight);
	return result;
}

std::pair<InputDcel::HalfEdge, std::vector<int>> FingerFinder::computeTopEdge(InputDcel::Face f,
                                                                              double delta) {
	assert(m_dcel->isRedLeaf(f));
	InputDcel::HalfEdge candidate;
	f.forAllBoundaryEdges([&candidate](InputDcel::HalfEdge e) {
		if (e.data().pairedWithFace || e.twin().data().pairedWithFace) {
			candidate = e;
		}
	});
	assert(candidate.isInitialized());
	double candidateHeight = std::min(candidate.data().volumeAbove.heightForVolume(delta),
	                                  candidate.twin().data().volumeAbove.heightForVolume(delta));
	double height;
	std::vector<int> path;
	path.push_back(f.id());
	InputDcel::HalfEdge edge;
	int higherCount;
	do {
		height = candidateHeight;
		edge = candidate;
		path.push_back(candidate.incidentFace().id());
		higherCount = 0;
		candidateHeight = -std::numeric_limits<double>::infinity();
		edge.oppositeFace().forAllBoundaryEdges([this, &candidate, &candidateHeight, height,
		                                         &higherCount, delta](InputDcel::HalfEdge e) {
			if (e.data().pairedWithFace || e.twin().data().pairedWithFace) {
				double h = std::min(e.data().volumeAbove.heightForVolume(delta),
				                    e.twin().data().volumeAbove.heightForVolume(delta));
				if (h > height) {
					higherCount++;
				}
				if (h > candidateHeight) {
					candidateHeight = h;
					candidate = e;
				}
			}
		});
	} while (candidateHeight > height && higherCount == 1);

	return std::make_pair(edge, path);
}

std::vector<InputDcel::Face> FingerFinder::findSignificantLeafOrder(InputDcel::Face face) {
	//std::cout << "Finding leaf order for tree " << face.data().p << std::endl;
	std::vector<InputDcel::Face> order;

	InputDcel::HalfEdge e = face.boundary().next();
	while (e != face.boundary()) {
		if (e.twin().data().pairedWithFace) {
			findSignificantLeafOrderForSubtree(e.twin(), order);
		}
		e = e.next();
	}

	return order;
}

void FingerFinder::findSignificantLeafOrderForSubtree(InputDcel::HalfEdge edge,
                                                      std::vector<InputDcel::Face>& order) {
	if (m_dcel->isRedLeaf(edge.incidentFace())) {
		if (edge.incidentFace().data().isSignificant) {
			//std::cout << "    found significant leaf " << edge.incidentFace().data().p << std::endl;
			order.push_back(edge.incidentFace());
		}
	} else {
		InputDcel::HalfEdge e = edge.next();
		while (e != edge) {
			if (e.twin().data().pairedWithFace) {
				findSignificantLeafOrderForSubtree(e.twin(), order);
			}
			e = e.next();
		}
	}
}

void FingerFinder::computeSpurBoundary(InputDcel::HalfEdge topEdge, std::vector<int>& result) {
	InputDcel::HalfEdge e = topEdge.next();
	while (e != topEdge) {
		if (result.size() >= 2 && result[result.size() - 2] == e.origin().id()) {
			result.resize(result.size() - 1);
		} else if (result.size() >= 1 && result[result.size() - 1] == e.origin().id()) {
			// do nothing
		} else {
			result.push_back(e.origin().id());
		}

		if (e.data().pairedWithFace || e.twin().data().pairedWithFace) {
			computeSpurBoundary(e.twin(), result);
		}

		e = e.next();
	}
}
