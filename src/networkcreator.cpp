#include "networkcreator.h"

NetworkCreator::NetworkCreator(
        Striation& striation,
        MsComplex& msc,
        SandCache* sandCache,
        std::vector<Network::Path>* sortedPaths,
        bool bidirectional,
        double delta,
        Network* network,
        std::function<void(int)> progressListener) :
    striation(striation), msComplex(msc), sandCache(sandCache),
    sortedPaths(sortedPaths), bidirectional(bidirectional),
    delta(delta), network(network),
    progressListener(progressListener) {
}

void NetworkCreator::create() {

	signalProgress(0);

	std::vector<Network::Path> pathsTopToBottom = getPaths();
	std::vector<bool> used(sortedPaths->size(), false);

	int i = 0;

	for (const auto& path : *sortedPaths) {
		i++;
		signalProgress((int) (10 + 90 * i / sortedPaths->size()));

		double sand = delta;

		if (bidirectional) {

			// top

			for (int k = path.topBottomOrder() - 1;
			                     k >= 0; k--) {
				if (used[k]) {
					sand = sandCache->sandFunction(pathsTopToBottom[k], path);
					break;
				}
			}

			if (sand < delta) {
				continue;
			}

			// bottom

			for (int k = path.topBottomOrder() + 1;
			                     k < pathsTopToBottom.size(); k++) {
				if (used[k]) {
					sand = sandCache->sandFunction(pathsTopToBottom[k], path);
					break;
				}
			}

			if (sand < delta) {
				continue;
			}
		}

		// top

		for (int k = path.topBottomOrder() - 1;
		                         k >= 0; k--) {
			if (used[k]) {
				sand = sandCache->sandFunction(path, pathsTopToBottom[k]);
				break;
			}
		}

		if (sand < delta) {
			continue;
		}

		// bottom

		for (int k = path.topBottomOrder() + 1;
		                         k < pathsTopToBottom.size(); k++) {
			if (used[k]) {
				sand = sandCache->sandFunction(path, pathsTopToBottom[k]);
				break;
			}
		}

		if (sand < delta) {
			continue;
		}

		used[path.topBottomOrder()] = true;
		network->addPath(path);
	}

	network->sortTopToBottom();

	signalProgress(100);
}

std::vector<Network::Path> NetworkCreator::getPaths() {

	std::vector<Network::Path> paths;

	striation.forItemsInOrder([this, &paths](Striation::Item item, int i) {
		if (paths.empty()) {
			std::vector<Network::PathEdge> topCarvePath;
			for (auto e : item.m_topCarvePath) {
				topCarvePath.emplace_back(msComplex.halfEdge(e));
			}
			paths.emplace_back(paths.size(), topCarvePath);
		}
		std::vector<Network::PathEdge> bottomCarvePath;
		for (auto e : item.m_bottomCarvePath) {
			bottomCarvePath.emplace_back(msComplex.halfEdge(e));
		}
		paths.emplace_back(paths.size(), bottomCarvePath);
	});

	return paths;
}

void NetworkCreator::signalProgress(int progress) {
	if (progressListener != nullptr) {
		progressListener(progress);
	}
}
