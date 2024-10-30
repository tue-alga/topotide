#ifndef INPUTDCEL_H
#define INPUTDCEL_H

#include "dcel.h"
#include "inputgraph.h"
#include "point.h"
#include "piecewisecubicfunction.h"

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
		 * Half-edge this vertex is gradient-paired with, if any.
		 */
		int pairedWithEdge = -1;

		/**
		 * The index of the corresponding vertex in the Morse-Smale complex,
		 * if any.
		 *
		 * \note This is only used when computing a Morse-Smale complex from
		 * this DCEL.
		 */
		int msVertex = -1;
};

/**
 * A half-edge in the input DCEL.
 */
class InputDcelHalfEdge {

	public:

		/**
		 * A point in the center of this edge.
		 */
		Point p;

		/**
		 * Whether this half-edge is the highest half-edge (lexicographically)
		 * of its incident face.
		 */
		bool highestOfFace = false;

		/**
		 * Whether this half-edge is the second-highest half-edge (lexicographically)
		 * of its incident face.
		 */
		bool secondHighestOfFace = false;

		/**
		 * Whether this half-edge is gradient-paired with its origin.
		 */
		bool pairedWithVertex = false;

		/**
		 * Whether this half-edge is gradient-paired with its incident face.
		 */
		bool pairedWithFace = false;

		/**
		 * The index of the corresponding half-edge in the Morse-Smale complex,
		 * if any.
		 *
		 * \note This is only used when computing a Morse-Smale complex from
		 * this DCEL.
		 */
		mutable int msVertex = -1;
};

/**
 * A face in the input DCEL.
 */
class InputDcelFace {

	public:

		/**
		 * A point in the center of this face.
		 */
		Point p;

		/**
		 * Boundary half-edge this face is gradient-paired with, if any.
		 */
		int pairedWithEdge = -1;

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
		 * Checks if this vertex is critical (i.e., if it is a minimum).
		 */
		bool isCritical(Vertex vertex) const;

		/**
		 * Checks if this half-edge is critical (i.e., if it is a saddle).
		 */
		bool isCritical(HalfEdge edge) const;

		/**
		 * Computes the gradient-descent path starting from the given saddle.
		 *
		 * \param startingEdge The ID of the saddle edge to start with, pointing
		 * in the direction of the gradient path to trace.
		 * \return The path (starting with `startingEdge`).
		 */
		InputDcel::Path gradientPath(HalfEdge startingEdge);

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
