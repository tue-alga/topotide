#include "sortedpathscreator.h"

SortedPathsCreator::SortedPathsCreator(
        MsComplex* msComplex,
        Striation* striation,
        std::vector<Network::Path>* paths,
        std::function<void(int)> progressListener) :
    msComplex(msComplex), striation(striation),
    paths(paths), progressListener(progressListener) {
}

void SortedPathsCreator::create() {

	signalProgress(0);

	// get all striation paths
	striation->forItemsInOrder([this](Striation::Item item, int i) {
		if (paths->empty()) {
			std::vector<Network::PathEdge> topCarvePath;
			for (auto e : item.m_topCarvePath) {
				topCarvePath.emplace_back(msComplex->halfEdge(e));
			}
			paths->push_back(
			            Network::Path(paths->size(), topCarvePath));
		}
		std::vector<Network::PathEdge> bottomCarvePath;
		for (auto e : item.m_bottomCarvePath) {
			bottomCarvePath.emplace_back(msComplex->halfEdge(e));
		}
		paths->push_back(
		            Network::Path(paths->size(), bottomCarvePath));
	});

	// sort them
	std::sort(paths->begin(), paths->end());

	signalProgress(100);
}

void SortedPathsCreator::signalProgress(int progress) {
	if (progressListener != nullptr) {
		progressListener(progress);
	}
}
