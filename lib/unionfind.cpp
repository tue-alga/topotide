#include "unionfind.h"

UnionFindNode::UnionFindNode(int id) : point(id), parent(id) {
}

UnionFind::UnionFind(int points) {

	for (int i = 0; i < points; i++) {
		UnionFindNode node(i);
		_points.push_back(node);
	}
}

UnionFindNode& UnionFind::get(int p) {
	return _points[p];
}

void UnionFind::set(int p, UnionFindNode node) {
	_points[p] = node;
}

int UnionFind::findSet(int p) {
	UnionFindNode &x = get(p);
	if (p != x.parent) {
		x.parent = findSet(x.parent);
	}
	return x.parent;
}

void UnionFind::merge(int p1, int p2) {
	int rep1 = findSet(p1);
	int rep2 = findSet(p2);
	get(rep2).parent = rep1;
}
