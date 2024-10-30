#ifndef MSCOMPLEX_H
#define MSCOMPLEX_H

#include <variant>
#include <vector>

#include "inputdcel.h"
#include "vertextype.h"

/**
 * A critical point (that is, a vertex) in the Morse-Smale complex.
 */
class MsVertex {

	public:

		/**
		 * The position (x and y-coordinate and height value) of this
		 * vertex.
		 */
		Point p;

		/**
		 * The type of this vertex.
		 */
		VertexType type;

		/**
		 * The corresponding vertex in the InputDcel.
		 */
	    std::variant<InputDcel::Vertex, InputDcel::HalfEdge, InputDcel::Face> inputDcelSimplex;
};

/**
 * A half-edge in the Morse-Smale complex.
 */
class MsHalfEdge {

	public:

		/**
		 * The path represented by this Morse-Smale edge.
		 *
		 * This is used only for saddle -> minimum edges; the DCEL path of a
		 * minimum -> saddle edge can be found by reversing the dcelPath of its
		 * twin.
		 */
		InputDcel::Path m_dcelPath;

		/**
		 * The δ-value for this half-edge, as computed by the persistence
		 * simplification.
		 */
		double m_delta;
};

/**
 * A descending Morse-Smale cell.
 *
 * Every Morse-Smale cell contains exactly one maximum in its interior.
 */
class MsFace {

	public:

		/**
		 * The maximum inside this face in the InputDcel.
		 */
		InputDcel::Face maximum;

		/**
		 * A list of faces within a Morse-Smale cell, given as IDs in the
		 * InputDcel.
		 */
		std::vector<InputDcel::Face> faces;

		/**
		 * The volume function of this face. For any height _h_,
		 * `volumeAbove(h)` returns the volume of sand within this face that is
		 * above the horizontal plane at height _h_.
		 */
		PiecewiseCubicFunction volumeAbove;
};

/**
 * A descending quasi-Morse-Smale complex. The vertices of this DCEL represent
 * critical points (minima and saddles) of the input graph. The edges represent
 * Morse-Smale edges, which are sequences of (wedge-)steepest-descent edges
 * from saddles to minima in the input graph.
 */
class MsComplex : public Dcel<MsVertex, MsHalfEdge, MsFace> {

	public:

		/**
		 * Creates an empty Morse-Smale complex.
		 */
		MsComplex();

		/**
		 * Returns the steepest-descent path represented by a Morse-Smale
		 * edge.
		 *
		 * If this is a saddle -> minimum edge, this returns `m_dcelPath`;
		 * if this is a minimum -> saddle edge, this returns a reversed version
		 * of it.
		 *
		 * \param e A Morse-Smale edge.
		 * \return The corresponding DCEL path of \c e.
		 */
		InputDcel::Path dcelPath(HalfEdge e);
};

#endif /* MSCOMPLEX_H */
