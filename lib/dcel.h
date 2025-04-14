#ifndef DCEL_H
#define DCEL_H

#include <cassert>
#include <functional>
#include <iomanip>
#include <iostream>
#include <queue>
#include <type_traits>
#include <vector>

/**
 * \brief A doubly-connected edge list.
 *
 * An implementation of a doubly-connected edge list (DCEL), a data structure
 * that stores a planar subdivision. A DCEL consists of vertices, edges and
 * faces, that are connected to each other in such a way that it is easy to
 * traverse the subdivision. Every edge is stored twice, one for each
 * direction; both halves are called half-edges. A half-edge, representing
 * one side of an edge, is incident to only one face.
 *
 * A cycle of a half-edge and its twin is supposed to run in counter-clockwise
 * order.
 *
 * Vertices have a pointer to
 *
 * * an arbitrary outgoing half-edge.
 *
 * Half-edges have a pointer to
 *
 * * its origin vertex;
 * * its incident face;
 * * its previous and next half-edges on the incident face;
 * * its twin (the opposite half-edge).
 *
 * Faces have a pointer to
 *
 * * an arbitrary half-edge on the boundary.
 *
 * In this class, the pointers are implemented as indexes into the list of
 * vertices, the list of edges and the list of faces.
 *
 * Furthermore, vertices, half-edges and faces can carry data of an arbitrary
 * type.
 *
 * \tparam VertexData The vertex data type; needs to be default-constructible.
 * \tparam HalfEdgeData The half-edge data type; needs to be
 * default-constructible.
 * \tparam FaceData The face data type; needs to be default-constructible.
 */
template<typename VertexData, typename HalfEdgeData, typename FaceData>
class Dcel {

	protected:

		/**
		 * The pointers of a vertex in the DCEL.
		 */
		struct VertexImpl {

			/**
			 * The associated data.
			 */
			VertexData m_data;

			/**
			 * The index of an outgoing half-edge.
			 */
			int m_outgoing = -1;

			/**
			 * Whether this vertex has been removed.
			 */
			bool removed = false;
		};

		/**
		 * The pointers of a half-edge in the DCEL.
		 */
		struct HalfEdgeImpl {

			/**
			 * The associated data.
			 */
			HalfEdgeData m_data;

			/**
			 * The index of the origin vertex.
			 */
			int m_origin = -1;

			/**
			 * The index of the twin half-edge.
			 */
			int m_twin = -1;

			/**
			 * The index of the previous half-edge on the incident face.
			 */
			int m_previous = -1;

			/**
			 * The index of the next half-edge on the incident face.
			 */
			int m_next = -1;

			/**
			 * The index of the incident face.
			 */
			int m_incidentFace = -1;

			/**
			 * Whether this half-edge has been removed.
			 */
			bool removed = false;
		};

		/**
		 * The pointers of a face in the DCEL.
		 */
		struct FaceImpl {

			/**
			 * The associated data.
			 */
			FaceData m_data;

			/**
			 * The index of an incident edge.
			 */
			int m_boundary = -1;

			/**
			 * Whether this face has been removed.
			 */
			bool removed = false;
		};

	public:

		class Vertex;
		class HalfEdge;
		class Face;
		class Wedge;

		/**
		 * A vertex in a doubly-connected edge list.
		 *
		 * Technically, this class does not contain the vertex data itself;
		 * it just references the actual vertex data which gets stored in the
		 * Dcel class. Therefore, Vertex objects can be shared by value
		 * efficiently.
		 *
		 * A Vertex can be in an uninitialized state (when they are constructed
		 * using the default constructor). Initialized vertices can be obtained
		 * only via the Dcel class, or by following pointers from other
		 * initialized objects. The result of calling any method, except for
		 * isInitialized(), on an uninitialized vertex is undefined.
		 */
		class Vertex {

			/**
			 * The main `Dcel` class may access our pointers. Other classes
			 * need to go through the `Dcel` class.
			 */
			template<typename Vertex, typename HalfEdge, typename Face>
			friend class Dcel;

			public:

				/**
				 * Creates an uninitialized vertex.
				 */
				Vertex() {
				}

				/**
				 * Checks whether this vertex is initialized.
				 *
				 * \return `true` if the vertex is initialized; `false`
				 * otherwise.
				 */
				bool isInitialized() const {
					return m_id != -1;
				}

				/**
				 * Returns whether this vertex has been removed.
				 *
				 * @return `true` if the vertex has been removed; `false`
				 * otherwise.
				 */
				bool isRemoved() const {
					return m_dcel->m_vertices[id()].removed;
				}

				/**
				 * Returns the ID of this vertex.
				 *
				 * \return The index `i` such that `Dcel::vertex(i)` returns
				 * this vertex.
				 */
				int id() const {
					assert(isInitialized());
					return m_id;
				}

				/**
				 * Returns the vertex data.
				 * \return The vertex data.
				 */
				VertexData& data() {
					assert(isInitialized());
					return m_dcel->m_vertices[id()].m_data;
				}

				/**
				 * Returns the vertex data.
				 * \return The vertex data.
				 */
				const VertexData& data() const {
					assert(isInitialized());
					return m_dcel->m_vertices[id()].m_data;
				}

				/**
				 * Sets the vertex data.
				 * \param data The new vertex data.
				 */
				void setData(VertexData data) {
					assert(isInitialized());
					m_dcel->m_vertices[id()].m_data = data;
				}

				/**
				 * Returns the outgoing half-edge of a vertex.
				 * \return The ID of the outgoing half-edge.
				 */
				HalfEdge outgoing() const {
					assert(isInitialized());
					return m_dcel->halfEdge(m_dcel->m_vertices[id()].m_outgoing);
				}

				/**
				 * Returns an incoming half-edge of this vertex (the twin
				 * of the `outgoing` half-edge).
				 *
				 * \return An incoming half-edge.
				 */
				HalfEdge incoming() const {
					assert(isInitialized());
					return outgoing().twin();
				}

				/**
				 * Sets the outgoing half-edge of this vertex.
				 * \param outgoing The outgoing half-edge.
				 */
				void setOutgoing(HalfEdge outgoing) {
					assert(isInitialized());
					assert(outgoing.isInitialized());
					m_dcel->m_vertices[id()].m_outgoing = outgoing.id();
				}

				/**
				 * Returns an incident face of this vertex (the incident
				 * face of the outgoing half-edge).
				 *
				 * \return An incident face.
				 */
				Face incidentFace() const {
					assert(isInitialized());
					return outgoing().incidentFace();
				}

				/**
				 * Performs an action for all outgoing edges of this vertex,
				 * starting from the edge returned by outgoing(), in
				 * counter-clockwise order.
				 *
				 * \param f A function to call for every outgoing edge.
				 */
				void forAllOutgoingEdges(std::function<void(HalfEdge)> f) {
					assert(isInitialized());
					if (!outgoing().isInitialized()) {
						// this vertex has no incident edges
						return;
					}
					forAllOutgoingEdges(outgoing(), f);
				}

				/**
				 * Performs an action for all outgoing edges of this vertex,
				 * starting from the given edge, in counter-clockwise order.
				 *
				 * \param startEdge The edge to start from. This must be an
				 * outgoing edge of this vertex.
				 * \param f A function to call for every outgoing edge.
				 */
				void forAllOutgoingEdges(HalfEdge startEdge,
										 std::function<void(HalfEdge)> f) {

					assert(isInitialized());
					assert(startEdge.isInitialized());
					assert(startEdge.origin() == *this);

					HalfEdge edge = startEdge;

					do {
						f(edge);
						edge = edge.nextOutgoing();
					} while (edge != startEdge);
				}

				/**
				 * Returns the outgoing half-edge of this vertex to the given
				 * neighbor vertex.
				 *
				 * \param neighbor The neighbor vertex to search for.
				 * \return The outgoing edge to \c neighbor, or if that does not
				 * exist, an uninitialized edge.
				 */
				HalfEdge outgoingTo(Vertex neighbor) {
					assert(neighbor.isInitialized());
					HalfEdge result;
					forAllOutgoingEdges([neighbor, &result](HalfEdge e) {
						if (e.destination() == neighbor) {
							result = e;
						}
					});
					return result;
				}

				/**
				 * Performs an action for all incoming edges of this vertex,
				 * starting from the edge returned by incoming(), in
				 * counter-clockwise order.
				 *
				 * \param f A function to call for every incoming edge.
				 */
				void forAllIncomingEdges(std::function<void(HalfEdge)> f) {
					assert(isInitialized());
					if (!outgoing().isInitialized()) {
						// this vertex has no incident edges
						return;
					}
					forAllIncomingEdges(incoming(), f);
				}

