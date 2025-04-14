#include "mergetree.h"

#include <algorithm>

MergeTree::MergeTree(const std::shared_ptr<MsComplex>& msc) : m_msc(msc) {
	// add all maxima (MS faces)
	std::vector<int> faceToNodeIdMap(m_msc->faceCount(), -1);
	for (int i = 0; i < m_msc->faceCount(); i++) {
		MsComplex::Face face = m_msc->face(i);
		int nodeId = addNode(face, face.data().maximum.data().p, {});
		faceToNodeIdMap[i] = nodeId;
	}

	// sort all saddles from high to low
	std::vector<MsComplex::Vertex> saddles;
	saddles.reserve(m_msc->vertexCount());
	for (int i = 0; i < m_msc->vertexCount(); i++) {
		if (m_msc->vertex(i).data().type == VertexType::saddle) {
			saddles.push_back(m_msc->vertex(i));
		}
	}
	std::sort(saddles.begin(), saddles.end(), [](MsComplex::Vertex v1, MsComplex::Vertex v2) {
		return v1.data().p > v2.data().p;
	});

	// add a merge tree vertex for each saddle
	for (int i = 0; i < saddles.size(); i++) {
		MsComplex::Vertex saddle = saddles[i];
		assert(saddle.data().type == VertexType::saddle);
		MsComplex::Face f1 = saddle.outgoing().incidentFace();
		MsComplex::Face f2 = saddle.outgoing().nextOutgoing().incidentFace();

		int f1NodeId = faceToNodeIdMap[f1.id()];
		assert(f1NodeId != -1);
		int f1RootId = findRootOfSubtree(f1NodeId);

		int f2NodeId = faceToNodeIdMap[f2.id()];
		assert(f2NodeId != -1);
		int f2RootId = findRootOfSubtree(f2NodeId);

		if (f1RootId != f2RootId) {
			int newNodeId = addNode(saddle, saddle.data().p, {f1RootId, f2RootId});
			m_nodes[newNodeId].m_volumeAbove = computeVolumeAbove(m_nodes[newNodeId]);
		}
	}
}

const MergeTree::Node& MergeTree::root() const {
	assert(!m_nodes.empty());
	return m_nodes.back();
}

const MergeTree::Node& MergeTree::get(int index) const {
	assert(index >= 0 && index < m_nodes.size());
	return m_nodes[index];
}

int MergeTree::addNode(std::variant<MsComplex::Vertex, MsComplex::Face> criticalSimplex, Point p,
                       std::vector<int> children) {
	int index = m_nodes.size();
	Node node{index, children, -1, p, 0, criticalSimplex};
	for (int childIndex : children) {
		assert(childIndex >= 0 && childIndex < m_nodes.size());
		m_nodes[childIndex].m_parent = index;
	}
	m_nodes.push_back(node);
	return index;
}

int MergeTree::findRootOfSubtree(int index) {
	while (m_nodes[index].m_parent != -1) {
		assert(index >= 0 && index < m_nodes.size());
		index = m_nodes[index].m_parent;
	}
	return index;
}

void MergeTree::sort(std::function<bool(Node&, Node&)> comparator) {
	Node& root = m_nodes.back();
	sort(root, comparator);
}

void MergeTree::sort(Node& root, std::function<bool(Node&, Node&)> comparator) {
	std::sort(root.m_children.begin(), root.m_children.end(),
	          [this, &comparator](int i1, int i2) { return comparator(m_nodes[i1], m_nodes[i2]); });
	for (int child : root.m_children) {
		sort(m_nodes[child], comparator);
	}
}

double MergeTree::computeVolumeAbove(Node& node) {
	return computeVolumeAbove(node, node.m_p.h);
}

double MergeTree::computeVolumeAbove(Node& node, double height) {
	// are we a leaf?
	if (node.m_children.empty()) {
		assert(std::holds_alternative<MsComplex::Face>(node.m_criticalSimplex));
		MsComplex::Face maximum = std::get<MsComplex::Face>(node.m_criticalSimplex);
		return maximum.data().volumeAbove(height);
	}

	// otherwise, we are an internal node
	double volume = 0;
	for (int child : node.m_children) {
		volume += computeVolumeAbove(m_nodes[child], height);
	}
	return volume;
}

std::optional<int> MergeTree::parentAtHeight(int nodeId, double height) {
	if (m_nodes[nodeId].m_p.h < height) {
		return std::nullopt;
	}
	while (m_nodes[nodeId].m_parent != -1 && m_nodes[m_nodes[nodeId].m_parent].m_p.h > height) {
		nodeId = m_nodes[nodeId].m_parent;
	}
	return nodeId;
}
