#include "striation.h"

Striation::Striation() = default;

Striation::Item& Striation::item(int id) {
	return m_items[id];
}

int Striation::addItem(int face,
                       std::vector<int> topCarvePath,
                       std::vector<int> bottomCarvePath,
                       std::vector<int> topVertices,
                       std::vector<int> bottomVertices) {
	m_items.emplace_back(face,
	                     topCarvePath,
	                     bottomCarvePath,
	                     topVertices,
	                     bottomVertices);
	return m_items.size() - 1;
}

void Striation::forItemsInOrder(const std::function<void(Item&, int)>& f) {
	if (m_items.size() == 0) {
		return;
	}
	forItemsInOrder(0, f);
}

void Striation::forItemsInOrder(int i,
                                const std::function<void(Item&, int)>& f) {
	if (i == -1) {
		return;
	}

	forItemsInOrder(m_items[i].m_topItem, f);
	f(m_items[i], i);
	forItemsInOrder(m_items[i].m_bottomItem, f);
}

int Striation::itemCount() {
	return m_items.size();
}
