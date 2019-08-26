#include "network.h"

#include <algorithm>

Network::Path::Path(int topBottomOrder, const std::vector<PathEdge>& edges) :
        m_topBottomOrder(topBottomOrder),
        m_edges(edges),
        m_edgesOnHeight(edges) {
	std::sort(m_edgesOnHeight.rbegin(), m_edgesOnHeight.rend());
}

const std::vector<Network::PathEdge>& Network::Path::edges() const {
	return m_edges;
}

const std::vector<Network::PathEdge>& Network::Path::edgesOnHeight() const {
	return m_edgesOnHeight;
}

int Network::Path::topBottomOrder() const {
	return m_topBottomOrder;
}

bool Network::Path::operator<(const Path& other) const {
	int i = 0;
	while (i < m_edgesOnHeight.size() && i < other.m_edgesOnHeight.size()) {
		if (m_edgesOnHeight[i] == other.m_edgesOnHeight[i]) {
			i++;
		} else {
			return m_edgesOnHeight[i] < other.m_edgesOnHeight[i];
		}
	}
	return m_edgesOnHeight.size() < other.m_edgesOnHeight.size();
}

Network::Network() = default;

void Network::addPath(const Path& p) {
	m_paths.push_back(p);
}

const std::vector<Network::Path>& Network::paths() {
	return m_paths;
}

void Network::sortTopToBottom() {
	std::sort(m_paths.begin(), m_paths.end(), [](Path p1, Path p2) {
		return p1.topBottomOrder() < p2.topBottomOrder();
	});
}