				/**
				 * Performs an action for all incoming edges of this vertex,
				 * starting from the given edge, in counter-clockwise order.
				 *
				 * \param startEdge The edge to start from. This must be an
				 * incoming edge of this vertex.
				 * \param f A function to call for every incoming edge.
				 */
				void forAllIncomingEdges(HalfEdge startEdge,
				                         std::function<void(HalfEdge)> f) {

					assert(isInitialized());
					assert(startEdge.isInitialized());
					assert(startEdge.destination() == *this);

					HalfEdge edge = startEdge;

					do {
						f(edge);
						edge = edge.nextIncoming();
					} while (edge != startEdge);
				}

				/**
				 * Returns the incoming half-edge of this vertex from the given
				 * neighbor vertex.
				 *
				 * \param neighbor The neighbor vertex to search for.
				 * \return The incoming edge from \c neighbor, or if that does
				 * not exist, an uninitialized edge.
				 */
				HalfEdge incomingFrom(Vertex neighbor) {
					HalfEdge result;
					forAllIncomingEdges([neighbor, &result](HalfEdge e) {
						if (e.origin() == neighbor) {
							result = e;
						}
					});
					return result;
				}

				/**
				 * Performs an action for all incident faces of this vertex,
				 * starting from the edge returned by outgoing(), in
				 * counter-clockwise order.
				 *
				 * \param f A function to call for every incident face.
				 */
				void forAllIncidentFaces(std::function<void(Face)> f) {
					assert(isInitialized());

					forAllOutgoingEdges([f](HalfEdge e) {
						f(e.incidentFace());
					});
				}

				/**
				 * Checks whether this vertex is part of the given face.
				 *
				 * \param f The face to check for.
				 * \return `true` if this vertex is incident to `f`; `false`
				 * otherwise.
				 */
				bool incidentToFace(Face f) {

					bool incident = false;

					forAllOutgoingEdges([&incident, f](HalfEdge e) {
						if (e.incidentFace() == f) {
							incident = true;
						}
					});

					return incident;
				}

				/**
				 * Determines the degree of this vertex.
				 *
				 * \return The degree of this vertex, that is, the number of
				 * outgoing half-edges.
				 */
				int degree() {

					int degree = 0;

					forAllOutgoingEdges([&degree](HalfEdge) {
						degree++;
					});

					return degree;
				}

				/**
				 * Performs an action for all vertices that are reachable from
				 * this vertex (but not this vertex itself).
				 *
				 * \note This performs a BFS on the DCEL.
				 *
				 * \param f A function to call for every reachable vertex. The
				 * first argument contains the vertex, the second argument is
				 * the half-edge that lead to this vertex. (That is, all those
				 * half-edges together form a BFS tree.)
				 */
				void forAllReachableVertices(
				        std::function<void(Vertex, HalfEdge)> f) {

					assert(isInitialized());
					forAllReachableVertices([](HalfEdge e) {
						return true;
					}, f);
				}

				/**
				 * Performs an action for all vertices that are reachable from
				 * this vertex (but not this vertex itself), while passing
				 * along certain edges only.
				 *
				 * \note This performs a BFS on the DCEL.
				 *
				 * \param edgeCheck A function to call for every encountered
				 * half-edge, that returns whether we are allowed to take this
				 * half-edge or not.
				 * \param f A function to call for every reachable vertex. The
				 * first argument contains the vertex, the second argument is
				 * the half-edge that lead to this vertex. (That is, all those
				 * half-edges together form a BFS tree.)
				 */
				void forAllReachableVertices(
				        std::function<bool(HalfEdge)> edgeCheck,
				        std::function<void(Vertex, HalfEdge)> f) {

					assert(isInitialized());
					std::vector<bool> visited(m_dcel->vertexCount(), false);
					Vertex v = *this;
					visited[v.id()] = true;
					std::queue<Vertex> queue;
					queue.push(v);

					while (!queue.empty()) {
						Vertex v = queue.front();
						queue.pop();
						v.forAllOutgoingEdges(
						            [&f, &queue, &visited, &edgeCheck]
						            (HalfEdge outgoing) {
							if (!edgeCheck(outgoing)) {
								return;
							}
							Vertex vNew = outgoing.destination();
							if (!visited[vNew.id()]) {
								visited[vNew.id()] = true;
								queue.push(vNew);
								f(vNew, outgoing);
							}
						});
					}
				}

				/**
				 * Performs an action for all vertices that are reachable from
				 * this vertex (but not this vertex itself), while passing
				 * along certain edges only. This variant of the method
				 * maintains the distance (in the number of half-edges) of all
				 * returned vertices to this vertex.
				 *
				 * \note This performs a BFS on the DCEL.
				 *
				 * \param edgeCheck A function to call for every encountered
				 * half-edge, that returns whether we are allowed to take this
				 * half-edge or not.
				 * \param f A function to call for every reachable vertex. The
				 * first argument contains the vertex, the second argument is
				 * the half-edge that lead to this vertex (that is, all those
				 * half-edges together form a BFS tree) and the third argument
				 * is the distance from this vertex to the returned vertex.
				 */
				void forAllReachableVertices(
				        std::function<bool(HalfEdge)> edgeCheck,
				        std::function<void(Vertex, HalfEdge, int)> f) {

					assert(isInitialized());
					std::vector<int> distance(m_dcel->vertexCount(), -1);
					Vertex v = *this;
					distance[v.id()] = 0;
					std::queue<Vertex> queue;
					queue.push(v);

					while (!queue.empty()) {
						Vertex v = queue.front();
						queue.pop();
						v.forAllOutgoingEdges(
						            [&f, &v, &queue, &distance, &edgeCheck]
									(HalfEdge outgoing) {
							if (!edgeCheck(outgoing)) {
								return;
							}
							Vertex vNew = outgoing.destination();
							if (distance[vNew.id()] == -1) {
								distance[vNew.id()] = distance[v.id()] + 1;
								queue.push(vNew);
								f(vNew, outgoing, distance[vNew.id()]);
							}
						});
					}
				}

				/**
				 * Checks whether some other vertex is reachable from this
				 * vertex.
				 *
				 * \note This performs a BFS on the DCEL, and hence takes linear
				 * time.
				 *
				 * \param v The vertex to search for.
				 * \return `true` if `v` is reachable from this vertex; `false`
				 * otherwise.
				 */
				bool isReachable(Vertex v) {

					if (v == *this) {
						return true;
					}

					bool reachable = false;

					forAllReachableVertices([&reachable, &v](Vertex v2,
					                        HalfEdge e) {
						if (v2 == v) {
							reachable = true;
						}
					});

					return reachable;
				}

				/**
				 * Splits this vertex into two, along two edges.
				 *
				 * \image html dcel-vertex-split-edge-edge.png
				 *
				 * The splitting is performed in the following way:
				 *
				 * * make a copy v' of this vertex;
				 *
				 * * make copies of half-edges e1 and e2 and their twins, that
				 *   go to v' instead of this vertex;
				 *
				 * * move all edges incident to v that are between e2 and e1
				 *   (start at e2, walk counter-clockwise until encountering e1)
				 *   to v';
				 *
				 * * add a new face f with boundary edges e1, e2, e2', e1';
				 *
				 * * set all pointers correctly.
				 *
				 * It is guaranteed that the outgoing pointer of v' will be
				 * set to e1'.
				 *
				 * \note This method assumes that VertexData and HalfEdgeData
				 * are copyable.
				 *
				 * \param e1 The first outgoing edge to split along.
				 * \param e2 The second outgoing edge to split along.
				 * \return The new vertex.
				 */
				Vertex split(HalfEdge e1, HalfEdge e2) {
					assert(isInitialized());
					assert(e1.origin() == *this);
					assert(e2.origin() == *this);

					Vertex vs = m_dcel->addVertex();
					vs.setData(this->data());

					HalfEdge e1s = m_dcel->addEdge(vs, e1.destination());
					e1s.setData(e1.data());
					e1s.twin().setData(e1.twin().data());

					HalfEdge e2s = m_dcel->addEdge(vs, e2.destination());
					e2s.setData(e2.data());
					e2s.twin().setData(e2.twin().data());

					// make sure that the outgoing half-edge is not one that is
					// going to be moved to vs
					setOutgoing(e1);

					vs.setOutgoing(e1s);

					e1s.twin().setNext(e2s);
					e1.previous().setNext(e1s);

					e2s.twin().setNext(e2.twin().next());
					e2.twin().setNext(e1);

					e2.twin().previous().setNext(e2s.twin());
					e2s.setNext(e2.twin());

					e1s.setNext(e1.next());
					e1.setNext(e1s.twin());

					Face f = m_dcel->addFace(e1);
					e1s.setIncidentFace(e1.incidentFace());
					e1.incidentFace().setBoundary(e1s);
					e2s.twin().setIncidentFace(e2.twin().incidentFace());
					e2.twin().incidentFace().setBoundary(e2s.twin());
					e1.setIncidentFace(f);
					e1s.twin().setIncidentFace(f);
					e2s.setIncidentFace(f);
					e2.twin().setIncidentFace(f);

					HalfEdge e = e2s.nextOutgoing();
					while (e != e1s) {
						e.setOrigin(vs);
						e = e.nextOutgoing();
					}

					return vs;
				}

