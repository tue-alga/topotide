#ifndef CARVEDCOMPLEX_H
#define CARVEDCOMPLEX_H

#include <limits>
#include <vector>

#include "inputdcel.h"
#include "mscomplex.h"

/**
 * A vertex in a carved Morse-Smale complex.
 */
class CarvedVertex {

	public:

		/**
		 * The original Morse-Smale vertex, before carving.
		 */
		MsComplex::Vertex msVertex;

		/**
		 * ID of this vertex within the connected component.
		 */
		int temporaryId;

		/**
		 * The directions towards the lowest path to the source and the sink for
		 * a vertex.
		 */
		struct LowestPathDirections {

			/**
			 * The ID of the outgoing half-edge of the vertex that forms the
			 * first edge of the lowest path towards the source.
			 */
			int directionToSource;

			/**
			 * The ID of the outgoing half-edge of the vertex that forms the
			 * first edge of the lowest path towards the sink.
			 */
			int directionToSink;
		};

		/**
		 * The directions towards the source and the sink.
		 */
		LowestPathDirections lptDirections;

		/**
		 * The ID of the outgoing half-edge of the wedge that is open for this
		 * vertex, or `-1` if this vertex is closed.
		 */
		int openWedge;
};

/**
 * A half-edge in a carved Morse-Smale complex.
 */
class CarvedHalfEdge {

	public:

		/**
		 * The original Morse-Smale half-edge, before carving.
		 */
		MsComplex::HalfEdge msHalfEdge;

		/**
		 * Whether this half-edge is part of the lowest path tree that we are
		 * currently computing.
		 */
		bool inLpt;
};

/**
 * A face in a carved Morse-Smale complex.
 */
class CarvedFace {

	public:

		/**
		 * The ID of the Morse-Smale minimum that we end up in when taking the
		 * steepest-descent path from the maximum in the InputDcel.
		 */
		int lowestPathVertex = -1;

		/**
		 * The persistence of the maximum in this face.
		 */
		double persistence = std::numeric_limits<double>::infinity();
};

/**
 * A Morse-Smale complex that is being carved by the striation algorithm.
 *
 * To carve a CarvedComplex, use the carveEdges() method.
 *
 * This is a separate class from MsComplex, because many attributes of the
 * vertices, edges and faces of the Morse-Smale complex do not need to be
 * maintained while carving. As carving duplicates many vertices and edges,
 * this saves on memory usage (and copying time).
 */
class CarvedComplex : public Dcel<CarvedVertex, CarvedHalfEdge, CarvedFace> {

	public:

		/**
		 * Creates an empty carved complex.
		 */
		CarvedComplex();

		/**
		 * Creates a copy of a Morse-Smale complex.
		 * \param msc The Morse-Smale complex to copy.
		 */
		CarvedComplex(MsComplex& msc);

		/**
		 * Carves a set of edges in the DCEL.
		 *
		 * \image html dcel-carve-edges.png
		 *
		 * The set of edges does not need to represent a path, but it should
		 * form some connected structure. It must not contain the same half-edge
		 * twice; also, it must not contain both half-edges of a twin pair.
		 *
		 * The starting wedge and the ending wedge need to belong to the same
		 * face, otherwise behavior is undefined.
		 *
		 * \note This method can leave the outer face with more than one
		 * connected component. In that case, as this DCEL implementation does
		 * not support multiple boundary pointers for a face, some part of the
		 * DCEL is not reachable anymore by traversing it.
		 *
		 * \param edges The set of edges to carve.
		 * \param start The starting wedge.
		 * \param end The ending wedge.
		 * \param onSplit A function that is called every time a vertex is split
		 * into two parts. This is meant to update references to these split
		 * vertices. For example, if the DCEL in each face stores references to
		 * a vertex on its boundary, the `onSplit` function can update those
		 * references after a vertex split.
		 */
		void carveEdges(std::vector<HalfEdge> edges,
		                Wedge start,
		                Wedge end,
		                const std::function<void(Vertex v, Vertex splitV)>&
		                        onSplit = nullptr);

	private:

		/**
		 * Carves a single edge.
		 *
		 * \note This is a helper method for carveEdges().
		 *
		 * \param edge The edge to carve.
		 * \param outerFace The outer face.
		 * \param onSplit A function that is called every time a vertex is split
		 * into two parts. This is meant to update references to these split
		 * vertices. For example, if the DCEL in each face stores references to
		 * a vertex on its boundary, the `onSplit` function can update those
		 * references after a vertex split.
		 */
		void carveEdge(HalfEdge edge,
		               Face outerFace,
		               const std::function<void(Vertex v, Vertex splitV)>&
		                       onSplit = nullptr);
};

#endif /* CARVEDCOMPLEX_H */
