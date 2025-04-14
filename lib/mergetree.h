#ifndef MERGETREE_H
#define MERGETREE_H

#include <memory>
#include <optional>
#include <variant>

#include "mscomplex.h"

class MergeTree {

	public:
		MergeTree(const std::shared_ptr<MsComplex>& msc);

		class Node {
			public:
				int m_index;
				std::vector<int> m_children;
				int m_parent;
				Point m_p;
				double m_volumeAbove;
				std::variant<MsComplex::Vertex, MsComplex::Face> m_criticalSimplex;
		};

		const Node& root() const;
		const Node& get(int index) const;

		void sort(std::function<bool(Node&, Node&)> comparator);
		std::optional<int> parentAtHeight(int nodeId, double height);

	private:
		int addNode(std::variant<MsComplex::Vertex, MsComplex::Face> criticalSimplex, Point p,
					std::vector<int> children);
		int findRootOfSubtree(int index);
		void sort(Node& root, std::function<bool(Node&, Node&)> comparator);
		double computeVolumeAbove(Node& node);
		double computeVolumeAbove(Node& node, double height);

		std::vector<Node> m_nodes;
		int m_rootIndex;
		std::shared_ptr<MsComplex> m_msc;
};

#endif /* MERGETREE_H */