				/**
				 * Splits this vertex into two, along a wedge and an edge.
				 *
				 * \image html dcel-vertex-split-wedge-edge.png
				 *
				 * The splitting is performed in the following way:
				 *
				 * * make a copy v' of this vertex;
				 *
				 * * make copies of half-edge e and its twin, that go to v'
				 *   instead of this vertex;
				 *
				 * * move all edges incident to v that are between e and w
				 *   (start at e, walk counter-clockwise until encountering w)
				 *   to v';
				 *
				 * * enlarge the face of w so that it includes the twin of e
				 *   and e';
				 *
				 * * set all pointers correctly.
				 *
				 * It is guaranteed that the outgoing pointer of v' will be
				 * set to e'.
				 *
				 * \note This method assumes that VertexData and HalfEdgeData
				 * are copyable.
				 *
				 * \param w The wedge to split along.
				 * \param e The outgoing edge to split along.
				 * \return The new vertex.
				 */
				Vertex split(Wedge w, HalfEdge e) {
					assert(isInitialized());
					assert(w.vertex() == *this);
					assert(e.origin() == *this);

					Vertex vs = m_dcel->addVertex();
					vs.setData(this->data());

					HalfEdge es = m_dcel->addEdge(vs, e.destination());
					es.setData(e.data());
					es.twin().setData(e.twin().data());

					// make sure that the outgoing half-edge is not one that is
					// going to be moved to vs
					setOutgoing(e);

					vs.setOutgoing(es);

					w.incomingHalfEdge().setNext(es);
					es.twin().setNext(e.twin().next());
					e.twin().previous().setNext(es.twin());
					es.setNext(e.twin());
					e.twin().setNext(w.outgoingHalfEdge());

					es.twin().setIncidentFace(e.twin().incidentFace());
					e.twin().setIncidentFace(w.face());
					es.setIncidentFace(w.face());

					HalfEdge edge = es.nextOutgoing();
					while (edge != es) {
						edge.setOrigin(vs);
						edge = edge.nextOutgoing();
					}

					return vs;
				}

				/**
				 * Splits this vertex into two, along two wedges.
				 *
				 * \image html dcel-vertex-split-wedge-wedge.png
				 *
				 * The splitting is performed in the following way:
				 *
				 * \todo Document this.
				 *
				 * Both wedges need to be part of the same face.
				 *
				 * \note This method assumes that VertexData and HalfEdgeData
				 * are copyable.
				 *
				 * \param w1 The first wedge to split along.
				 * \param w2 The second wedge to split along.
				 * \return The new vertex.
				 */
				Vertex split(Wedge w1, Wedge w2) {
					assert(isInitialized());
					assert(w1.vertex() == *this);
					assert(w2.vertex() == *this);
					assert(w1.face() == w2.face());

					Vertex vs = m_dcel->addVertex();
					vs.setData(this->data());

					// make sure that the outgoing half-edge is not one that is
					// going to be moved to vs
					setOutgoing(w1.outgoingHalfEdge());
					vs.setOutgoing(w2.outgoingHalfEdge());

					HalfEdge w1In = w1.incomingHalfEdge();
					HalfEdge w2In = w2.incomingHalfEdge();
					HalfEdge w1Out = w1.outgoingHalfEdge();
					HalfEdge w2Out = w2.outgoingHalfEdge();

					w1In.setNext(w2Out);
					w2In.setNext(w1Out);

					HalfEdge edge = vs.outgoing();
					do {
						edge.setOrigin(vs);
						edge = edge.nextOutgoing();
					} while (edge != vs.outgoing());

					return vs;
				}

				/**
				 * Removes this vertex, and all incident edges, from the DCEL,
				 * and merges all faces around this vertex into one face.
				 *
				 * This method assumes that all incident faces of incident
				 * edges are distinct.
				 *
				 * \param faceToRetain The outgoing edge of this vertex whose
				 * incident face should be retained when merging all faces
				 * together.
				 */
				void remove(HalfEdge faceToRetain) {
					assert(isInitialized());
					assert(faceToRetain.origin() == *this);

#ifdef DCEL_ENABLE_TRACING_OUTPUT
					std::cout << "REMOVE VERTEX " << id() << std::endl;
#endif

					// = forAllOutgoingEdges(faceToRetain, ...)
					// except that we need to stop as soon as we see a removed
					// edge
					HalfEdge edge = faceToRetain;
					do {
						edge.remove();
						edge = edge.nextOutgoing();
					} while (!edge.isRemoved());

					m_dcel->m_vertices[id()].removed = true;
				}

				/**
				 * Checks whether two vertices are the same.
				 *
				 * \param other The other vertex.
				 * \return `true` if `other` is the same vertex as `this`;
				 * `false` otherwise.
				 */
				bool operator==(const Vertex& other) const {
					return id() == other.id();
				}

				/**
				 * Checks whether two vertices are not the same.
				 *
				 * \param other The other vertex.
				 * \return `false` if `other` is the same vertex as `this`;
				 * `true` otherwise.
				 */
				bool operator!=(const Vertex& other) const {
					return !operator==(other);
				}

				/**
				 * Outputs a representation of a vertex to an output stream,
				 * for debugging purposes.
				 *
				 * \param os The output stream.
				 * \param v The vertex to output.
				 * \return The output stream.
				 */
				friend std::ostream& operator<<(std::ostream& os,
				                                Vertex const& v) {
					os << "vertex " << v.id();
					return os;
				}

			private:

				/**
				 * Creates a new vertex.
				 *
				 * \param dcel The DCEL instance.
				 * \param id The ID of this vertex.
				 */
				Vertex(Dcel<VertexData, HalfEdgeData, FaceData>* dcel,
						int id) : m_dcel(dcel), m_id(id) {
				}

				/**
				 * The DCEL this vertex is a part of.
				 */
				Dcel<VertexData, HalfEdgeData, FaceData>* m_dcel = nullptr;

				/**
				 * The index of this vertex in the `Dcel::vertices()` list, or
				 * `-1` if this vertex is invalid.
				 *
				 * Alternatively `Dcel::vertex()` returns this vertex for this
				 * index.
				 */
				int m_id = -1;
		};

		/**
		 * A half-edge in a doubly-connected edge list.
		 *
		 * Technically, this class does not contain the half-edge data itself;
		 * it just references the actual half-edge data which gets stored in the
		 * Dcel class. Therefore, HalfEdge objects can be shared by value
		 * efficiently.
		 *
		 * A HalfEdge can be in an uninitialized state (when they are
		 * constructed using the default constructor). Initialized half-edges
		 * can be obtained only via the Dcel class, or by following pointers
		 * from other initialized objects. The result of calling any method,
		 * except for isInitialized(), on an uninitialized half-edge is
		 * undefined.
		 */
		class HalfEdge {

			/**
			 * The main `Dcel` class may access our pointers. Other classes
			 * need to go through the `Dcel` class.
			 */
			template<typename Vertex, typename HalfEdge, typename Face>
			friend class Dcel;

			public:

				/**
				 * Creates an uninitialized half-edge.
				 */
				HalfEdge() {
				}

				/**
				 * Checks whether this half-edge is initialized.
				 *
				 * \return `true` if the half-edge is initialized; `false`
				 * otherwise.
				 */
				bool isInitialized() const {
					return m_id != -1;
				}

				/**
				 * Returns whether this half-edge has been removed.
				 *
				 * @return `true` if the half-edge has been removed; `false`
				 * otherwise.
				 */
				bool isRemoved() const {
					return m_dcel->m_halfEdges[id()].removed;
				}

				/**
				 * Returns the ID of this half-edge.
				 *
				 * \return The index `i` such that `Dcel::halfEdge(i)` returns
				 * this half-edge.
				 */
				int id() const {
					assert(isInitialized());
					return m_id;
				}

				/**
				 * Returns the half-edge data.
				 * \return The half-edge data.
				 */
				HalfEdgeData& data() {
					assert(isInitialized());
					return m_dcel->m_halfEdges[id()].m_data;
				}

