#ifndef VERTEX_TYPE_H
#define VERTEX_TYPE_H

/**
 * Possible types of a vertex.
 */
enum class VertexType {

	/**
	 * This vertex is a regular point (none of the other cases).
	 */
	regular,

	/**
	 * This vertex is a minimum; all of its neighbors are higher.
	 */
	minimum,

	/**
	 * This vertex is a saddle; it has more than one descending and
	 * ascending wedge.
	 */
	saddle,

	/**
	 * This vertex is a maximum; all of its neighbors are lower.
	 */
	maximum,

	/**
	 * This vertex is disconnected (does not have any neighbors).
	 */
	disconnected
};

#endif /* VERTEX_TYPE_H */

