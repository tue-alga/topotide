#ifndef UNIONFIND_H
#define UNIONFIND_H

#include <vector>

/**
 * A node in the union-find data structure.
 */
class UnionFindNode {

	public:

		/**
		 * Constructs a new node.
		 * \param point The point ID.
		 */
		UnionFindNode(int point);

		/**
		 * The point ID.
		 */
		int point;

		/**
		 * The parent node.
		 */
		int parent;
};

/**
 * A union-find data structure with path compression, but without the rank
 * heuristic. This means that merging is done deterministically: merging node 1
 * into node 2 makes 2 the representative; merging 2 into 1 makes 1 the
 * representative.
 */
class UnionFind {

	public:

		/**
		 * Creates a new union-find structure.
		 * \param points The number of elements.
		 */
		UnionFind(int points);

		/**
		 * Returns the node of element `p`.
		 *
		 * \param p The element to get the node from.
		 * \return The corresponding node.
		 */
		UnionFindNode& get(int p);

		/**
		 * Sets the node of element `p`.
		 *
		 * \param p The element to set the node of.
		 * \param node The new node.
		 */
		void set(int p, UnionFindNode node);

		/**
		 * Returns the representative of the given element.
		 *
		 * \param p The element to get the representative of.
		 * \return The representative.
		 */
		int findSet(int p);

		/**
		 * Merges `p2` into `p1`, so that `p1`'s representative becomes
		 * the representative of `p2` as well.
		 */
		void merge(int p1, int p2);

	private:

		/**
		 * The list of nodes.
		 */
		std::vector<UnionFindNode> _points;
};

#endif /* UNIONFIND_H */
