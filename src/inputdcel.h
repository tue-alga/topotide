#ifndef INPUTDCEL_H
#define INPUTDCEL_H

#include "dcel.h"
#include "inputgraph.h"
#include "point.h"
#include "piecewisecubicfunction.h"
#include "unit.h"

/**
 * A vertex in the input DCEL.
 */
class InputDcelVertex {

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
		 * The index of the steepest-descending outgoing edge.
		 */
		int steepestDescentEdge = -1;

		/**
		 * The index of the corresponding vertex in the Morse-Smale complex,
		 * if any.
		 */
		int msVertex = -1;
};

/**
 * A half-edge in the input DCEL.
 */
class InputDcelHalfEdge {

	public:

		/**
		 * Whether this half-edge is the steepest-descent edge within its
		 * descending wedge from the origin vertex.
		 */
		bool wedgeSteepest = false;

		/**
		 * Whether this half-edge is the overall steepest-descent edge from its
		 * origin vertex.
		 */
		bool steepest = false;

		/**
		 * For every half-edge with `wedgeSteepest == true` whose origin is a
		 * saddle, this stores the corresponding half-edge in the MsComplex
		 * from the saddle towards a minimum (that has this half-edge as the
		 * first half-edge).
		 *
		 * \note This is only used when computing a Morse-Smale complex from
		 * this DCEL.
		 *
		 * \todo This is declared `mutable` to be able to change it from the
		 * MsComplex constructor, that does not change any of the other members
		 * of this class and is hence given a `const` InputDcel. This should be
		 * changed.
		 */
		mutable int msHalfEdge = -1;

		/**
		 * For every edge that lies on a Morse-Smale edge, this stores the index
		 * of the face this edge is adjacent to.
		 *
		 * \note This is only used when computing a Morse-Smale complex from
		 * this DCEL.
		 *
		 * \todo This is declared `mutable` to be able to change it from the
		 * MsComplex constructor, that does not change any of the other members
		 * of this class and is hence given a `const` InputDcel. This should be
		 * changed.
		 */
		mutable int incidentMsFace = -1;
};

/**
 * A face in the input DCEL.
 */
class InputDcelFace {

	public:

		/**
		 * The ID of the Morse-Smale face that this DCEL face is a part of.
		 *
		 * \note This is only used when computing a Morse-Smale complex from
		 * this DCEL.
		 *
		 * \todo This is declared `mutable` to be able to change it from the
		 * MsComplex constructor, that does not change any of the other members
		 * of this class and is hence given a `const` InputDcel. This should be
		 * changed.
		 */
		mutable int msFace = -1;
};

/**
 * A DCEL that we generated from the InputGraph.
 */
class InputDcel : public Dcel<InputDcelVertex,
                              InputDcelHalfEdge,
                              InputDcelFace> {

	public:

		/**
		 * Creates an empty InputDcel.
		 */
		InputDcel();

		/**
		 * Creates a InputDcel based on an InputGraph.
		 *
		 * This creates as many vertices as there are in the graph, where
		 * vertices in the DCEL have the same IDs as the corresponding
		 * vertices in the graph. Furthermore, half-edges and faces are
		 * added.
		 *
		 * \param g The graph to use.
		 */
		InputDcel(const InputGraph& g);

		/**
		 * Splits all monkey saddles into non-degenerate saddles.
		 *
		 * For each monkey saddle, the splitting is done in such a way that
		 * its steepest-descent edge ends up in all newly created saddles.
		 */
		void splitMonkeySaddles();

		/**
		 * Computes the steepest-descent path, starting from the given saddle.
		 *
		 * \param startingEdge The ID of the edge to start with. This edge
		 * needs to be in descending direction.
		 * \return A list of IDs of edges on the path, in order (starting with
		 * `startingEdge`).
		 */
		InputDcel::Path steepestDescentPath(HalfEdge startingEdge);

		/**
		 * Checks whether the given half-edge is descending, that is, whether
		 * its destination is lower than its origin.
		 *
		 * \param edge The half-edge to check.
		 * \return `true` if `edge` is descending; `false` if `edge` is
		 * ascending.
		 * \see isAscending(HalfEdge)
		 */
		bool isDescending(HalfEdge edge);

		/**
		 * Checks whether the given half-edge is ascending, that is, whether
		 * its destination is higher than its origin.
		 *
		 * \param edge The half-edge to check.
		 * \return `true` if `edge` is ascending; `false` if `edge` is
		 * descending.
		 * \see isDescending(HalfEdge)
		 */
		bool isAscending(HalfEdge edge);

		/**
		 * Returns a piecewise cubic function representing the volume of sand
		 * above height *h* in the given face.
		 *
		 * \note This only works if `face` is triangular (which all faces in an
		 * InputDcel should be, anyway). It does not work on faces containing
		 * the source, the sink or the global maximum.
		 *
		 * \param face The face to return the sand function of.
		 * \return The resulting sand function.
		 */
		PiecewiseCubicFunction volumeAboveFunction(Face face);

		/**
		 * Returns a piecewise cubic function representing the volume of air
		 * below height *h* in the given face.
		 *
		 * \note This only works if `face` is triangular (which all faces in an
		 * InputDcel should be, anyway). It does not work on faces containing
		 * the source, the sink or the global maximum.
		 *
		 * \param face The face to return the air function of.
		 * \return The resulting air function.
		 */
		PiecewiseCubicFunction volumeBelowFunction(Face face);

		/**
		 * Returns the vertex at the given position.
		 *
		 * \param x The x-coordinate.
		 * \param y The y-coordinate.
		 * \return The vertex at that position, or an uninitialized vertex if
		 * the coordinate is out of bounds.
		 */
		Vertex vertexAt(double x, double y);
};

#endif // INPUTDCEL_H
