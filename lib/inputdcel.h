#ifndef INPUTDCEL_H
#define INPUTDCEL_H

#include <optional>

#include "boundarystatus.h"
#include "dcel.h"
#include "inputgraph.h"
#include "point.h"
#include "piecewiselinearfunction.h"

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

		/// Whether this vertex is on the boundary.
		BoundaryStatus boundaryStatus = BoundaryStatus::IMPERMEABLE;

		/// When `boundaryStatus == BoundaryStatus::PERMEABLE`, this stores the
		/// index of that permeable region.
		std::optional<int> permeableRegion;

		/**
		 * Prints the coordinate and type of this vertex data, for debugging
		 * purposes (see \ref Dcel::output(std::ostream&)).
		 */
		void output(std::ostream& out);
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

		/**
		 * The volume of the part of the red tree that arises when we cut the
		 * red tree at this half-edge. Set by the \ref FingerFinder.
		 */
		PiecewiseLinearFunction volumeAbove;

		/// Whether this edge is on the boundary. This value is identical for
		/// both twin half-edges.
		BoundaryStatus boundaryStatus = BoundaryStatus::INTERIOR;

		/// When `boundaryStatus == BoundaryStatus::PERMEABLE`, this stores the
		/// index of that permeable region.
		std::optional<int> permeableRegion;
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

		/**
		 * The ID of the half edge that forms the top edge of this face. Only
		 * defined for leaves in the red tree. Set by the \ref FingerFinder.
		 */
		int topEdge = -1;

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
		/// Holds face IDs
		std::vector<int> pathToTopEdge;
		std::vector<int> spurFaces;
		/// Holds vertex IDs
		std::vector<int> spurBoundary;

		double flankingHeight;
		bool isSignificant = false;
#endif
};

/**
 * A DCEL that we generated from the InputGraph.
 */
class InputDcel : public Dcel<InputDcelVertex, InputDcelHalfEdge, InputDcelFace> {

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
		 * Sets the center coordinates for all edges and faces.
		 */
		void setEdgeAndFaceCoordinates();

		/**
		 * Computes vertex-edge and edge-face gradient pairs.
		 */
		void computeGradientFlow();

		/**
		 * Checks if this vertex is critical (i.e., if it is a minimum).
		 */
		bool isCritical(Vertex vertex) const;

		/**
		 * Checks if this half-edge is critical (i.e., if it is a saddle).
		 */
		bool isCritical(HalfEdge edge) const;

		/**
		 * Checks if this face is critical (i.e., if it is a maximum).
		 */
		bool isCritical(Face face) const;

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
		 * \param face The face to return the sand function of.
		 * \return The resulting sand function.
		 */
		PiecewiseLinearFunction volumeAbove(Face face);

		/**
		 * Returns the vertex at the given position.
		 *
		 * \param x The x-coordinate.
		 * \param y The y-coordinate.
		 * \return The vertex at that position, or an uninitialized vertex if
		 * the coordinate is out of bounds.
		 */
		Vertex vertexAt(double x, double y);

		void pair(Vertex v, HalfEdge e);
		void pair(HalfEdge e, Face f);

		void unpair(Vertex v, HalfEdge e);
		void unpair(HalfEdge e, Face f);

		bool isBlueLeaf(Vertex v) const;
		bool isRedLeaf(Face f) const;

		/// Returns the outer face.
		Face outerFace();

	private:
		int m_outerFaceId;
};

#endif // INPUTDCEL_H