				/**
				 * Returns the half-edge data.
				 * \return The half-edge data.
				 */
				const HalfEdgeData& data() const {
					assert(isInitialized());
					return m_dcel->m_halfEdges[id()].m_data;
				}

				/**
				 * Sets the half-edge data.
				 * \param data The new half-edge data.
				 */
				void setData(HalfEdgeData data) {
					assert(isInitialized());
					m_dcel->m_halfEdges[id()].m_data = data;
				}

				/**
				 * Returns the origin of this half-edge.
				 * \return The origin vertex.
				 */
				Vertex origin() const {
					assert(isInitialized());
					return m_dcel->vertex(m_dcel->m_halfEdges[id()].m_origin);
				}

				/**
				 * Sets the origin of this half-edge.
				 * \param origin The origin vertex.
				 */
				void setOrigin(Vertex origin) {
					assert(isInitialized());
					assert(origin.isInitialized());
					m_dcel->m_halfEdges[id()].m_origin = origin.id();
				}

				/**
				 * Returns the destination of this half-edge (the origin of
				 * its twin).
				 *
				 * \return The destination vertex.
				 */
				Vertex destination() const {
					assert(isInitialized());
					return twin().origin();
				}

				/**
				 * Returns the twin half-edge of this half-edge.
				 * \return The twin half-edge.
				 */
				HalfEdge twin() const {
					assert(isInitialized());
					return m_dcel->halfEdge(m_dcel->m_halfEdges[id()].m_twin);
				}

				/**
				 * Sets the twin half-edge of this half-edge.
				 * \param twin The twin.
				 */
				void setTwin(HalfEdge twin) {
					assert(isInitialized());
					assert(twin.isInitialized());
					m_dcel->m_halfEdges[id()].m_twin = twin.id();
					m_dcel->m_halfEdges[twin.id()].m_twin = id();
				}

				/**
				 * Returns the previous half-edge on the face of this
				 * half-edge.
				 *
				 * \return The previous half-edge.
				 *
				 * \note To set the previous half-edge, use
				 * `setNext(int)`.
				 */
				HalfEdge previous() const {
					assert(isInitialized());
					return m_dcel->halfEdge(
								m_dcel->m_halfEdges[id()].m_previous);
				}

				/**
				 * Returns the next half-edge on the face of this half-edge.
				 * \return The next half-edge.
				 */
				HalfEdge next() const {
					assert(isInitialized());
					return m_dcel->halfEdge(m_dcel->m_halfEdges[id()].m_next);
				}

				/**
				 * Sets the next half-edge on the face of this half-edge.
				 * \param next The next half-edge.
				 */
				void setNext(HalfEdge next) {
					assert(isInitialized());
					assert(next.isInitialized());
					m_dcel->m_halfEdges[id()].m_next = next.id();
					m_dcel->m_halfEdges[next.id()].m_previous = id();
				}

				/**
				 * Returns the next outgoing half-edge (in counter-clockwise
				 * order) of the origin of this half-edge.
				 *
				 * \return The next outgoing half-edge.
				 */
				HalfEdge nextOutgoing() const {
					assert(isInitialized());
					return twin().next();
				}

				/**
				 * Returns the previous outgoing half-edge (in counter-clockwise
				 * order) of the origin of this half-edge.
				 *
				 * \return The previous outgoing half-edge.
				 */
				HalfEdge previousOutgoing() const {
					assert(isInitialized());
					return previous().twin();
				}

				/**
				 * Returns the next incoming half-edge (in counter-clockwise
				 * order) of the destination of this half-edge.
				 *
				 * \return The next incoming half-edge.
				 */
				HalfEdge nextIncoming() const {
					assert(isInitialized());
					return next().twin();
				}

				/**
				 * Returns the previous incoming half-edge (in counter-clockwise
				 * order) of the destination of this half-edge.
				 *
				 * \return The previous incoming half-edge.
				 */
				HalfEdge previousIncoming() const {
					assert(isInitialized());
					return twin().previous();
				}

				/**
				 * Returns the incident face of this half-edge.
				 *
				 * \return The incident face.
				 */
				Face incidentFace() const {
					assert(isInitialized());
					return m_dcel->face(
								m_dcel->m_halfEdges[id()].m_incidentFace);
				}

				/**
				 * Sets the incident face of this half-edge.
				 * \param incidentFace The incident face.
				 */
				void setIncidentFace(Face incidentFace) {
					assert(isInitialized());
					assert(incidentFace.isInitialized());
					m_dcel->m_halfEdges[id()].m_incidentFace =
								incidentFace.id();
				}

				/**
				 * Returns the opposite face of this half-edge (the incident
				 * face of the twin).
				 *
				 * \return The opposite face.
				 */
				Face oppositeFace() const {
					assert(isInitialized());
					return twin().incidentFace();
				}

				/**
				 * Splits this half-edge and its twin into two pairs of
				 * half-edges, with a new face in between.
				 *
				 * \image html dcel-halfedge-split.png
				 *
				 * \return The newly created face.
				 */
				Face split() {
					assert(isInitialized());
					Face f = m_dcel->addFace(twin());
					split(f);
					return f;
				}

				/**
				 * Splits this half-edge and its twin into two pairs of
				 * half-edges, with the given face in between.
				 *
				 * \image html dcel-halfedge-split.png
				 *
				 * \note If f is an existing face, this adds a new, disconnected
				 * component to the face. This breaks the DCEL structure, which
				 * can be fixed by connecting the two parts of faces together
				 * again. If you want to add a new face instead, use split().
				 */
				void split(Face f) {
					assert(isInitialized());

					HalfEdge e = *this;
					HalfEdge es = e.twin();

					HalfEdge eTwin = m_dcel->addHalfEdge(e.destination());
					e.setTwin(eTwin);
					HalfEdge esTwin = m_dcel->addHalfEdge(e.origin());
					es.setTwin(esTwin);

					eTwin.setData(es.data());
					esTwin.setData(e.data());

					eTwin.setNext(esTwin);
					esTwin.setNext(eTwin);

					eTwin.setIncidentFace(f);
					esTwin.setIncidentFace(f);
				}

				/**
				 * Removes this half-edge and its twin half-edge from the DCEL,
				 * and merges the two incident faces into one face.
				 *
				 * The incident face of this half-edge is maintained; the
				 * incident face of the twin is discarded.
				 *
				 * This method assumes that this half-edge and its twin do not
				 * have the same incident face, as this would split the DCEL in
				 * two.
				 *
				 * If either the source or target of this half-edge have only
				 * this half-edge incident to them, the outgoing-edge pointer
				 * stays pointed at this (now removed) edge. This method then
				 * marks the source / target as removed as well.
				 *
				 * After this method returns, this half-edge and its twin
				 * half-edge, and the discarded face, are removed.
				 */
				void remove() {
					assert(isInitialized());
					assert(!isRemoved());
#ifdef DCEL_ENABLE_TRACING_OUTPUT
					std::cout << "removing edge " << this->id()
					                  << " (" << origin().id() << " -> "
					                  << destination().id() << ")"
					                  << std::endl;
#endif

					m_dcel->m_halfEdges[id()].removed = true;
					m_dcel->m_halfEdges[twin().id()].removed = true;

					// if origin was pointing at this edge, move that pointer to
					// another outgoing edge (and this is the only outgoing
					// edge, remove the origin altogether)
					if (origin().outgoing() == *this) {
						if (nextOutgoing() == *this) {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "    remove origin "
							          << origin().id() << std::endl;
#endif
							m_dcel->m_vertices[origin().id()].removed = true;
						} else {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "    move origin.outgoing to "
							          << nextOutgoing().id() << std::endl;
#endif
							origin().setOutgoing(nextOutgoing());
						}
					}

					// same for the destination
					if (destination().outgoing() == twin()) {
						if (twin().nextOutgoing() == twin()) {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "    remove destination "
							          << destination().id() << std::endl;
#endif
							m_dcel->m_vertices[destination().id()].removed = true;
						} else {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "    move destination.outgoing to "
							          << twin().nextOutgoing().id() << std::endl;
#endif
							destination().setOutgoing(twin().nextOutgoing());
						}
					}

					// if a face was pointing at this edge, move that pointer
					// to another boundary edge (but in particular not the twin,
					// as that is also going to be removed)
					if (incidentFace().boundary() == *this) {
						if (next() == twin() && previous() == twin()) {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "    remove face "
							          << incidentFace().id() << std::endl;
#endif
							m_dcel->m_faces[incidentFace().id()].removed = true;
						} else if (next() == twin()) {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "    move incidentFace.boundary to "
							          << previous().id() << std::endl;
#endif
							incidentFace().setBoundary(previous());
						} else {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "    move incidentFace.boundary to "
							          << next().id() << std::endl;
#endif
							incidentFace().setBoundary(next());
						}
					}

					// same for the twin edge
					if (twin().incidentFace().boundary() == twin()) {
						if (twin().next() == *this && twin().previous() == *this) {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "    remove face "
							          << twin().incidentFace().id() << std::endl;
#endif
							m_dcel->m_faces[twin().incidentFace().id()].removed = true;
						} else if (twin().next() == *this) {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "    move twin.incidentFace.boundary to "
							          << twin().previous().id() << std::endl;
#endif
							twin().incidentFace().setBoundary(twin().previous());
						} else {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "    move twin.incidentFace.boundary to "
							          << twin().next().id() << std::endl;
#endif
							twin().incidentFace().setBoundary(twin().next());
						}
					}

					// merge faces (if they were not the same already)
					if (incidentFace() != twin().incidentFace()) {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
						std::cout << "    merge faces "
						          << incidentFace().id() << " and "
						          << twin().incidentFace().id() << std::endl;
#endif

						// remove incident face of twin
						m_dcel->m_faces[twin().incidentFace().id()].removed
						        = true;

						// let boundary edges point to the merged face
						twin().incidentFace().
						        forAllBoundaryEdges([this](HalfEdge e) {
#ifdef DCEL_ENABLE_TRACING_OUTPUT
							std::cout << "        repainting boundary edge " << e.id()
							          << " (" << e.origin().id() << " -> "
							          << e.destination().id() << "):"
							          << " from " << e.incidentFace().id()
							          << " to " << incidentFace().id()
							          << std::endl;
#endif
							e.setIncidentFace(incidentFace());
						});
					}

					// update previous / next pointers
					previous().setNext(twin().next());
					twin().previous().setNext(next());
				}

