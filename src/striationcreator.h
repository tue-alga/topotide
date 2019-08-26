#ifndef STRIATIONCREATOR_H
#define STRIATIONCREATOR_H

#include <functional>
#include <unordered_set>

#include "carvedcomplex.h"
#include "mscomplex.h"
#include "striation.h"
#include "units.h"

/**
 * An algorithm for computing a striation from a Morse-Smale complex.
 *
 * \note This is separate from the Striation class itself, so it is possible
 * to obtain progress information while the striation is being computed.
 */
class StriationCreator {

	public:

		/**
		 * Creates a striation creator that can create a striation from a
		 * Morse-Smale complex.
		 *
		 * \note Call create() to actually execute the algorithm.
		 *
		 * \param msc The Morse-Smale complex to create the striation from.
		 * \param striation An empty striation to store the result in.
		 * \param units Real-world units (needed for computing edge lengths).
		 * \param alongLowestPath Whether to consider only faces along the
		 * lowest path for carving (the hybrid striation method).
		 * \param progressListener A function that is called when a progress
		 * update is available.
		 */
		StriationCreator(MsComplex& msc, Striation* striation,
		                 Units units,
		                 bool alongLowestPath,
		                 std::function<void(int)> progressListener = nullptr);

		/**
		 * Creates the striation.
		 */
		void create();

	private:

		/**
		 * The Morse-Smale complex we are going to compute a striation for.
		 */
		MsComplex& msc;

		/**
		 * The carved version of the Morse-Smale complex.
		 */
		CarvedComplex cc;

		/**
		 * The striation that we are going to store our result in.
		 */
		Striation* striation;

		/**
		 * A piece of the striation.
		 */
		struct StriationPiece {

			/**
			 * The source of this piece.
			 */
			CarvedComplex::Wedge source;

			/**
			 * The sink of this piece.
			 */
			CarvedComplex::Wedge sink;

			/**
			 * The ID of the parent item in the striation.
			 */
			int parent;

			/**
			 * `true` if this is the top child of its parent; `false` if it is
			 * the bottom child.
			 */
			bool isTop;

			/**
			 * Constructs a striation piece.
			 *
			 * \param source The source of this piece.
			 * \param sink The sink of this piece.
			 * \param parent The ID of the parent item in the striation.
			 * \param isTop `true` if this is the top child of its parent;
			 * `false` if it is the bottom child.
			 */
			StriationPiece(CarvedComplex::Wedge source,
			           CarvedComplex::Wedge sink,
			           int parent,
			           bool isTop) :
			    source(source), sink(sink), parent(parent), isTop(isTop) {
			}
		};

		/**
		 * Finds the source wedge and returns it. The source wedge is the wedge
		 * of the source that is part of the outer face.
		 *
		 * \return The source wedge.
		 */
		CarvedComplex::Wedge findSourceWedge();

		/**
		 * Finds the sink wedge and returns it. The sink wedge is the wedge
		 * of the sink that is part of the outer face.
		 *
		 * \return The sink wedge.
		 */
		CarvedComplex::Wedge findSinkWedge();

		/**
		 * Returns the set of all faces in the connected component that the
		 * given vertex is in.
		 *
		 * \param v The vertex to start searching from.
		 * \return The set of all reachable faces from this vertex.
		 */
		std::unordered_set<int> findReachableFaceIds(CarvedComplex::Vertex v);

		/**
		 * Returns the set of all faces that have an edge in common with the
		 * lowest path between the two given vertices.
		 *
		 * \param source The start vertex of the lowest path.
		 * \param sink The end vertex of the lowest path.
		 * \return The set of all faces along the lowest path from \c source to
		 * \c sink.
		 */
		std::unordered_set<int> findFacesAlongLowestPath(
		        CarvedComplex::Vertex source,
		        CarvedComplex::Vertex sink);

		/**
		 * The result of a single carve around a Morse-Smale face.
		 */
		struct CarveResult {
			CarvedComplex::Wedge topSourceWedge;
			CarvedComplex::Wedge topSinkWedge;
			CarvedComplex::Wedge bottomSourceWedge;
			CarvedComplex::Wedge bottomSinkWedge;
		};

		/**
		 * Carves the Morse-Smale complex along the lowest paths around the
		 * given Morse-Smale face.
		 *
		 * \param f The face to carve around.
		 * \param source The source vertex.
		 * \param sink The sink vertex.
		 * \return The result of the carve.
		 */
		CarveResult carveAround(CarvedComplex::Face f,
		                        CarvedComplex::Wedge source,
		                        CarvedComplex::Wedge sink);

		/**
		 * Finds the carving paths around the given MS-face.
		 *
		 * \param f The face to carve around.
		 * \param source The source vertex.
		 * \param sink The sink vertex.
		 * \return A set of half-edge IDs that we need to carve to carve around
		 * `f`.
		 */
		std::unordered_set<int> findCarveEdgeIdsAround(
		        CarvedComplex::Face f,
		        CarvedComplex::Vertex source,
		        CarvedComplex::Vertex sink);

		/**
		 * Inserts an edge in a set of edge IDs. To ensure that two half-edges
		 * of a twin pair can never both be inserted, this method always inserts
		 * the minimum ID of the given half-edge and its twin.
		 *
		 * \param edgeSet The set.
		 * \param e One of the half-edges of the edge to insert.
		 */
		void insertEdge(std::unordered_set<int>& edgeSet,
		                CarvedComplex::HalfEdge e);

		/**
		 * Finds the outer face of the Morse-Smale complex, that is, the face
		 * with persistence infinity.
		 *
		 * \return The outer face, or an uninitialized Face if there is no
		 * outer face.
		 */
		CarvedComplex::Face findOuterFace();

		/**
		 * The outer face of the Morse-Smale complex, that is, the face with
		 * persistence infinity.
		 */
		CarvedComplex::Face outerFace;

		/**
		 * The real-world units.
		 */
		Units m_units;

		/**
		 * Whether to consider only faces along the lowest path for carving
		 * (the hybrid striation method).
		 */
		bool m_alongLowestPath;

		/**
		 * Calls the progress listener, if one is set.
		 * \param progress The progress value to report.
		 */
		void signalProgress(int progress);

		/**
		 * A function that is called when there is a progress update.
		 */
		std::function<void(int)> progressListener;
};

#endif // STRIATIONCREATOR_H
