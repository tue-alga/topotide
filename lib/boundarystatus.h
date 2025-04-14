#ifndef BOUNDARYSTATUS_H
#define BOUNDARYSTATUS_H

/// Denotes whether a vertex or edge is on the boundary of the analysis region,
/// and if so, if it is on a permeable or an impermeable part of the boundary.
enum class BoundaryStatus {
	/// This vertex or edge is not on the boundary; i.e., it is in the interior
	/// of the analysis region.
	INTERIOR,
	/// This vertex or edge is on the boundary and on a permeable region. If a
	/// vertex is the endpoint of a permeable region (which means that it is
	/// both on a permeable and an impermeable part) the vertex is marked as
	/// `PERMEABLE` too.
	PERMEABLE,
	/// This vertex or edge is on the boundary and on an impermeable region.
	IMPERMEABLE
};

#endif