				/**
				 * Checks whether two half-edges are the same.
				 *
				 * \param other The other half-edge.
				 * \return `true` if `other` is the same half-edge as `this`;
				 * `false` otherwise.
				 */
				bool operator==(const HalfEdge& other) const {
					return id() == other.id();
				}

				/**
				 * Checks whether two half-edges are not the same.
				 *
				 * \param other The other half-edge.
				 * \return `false` if `other` is the same half-edge as `this`;
				 * `true` otherwise.
				 */
				bool operator!=(const HalfEdge& other) const {
					return !operator==(other);
				}

				/**
				 * Outputs a representation of a half-edge to an output stream,
				 * for debugging purposes.
				 *
				 * \param os The output stream.
				 * \param e The half-edge to output.
				 * \return The output stream.
				 */
				friend std::ostream& operator<<(std::ostream& os,
				                                HalfEdge const& e) {
					os << "half-edge " << e.id() << " ("
					   << e.origin().id() << " -> " << e.destination().id()
					   << ")";
					return os;
				}

			private:

				/**
				 * Creates a new half-edge.
				 *
				 * \param dcel The DCEL instance.
				 * \param id The ID of this half-edge.
				 */
				HalfEdge(Dcel<VertexData, HalfEdgeData, FaceData>* dcel,
						int id) : m_dcel(dcel), m_id(id) {
				}

				/**
				 * The DCEL this half-edge is a part of.
				 */
				Dcel<VertexData, HalfEdgeData, FaceData>* m_dcel = nullptr;

				/**
				 * The index of this half-edge in the `Dcel::halfEdges()` list,
				 * or `-1` if this half-edge is invalid.
				 *
				 * Alternatively `Dcel::halfEdge()` returns this half-edge for
				 * this index.
				 */
				int m_id = -1;
		};

		/**
		 * A face in a doubly-connected edge list.
		 *
		 * Technically, this class does not contain the face data itself;
		 * it just references the actual face data which gets stored in the
		 * Dcel class. Therefore, Face objects can be shared by value
		 * efficiently.
		 *
		 * A Face can be in an uninitialized state (when they are constructed
		 * using the default constructor). Initialized faces can be obtained
		 * only via the Dcel class, or by following pointers from other
		 * initialized objects. The result of calling any method, except for
		 * isInitialized(), on an uninitialized face is undefined.
		 */
		class Face {

			/**
			 * The main `Dcel` class may access our pointers. Other classes
			 * need to go through the `Dcel` class.
			 */
			template<typename Vertex, typename HalfEdge, typename Face>
			friend class Dcel;

			public:

				/**
				 * Creates an uninitialized face.
				 */
				Face() {
				}

				/**
				 * Checks whether this face is initialized.
				 * \return `true` if the face is initialized; `false` otherwise.
				 */
				bool isInitialized() const {
					return m_id != -1;
				}

				/**
				 * Returns whether this face has been removed.
				 *
				 * @return `true` if the face has been removed; `false`
				 * otherwise.
				 */
				bool isRemoved() const {
					return m_dcel->m_faces[id()].removed;
				}

				/**
				 * Returns the ID of this face.
				 *
				 * \return The index `i` such that `Dcel::face(i)` returns
				 * this face.
				 */
				int id() const {
					assert(isInitialized());
					return m_id;
				}

				/**
				 * Returns the face data.
				 * \return The face data.
				 */
				FaceData& data() {
					assert(isInitialized());
					return m_dcel->m_faces[id()].m_data;
				}

				/**
				 * Returns the face data.
				 * \return The face data.
				 */
				const FaceData& data() const {
					assert(isInitialized());
					return m_dcel->m_faces[id()].m_data;
				}

				/**
				 * Sets the face data.
				 * \param data The new face data.
				 */
				void setData(FaceData data) {
					assert(isInitialized());
					m_dcel->m_faces[id()].m_data = data;
				}

				/**
				 * Returns a boundary edge of this face.
				 * \return A boundary edge.
				 */
				HalfEdge boundary() const {
					assert(isInitialized());
					return m_dcel->halfEdge(m_dcel->m_faces[id()].m_boundary);
				}

				/**
				 * Sets the boundary edge of this face.
				 * \param boundary The boundary edge.
				 */
				void setBoundary(HalfEdge boundary) {
					assert(isInitialized());
					assert(boundary.isInitialized());
					m_dcel->m_faces[id()].m_boundary = boundary.id();
				}

				/**
				 * Returns a boundary vertex of this face.
				 * \return A boundary vertex.
				 */
				Vertex boundaryVertex() const {
					assert(isInitialized());
					return boundary().origin();
				}

				/**
				 * Performs an action for all boundary edges of this face,
				 * starting from the edge returned by boundary(), in
				 * clockwise order around the face.
				 *
				 * \param f A function to call for every boundary edge.
				 */
				void forAllBoundaryEdges(std::function<void(HalfEdge)> f) {
					assert(isInitialized());
					forAllBoundaryEdges(boundary(), f);
				}

				/**
				 * Performs an action for all outgoing edges of this vertex,
				 * starting from the given edge, in clockwise order around
				 * the face.
				 *
				 * \param startEdge The edge to start from. This must be a
				 * boundary edge of this face.
				 * \param f A function to call for every boundary edge.
				 */
				void forAllBoundaryEdges(HalfEdge startEdge,
										 std::function<void(HalfEdge)> f) {

					assert(isInitialized());
					assert(startEdge.incidentFace() == *this);

					HalfEdge edge = startEdge;

					do {
						f(edge);
						edge = edge.next();
					} while (edge != startEdge);
				}

				/**
				 * Performs an action for all boundary vertices of this face,
				 * starting from the vertex returned by boundary().origin(), in
				 * clockwise order around the face.
				 *
				 * \param f A function to call for every boundary vertex.
				 */
				void forAllBoundaryVertices(std::function<void(Vertex)> f) {
					assert(isInitialized());
					forAllBoundaryEdges(boundary(), [f](HalfEdge e) {
						f(e.origin());
					});
				}

				/**
				 * Performs an action for all faces that are reachable from
				 * this face (but not this face itself).
				 *
				 * \note This performs a BFS on the DCEL.
				 *
				 * \param f A function to call for every reachable face. The
				 * first argument contains the face, the second argument is
				 * the half-edge that lead to this face. (That is, the dual of
				 * all those half-edges together form a BFS tree over the
				 * faces.)
				 */
				void forAllReachableFaces(
						std::function<void(Face, HalfEdge)> f) {

					assert(isInitialized());
					forAllReachableFaces([](HalfEdge e) {
						return true;
					}, f);
				}

				/**
				 * Performs an action for all faces that are reachable from
				 * this face (but not this face itself), while passing over
				 * certain edges only.
				 *
				 * \note This performs a BFS on the DCEL.
				 *
				 * \param edgeCheck A function to call for every encountered
				 * half-edge, that returns whether we are allowed to cross this
				 * half-edge or not.
				 * \param f A function to call for every reachable face. The
				 * first argument contains the face, the second argument is
				 * the half-edge that lead to this face. (That is, the dual of
				 * all those half-edges together form a BFS tree over the
				 * faces.)
				 */
				void forAllReachableFaces(
						std::function<bool(HalfEdge)> edgeCheck,
						std::function<void(Face, HalfEdge)> f) {

					assert(isInitialized());
					std::vector<bool> visited(m_dcel->faceCount(), false);
					Face face = *this;
					visited[face.id()] = true;
					std::queue<Face> queue;
					queue.push(face);

					while (!queue.empty()) {
						Face face = queue.front();
						queue.pop();
						face.forAllBoundaryEdges(
									[&f, &queue, &visited, &edgeCheck]
									(HalfEdge boundary) {
							if (!edgeCheck(boundary)) {
								return;
							}
							Face fNew = boundary.twin().incidentFace();
							if (!visited[fNew.id()]) {
								visited[fNew.id()] = true;
								queue.push(fNew);
								f(fNew, boundary);
							}
						});
					}
				}

				/**
				 * Checks whether two faces are the same.
				 *
				 * \param other The other face.
				 * \return `true` if `other` is the same face as `this`;
				 * `false` otherwise.
				 */
				bool operator==(const Face& other) const {
					return id() == other.id();
				}

				/**
				 * Checks whether two faces are not the same.
				 *
				 * \param other The other face.
				 * \return `false` if `other` is the same face as `this`;
				 * `true` otherwise.
				 */
				bool operator!=(const Face& other) const {
					return !operator==(other);
				}

				/**
				 * Outputs a representation of a face to an output stream,
				 * for debugging purposes.
				 *
				 * \param os The output stream.
				 * \param f The face to output.
				 * \return The output stream.
				 */
				friend std::ostream& operator<<(std::ostream& os,
				                                Face const& f) {
					os << "face " << f.id();
					return os;
				}

			private:

				/**
				 * Creates a new face.
				 *
				 * \param dcel The DCEL instance.
				 * \param id The ID of this face.
				 */
				Face(Dcel<VertexData, HalfEdgeData, FaceData>* dcel,
						int id) : m_dcel(dcel), m_id(id) {
				}

				/**
				 * The DCEL this vertex is a part of.
				 */
				Dcel<VertexData, HalfEdgeData, FaceData>* m_dcel = nullptr;

				/**
				 * The index of this face in the `Dcel::faces()` list, or `-1`
				 * if this face is invalid.
				 *
				 * Alternatively `Dcel::face()` returns this face for this
				 * index.
				 */
				int m_id = -1;
		};

		/**
		 * A wedge between two adjacent edges at a vertex. A wedge is the area
		 * between an incoming and the next outgoing half-edge.
		 *
		 * Actually, a wedge could just be represented as a half-edge. However,
		 * Wedge is a separate class to make code cleaner, and for overload
		 * resolution.
		 *
		 * A Wedge can be in an uninitialized state (when they are constructed
		 * using the default constructor). Initialized wedges can only be
		 * obtained via the Dcel class, or by following pointers from other
		 * initialized objects. The result of calling any method, except for
		 * isInitialized(), on an uninitialized wedge is undefined.
		 */
		class Wedge {

			/**
			 * The main `Dcel` class may access our pointers. Other classes
			 * need to go through the `Dcel` class.
			 */
			template<typename Vertex, typename HalfEdge, typename Face>
			friend class Dcel;

			public:

				/**
				 * Creates an uninitialized face.
				 */
				Wedge() {
				}

				/**
				 * Checks whether this face is initialized.
				 * \return `true` if the face is initialized; `false` otherwise.
				 */
				bool isInitialized() const {
					return m_outId != -1;
				}

				/**
				 * Returns the outgoing half-edge of this wedge.
				 * \return The outgoing half-edge.
				 */
				HalfEdge outgoingHalfEdge() const {
					assert(isInitialized());
					return m_dcel->halfEdge(m_outId);
				}

				/**
				 * Returns the incoming half-edge of this wedge.
				 * \return The incoming half-edge.
				 */
				HalfEdge incomingHalfEdge() const {
					assert(isInitialized());
					return m_dcel->halfEdge(m_outId).previous();
				}

				/**
				 * Returns the vertex this wedge is adjacent to.
				 * \return The vertex of this wedge.
				 */
				Vertex vertex() const {
					assert(isInitialized());
					return outgoingHalfEdge().origin();
				}

				/**
				 * Returns the face this wedge is a part of.
				 * \return The face of this wedge.
				 */
				Face face() const {
					assert(isInitialized());
					return outgoingHalfEdge().incidentFace();
				}

			private:

				/**
				 * Creates a new wedge.
				 *
				 * \param dcel The DCEL instance.
				 * \param outId The ID of the first half-edge.
				 */
				Wedge(Dcel<VertexData, HalfEdgeData, FaceData>* dcel,
						int outId) : m_dcel(dcel), m_outId(outId) {
				}

				/**
				 * The DCEL this wedge is a part of.
				 */
				Dcel<VertexData, HalfEdgeData, FaceData>* m_dcel = nullptr;

				/**
				 * The index of the first half-edge in the `Dcel::halfEdges()`
				 * list, or `-1` if this face is invalid.
				 */
				int m_outId = -1;
		};

		/**
		 * \name Constructors
		 */

		/**
		 * Creates an empty DCEL.
		 */
		Dcel() {
		}

		/**
		 * \name Accessors
		 */
		///@{

		/**
		 * Returns the vertex with a certain ID.
		 *
		 * \param index The ID of the vertex.
		 * \return The vertex with that ID, or an uninitialized vertex if
		 * \c index is out of bounds.
		 */
		Vertex vertex(int index) {
			if (index < 0 || index >= vertexCount()) {
				return Vertex();
			}
			return Vertex(this, index);
		}

		/**
		 * Returns the number of vertices in this DCEL.
		 * \return The number of vertices.
		 */
		int vertexCount() const {
			return m_vertices.size();
		}

		/**
		 * Returns the half-edge with a certain ID.
		 *
		 * \param index The ID of the half-edge.
		 * \return The half-edge with that ID, or an uninitialized half-edge
		 * if \c index is out of bounds.
		 */
		HalfEdge halfEdge(int index) {
			if (index < 0 || index >= halfEdgeCount()) {
				return HalfEdge();
			}
			return HalfEdge(this, index);
		}

		/**
		 * Returns the number of half-edges in this DCEL.
		 * \return The number of half-edges.
		 */
		int halfEdgeCount() const {
			return m_halfEdges.size();
		}

		/**
		 * Returns the face with a certain ID.
		 *
		 * \param index The ID of the face.
		 * \return The face with that ID, or an uninitialized face if \c index
		 * is out of bounds.
		 */
		Face face(int index) {
			if (index < 0 || index >= faceCount()) {
				return Face();
			}
			return Face(this, index);
		}

		/**
		 * Returns the number of faces in this DCEL.
		 * \return The number of faces.
		 */
		int faceCount() const {
			return m_faces.size();
		}

		/**
		 * Returns the wedge corresponding to some outgoing half-edge.
		 *
		 * \param out The outgoing half-edge.
		 * \return The wedge.
		 */
		Wedge wedge(HalfEdge out) {
			assert(out.isInitialized());
			assert(out.m_dcel == this);
			return Wedge(this, out.id());
		}

		/**
		 * Searches for the wedge around a certain vertex that has the given
		 * face.
		 *
		 * Note that there may be more than one such wedge. In such a case, an
		 * arbitrary wedge is returned.
		 *
		 * \param v The vertex.
		 * \param f The face to look for.
		 * \return The resulting wedge, or an uninitialized Wedge if such a
		 * wedge does not exist.
		 */
		Wedge wedge(Vertex v, Face f) {
			assert(v.isInitialized());
			assert(f.isInitialized());

			HalfEdge e = v.outgoing();
			do {
				if (e.incidentFace() == f) {
					return wedge(e);
				}
				e = e.nextOutgoing();
			} while (e != v.outgoing());

			return Wedge();
		}

		///@}

		/**
		 * \name High-level operations
		 */
		///@{

		/**
		 * Adds a new, unconnected vertex to the DCEL.
		 *
		 * \return The new vertex.
		 */
		Vertex addVertex() {
			int id = m_vertices.size();
			VertexImpl v;
			m_vertices.push_back(v);
			return vertex(id);
		}

		/**
		 * Adds a single half-edge. This sets the origin pointer.
		 * To make the structure consistent, it is still needed to create
		 * the other half-edge, set the twin pointer, set the
		 * previous and next pointers and the incident face pointer.
		 *
		 * \param origin The origin of the half-edge.
		 * \return The new half-edge.
		 */
		HalfEdge addHalfEdge(Vertex origin) {
			assert(origin.isInitialized());
			assert(origin.m_dcel == this);

			int id = m_halfEdges.size();

			HalfEdgeImpl e;
			m_halfEdges.push_back(e);

			halfEdge(id).setOrigin(origin);

			return halfEdge(id);
		}

		/**
		 * Adds a pair of half-edges. This sets the origin and twin pointers.
		 * To make the structure consistent, it is still needed to set the
		 * previous and next pointers and the incident face pointer.
		 *
		 * \param origin The origin of the first half-edge; the destination of
		 * the second half-edge.
		 * \param destination The destination of the second half-edge; the
		 * origin of the second half-edge.
		 * \return The first half-edge (from `origin` to `destination`).
		 * The second half-edge is the twin of the returned half-edge.
		 */
		HalfEdge addEdge(Vertex origin, Vertex destination) {
			assert(origin.isInitialized());
			assert(destination.isInitialized());

			assert(origin.m_dcel == this);
			assert(destination.m_dcel == this);

			HalfEdge forward = addHalfEdge(origin);
			HalfEdge backward = addHalfEdge(destination);
			forward.setTwin(backward);

			return forward;
		}

		/**
		 * Adds a face. This sets the boundary pointer. To make the
		 * structure consistent, the edges on the boundary still need to
		 * have their incident-face pointers set.
		 *
		 * \param boundary An edge on the boundary of the face.
		 * \return The new face.
		 *
		 * \note It is probably easier to use addFaces() to add all faces at
		 * once.
		 */
		Face addFace(HalfEdge boundary) {
			assert(boundary.isInitialized());
			assert(boundary.m_dcel == this);

			int id = m_faces.size();

			FaceImpl f;

			m_faces.push_back(f);

			face(id).setBoundary(boundary);

			return face(id);
		}

		/**
		 * Given a DCEL without any faces, adds faces based on the existing
		 * vertices and half-edges.
		 *
		 * This also sets the incident-face pointers for the incident
		 * half-edges.
		 */
		void addFaces() {
			std::vector<bool> marked(halfEdgeCount(), false);
			for (int i = 0; i < halfEdgeCount(); i++) {
				HalfEdge e = halfEdge(i);
				if (marked[e.id()]) {
					continue;
				}
				Face face = addFace(e);
				HalfEdge e2 = e;
				do {
					marked[e2.id()] = true;
					e2.setIncidentFace(face);
					e2 = e2.next();
				} while (e2 != e);
			}
		}

		/**
		 * Reassigns IDs to all vertices, half-edges and faces in this DCEL to
		 * remove gaps in the numbering caused by removing elements.
		 *
		 * When removing an element, it stays stored with the same ID, and its
		 * memory is not cleared. After executing this method all such removed
		 * elements are gone permanently from the DCEL.
		 *
		 * Executing this method invalidates any Vertex, HalfEdge and Face
		 * objects from this DCEL.
		 */
		void compact() {
			assert(isValid(true));

			// first just copy every non-removed element to a new list
			// while maintaining mappings from old to new IDs
			std::vector<VertexImpl> newVertices;
			std::vector<int> vertexMapping(vertexCount(), -1);
			for (int i = 0; i < vertexCount(); i++) {
				VertexImpl v = m_vertices[i];
				if (!v.removed) {
					vertexMapping[i] = newVertices.size();
					newVertices.push_back(v);
				}
			}

			std::vector<HalfEdgeImpl> newHalfEdges;
			std::vector<int> halfEdgeMapping(halfEdgeCount(), -1);
			for (int i = 0; i < halfEdgeCount(); i++) {
				HalfEdgeImpl e = m_halfEdges[i];
				if (!e.removed) {
					halfEdgeMapping[i] = newHalfEdges.size();
					newHalfEdges.push_back(e);
				}
			}

			std::vector<FaceImpl> newFaces;
			std::vector<int> faceMapping(faceCount(), -1);
			for (int i = 0; i < faceCount(); i++) {
				FaceImpl e = m_faces[i];
				if (!e.removed) {
					faceMapping[i] = newFaces.size();
					newFaces.push_back(e);
				}
			}

			// now we need to apply the mappings to everything
			for (VertexImpl& v : newVertices) {
				v.m_outgoing = halfEdgeMapping[v.m_outgoing];
				assert(v.m_outgoing != -1);
			}

			for (HalfEdgeImpl& e : newHalfEdges) {
				e.m_origin = vertexMapping[e.m_origin];
				assert(e.m_origin != -1);
				e.m_twin = halfEdgeMapping[e.m_twin];
				assert(e.m_twin != -1);
				e.m_next = halfEdgeMapping[e.m_next];
				assert(e.m_next != -1);
				e.m_previous = halfEdgeMapping[e.m_previous];
				assert(e.m_previous != -1);
				e.m_incidentFace = faceMapping[e.m_incidentFace];
				assert(e.m_incidentFace != -1);
			}

			for (FaceImpl& f : newFaces) {
				f.m_boundary = halfEdgeMapping[f.m_boundary];
				assert(f.m_boundary != -1);
			}

			// save the result
			m_vertices = newVertices;
			m_halfEdges = newHalfEdges;
			m_faces = newFaces;

			assert(isValid(true));
		}

		/**
		 * A path consisting of DCEL half-edges.
		 */
		class Path {

			public:

				/**
				 * Constructs an empty path.
				 */
				Path() {
				}

				/**
				 * Returns whether this path is empty, that is, if it does not
				 * contain any edges.
				 *
				 * \return \c true if this path is empty; \c false if it is not
				 * (that is, it contains edges).
				 */
				bool empty() const {
					return m_edges.empty();
				}

				/**
				 * Adds an half-edge to the end of this path.
				 *
				 * The origin of this edge should be equal to the destination
				 * of the last edge currently in the path. If not,
				 * behavior is undefined.
				 *
				 * \param e The edge to add.
				 */
				void addEdge(HalfEdge e) {
					assert(e.isInitialized());
					if (!empty()) {
						assert(e.origin() == m_edges.back().destination());
					}
					m_edges.push_back(e);
				}

				/**
				 * Returns a list of the half-edges in this path, in order.
				 * \return The edges in this path.
				 */
				const std::vector<HalfEdge>& edges() const {
					return m_edges;
				}

				/**
				 * Returns the number of edges in this path.
				 * \return The length of this path.
				 */
				int length() const {
					return m_edges.size();
				}

				/**
				 * Performs an action for all vertices on this path, in order
				 * from the beginning to the end of the path.
				 *
				 * \param f A function to call for every vertex.
				 */
				void forAllVertices(std::function<void(Vertex)> f) const {
					if (empty()) {
						return;
					}
					for (int i = 0; i < edges().size(); i++) {
						f(edges()[i].origin());
					}
					f(edges()[edges().size() - 1].destination());
				}

				/**
				 * Returns the first vertex of this path.
				 *
				 * \return The origin of this path. If this path is empty,
				 * returns an uninitialized vertex.
				 */
				Vertex origin() const {
					if (empty()) {
						return Vertex();
					}
					return edges()[0].origin();
				}

				/**
				 * Returns the last vertex of this path.
				 *
				 * \return The destination of this path. If this path is empty,
				 * returns an uninitialized vertex.
				 */
				Vertex destination() const {
					if (empty()) {
						return Vertex();
					}
					return edges()[edges().size() - 1].destination();
				}

				/**
				 * Returns the reversed variant of this path.
				 * \return The reversed path.
				 */
				Path reversed() const {
					Path result;
					for (int i = m_edges.size() - 1; i >= 0; --i) {
						result.addEdge(m_edges[i].twin());
					}
					return result;
				}

				/**
				 * Outputs a representation of a path to an output stream,
				 * for debugging purposes.
				 *
				 * \param os The output stream.
				 * \param p The path to output.
				 * \return The output stream.
				 */
				friend std::ostream& operator<<(std::ostream& os,
				                                Path const& p) {
					os << "path of length " << p.length()
					   << " with vertices: ";
					p.forAllVertices([&os](Vertex v) {
						os << v.id() << " ";
					});
					return os;
				}

			private:

				/**
				 * The half-edges in the path, in order.
				 */
				std::vector<HalfEdge> m_edges;
		};

		///@}

	public:

		/**
		 * \name Debugging
		 */
		///@{

		/**
		 * Performs a sanity check on this DCEL. This cannot detect all
		 * problems, but it detects unset pointers, pointers to removed
		 * elements, and the following situations:
		 *
		 *  * the outgoing half-edge of a vertex does not have that vertex
		 *    as the origin;
		 *  * twin half-edges are not pointing at each other;
		 *  * next and previous half-edges are not pointing at each other;
		 *  * the boundary edge of a face does not have that face as its
		 *    incident face.
		 *
		 * This method is meant for debugging purposes only. If it returns
		 * `false`, it will output information about the problem to `std::cout`.
		 *
		 * \note This method is designed not to crash, even if the DCEL is
		 * invalid.
		 *
		 * \param checkFaces Whether to check faces. If calling this method
		 * before faces are added, pass `false` here.
		 * \return `true` if this DCEL is valid. `false` if it is invalid.
		 */
		bool isValid(bool checkFaces) const {
			for (int i = 0; i < m_vertices.size(); i++) {
				VertexImpl v = m_vertices[i];
				if (v.removed) {
					continue;
				}
				if (v.m_outgoing == -1) {
					std::cout << "Dcel::isValid(): vertex " << i
							  << " invalid: outgoing == -1" << std::endl;
					return false;
				}
				if (m_halfEdges[v.m_outgoing].m_origin != i) {
					std::cout << "Dcel::isValid(): vertex " << i
							  << " invalid: outgoing.origin == "
							  << m_halfEdges[v.m_outgoing].m_origin
							  << " != " << i << std::endl;
					return false;
				}
				if (m_halfEdges[v.m_outgoing].removed) {
					std::cout << "Dcel::isValid(): vertex " << i
					          << " invalid: outgoing == "
					          << v.m_outgoing
					          << " which is removed" << std::endl;
					return false;
				}
			}

			for (int i = 0; i < m_halfEdges.size(); i++) {
				HalfEdgeImpl e = m_halfEdges[i];
				if (e.removed) {
					continue;
				}
				if (e.m_next == -1) {
					std::cout << "Dcel::isValid(): half-edge " << i
							  << " invalid: next == -1" << std::endl;
					return false;
				}
				if (e.m_previous == -1) {
					std::cout << "Dcel::isValid(): half-edge " << i
							  << " invalid: previous == -1" << std::endl;
					return false;
				}
				if (m_halfEdges[e.m_previous].m_next != i) {
					std::cout << "Dcel::isValid(): half-edge " << i
							  << " invalid: previous.next == "
							  << m_halfEdges[e.m_previous].m_next
							  << " != " << i << std::endl;
					return false;
				}
				if (m_halfEdges[e.m_previous].removed) {
					std::cout << "Dcel::isValid(): half-edge " << i
					          << " invalid: previous == "
					          << e.m_previous
					          << " which is removed" << std::endl;
					return false;
				}
				if (m_halfEdges[e.m_next].m_previous != i) {
					std::cout << "Dcel::isValid(): half-edge " << i
							  << " invalid: next.previous == "
							  << m_halfEdges[e.m_next].m_previous
							  << " != " << i << std::endl;
					return false;
				}
				if (m_halfEdges[e.m_next].removed) {
					std::cout << "Dcel::isValid(): half-edge " << i
					          << " invalid: next == "
					          << e.m_next
					          << " which is removed" << std::endl;
					return false;
				}
				if (e.m_origin== -1) {
					std::cout << "Dcel::isValid(): half-edge " << i
							  << " invalid: origin == -1" << std::endl;
					return false;
				}
				if (m_vertices[e.m_origin].removed) {
					std::cout << "Dcel::isValid(): half-edge " << i
					          << " invalid: origin == "
					          << e.m_origin
					          << " which is removed" << std::endl;
					return false;
				}
				if (e.m_twin == -1) {
					std::cout << "Dcel::isValid(): half-edge " << i
							  << " invalid: twin == -1" << std::endl;
					return false;
				}
				if (m_halfEdges[e.m_twin].m_twin != i) {
					std::cout << "Dcel::isValid(): half-edge " << i
							  << " invalid: twin.twin == "
							  << m_halfEdges[e.m_twin].m_twin
							  << " != " << i << std::endl;
					return false;
				}
				if (m_halfEdges[e.m_twin].removed) {
					std::cout << "Dcel::isValid(): half-edge " << i
					          << " invalid: twin == "
					          << e.m_twin
					          << " which is removed" << std::endl;
					return false;
				}

				if (checkFaces) {
					if (e.m_incidentFace == -1) {
						std::cout << "Dcel::isValid(): half-edge " << i
								  << " invalid: incidentFace == -1"
								  << std::endl;
						return false;
					}
					if (m_faces[e.m_incidentFace].removed) {
						std::cout << "Dcel::isValid(): half-edge " << i
						          << " invalid: incidentFace == "
						          << e.m_incidentFace
						          << " which is removed" << std::endl;
						return false;
					}
				}
			}

			if (checkFaces) {
				for (int i = 0; i < m_faces.size(); i++) {
					FaceImpl f = m_faces[i];
					if (f.removed) {
						continue;
					}
					if (f.m_boundary == -1) {
						std::cout << "Dcel::isValid(): face " << i
								  << " invalid: boundary == -1" << std::endl;
						return false;
					}
					if (m_halfEdges[f.m_boundary].m_incidentFace != i) {
						std::cout << "Dcel::isValid(): face " << i
								  << " invalid: boundary.incidentFace == "
								  << m_halfEdges[f.m_boundary].m_incidentFace
								  << " != " << i << std::endl;
						return false;
					}
					if (m_halfEdges[f.m_boundary].removed) {
						std::cout << "Dcel::isValid(): face " << i
						          << " invalid: boundary == "
						          << f.m_boundary
						          << " which is removed" << std::endl;
						return false;
					}
				}
			}

			return true;
		}
		
		/**
		 * Prints a representation of this DCEL for debugging purposes.
		 *
		 * If VertexData, HalfEdgeData, and/or FaceData have their own
		 * `output(std::ostream&)` method(s), then these are called for each
		 * vertex, half-edge, and/or face, to augment the DCEL's own output.
		 * This works best if the output of these methods is short and contains
		 * no line breaks.
		 *
		 * \note This method is designed not to crash, even if the DCEL is
		 * invalid.
		 *
		 * \param out The output stream to print to.
		 */
		void output(std::ostream& out) {
			out << "Vertices:\n";
			out << "--- id ---   --- outgoing ---\n";
			for (int i = 0; i < vertexCount(); i++) {
				VertexImpl v = m_vertices[i];
				out << std::setw(10) << i << "   " <<
				    std::setw(16) << v.m_outgoing <<
				    std::setw(4) << (v.removed ? "x" : "");
				if constexpr (requires {v.m_data.output(out);}) {
					out << "    ";
					v.m_data.output(out);
				}
				out << "\n";
			}

			out << "Half-edges:\n";
			out << "--- id ---   --- origin ---   --- previous ---   "
			       "--- next ---   --- twin ---   --- incidentFace ---\n";
			for (int i = 0; i < halfEdgeCount(); i++) {
				HalfEdgeImpl e = m_halfEdges[i];
				out << std::setw(10) << i << "   " <<
				       std::setw(14) << e.m_origin << "   " <<
				       std::setw(16) << e.m_previous << "   " <<
				       std::setw(12) << e.m_next << "   " <<
				       std::setw(12) << e.m_twin << "   " <<
				       std::setw(20) << e.m_incidentFace <<
				       std::setw(4) << (e.removed ? "x" : "");
				if constexpr (requires {e.m_data.output(out);}) {
					out << "    ";
					e.m_data.output(out);
				}
				out << "\n";
			}

			out << "Faces:\n";
			out << "--- id ---   --- boundary ---\n";
			for (int i = 0; i < faceCount(); i++) {
				FaceImpl f = m_faces[i];
				out << std::setw(10) << i << "   " <<
				       std::setw(16) << f.m_boundary <<
				       std::setw(4) << (f.removed ? "x" : "");
				if constexpr (requires {f.m_data.output(out);}) {
					out << "    ";
					f.m_data.output(out);
				}
				out << "\n";
			}

			out << std::flush;
		}
		///@}

	protected:

		/**
		 * The list of vertices.
		 */
		std::vector<VertexImpl> m_vertices;

		/**
		 * The list of half-edges.
		 */
		std::vector<HalfEdgeImpl> m_halfEdges;

		/**
		 * The list of faces.
		 */
		std::vector<FaceImpl> m_faces;
};

#endif /* DCEL_H */
