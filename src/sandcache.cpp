#include "sandcache.h"

SandCache::SandCache(MsComplex* msc, Striation* striation,
                     SandFunction sandFunction) :
        m_msc(msc),
        m_sandFunction(sandFunction) {

	m_sandCache = std::vector<std::vector<double>>(
	                  striation->itemCount() + 1,
	                  std::vector<double>(striation->itemCount() + 1, -1)
	                  );

	striation->forItemsInOrder([this](Striation::Item& item, int i) {
		m_striationTopBottom.push_back(item);
	});
}

double
SandCache::sandFunction(const Network::Path& from, const Network::Path& to) {
	return sandFunction(from.topBottomOrder(), to.topBottomOrder());
}

double
SandCache::sandFunction(int fromId, int toId) {

	// try to retrieve from cache
	if (m_sandCache[fromId][toId] != -1) {
		return m_sandCache[fromId][toId];
	}

	m_sandFunction(m_sandCache, m_msc,
	               m_striationTopBottom,
	               fromId, toId);

	return m_sandCache[fromId][toId];
}

void
SandCache::waterLevelSandFunction(
        std::vector<std::vector<double>>& sandCache,
        MsComplex* msc,
        std::vector<Striation::Item>& striationTopBottom,
        int fromId,
        int toId) {

	// store propagated height values on MS-vertices
	std::vector<double> temporaryHeight(msc->vertexCount());
	for (int i = 0; i < msc->vertexCount(); i++) {
		temporaryHeight[i] = msc->vertex(i).data().p.h;
	}

	double sand = 0;
	sandCache[fromId][fromId] = 0;

	// is from above to?
	if (fromId < toId) {

		// iterate over all MS-faces between from and to
		for (int i = fromId; i < toId; i++) {
			Striation::Item& item = striationTopBottom[i];
			MsComplex::Face f = msc->face(item.m_face);
			double maxHeight = 0;

			// find the maximum height on the incoming side
			for (auto topVertex : item.m_topVertices) {
				double h = temporaryHeight[topVertex];
				if (h > maxHeight) {
					maxHeight = h;
				}
			}

			// compute sand function
			double thisSand = f.data().volumeAbove(maxHeight);
			if (thisSand < 0) {
				assert(thisSand > -1);
				thisSand = 0; // avoid rounding errors
			}
			sand += thisSand;
			sandCache[fromId][i + 1] = sand;

			// push the maximum height to the outgoing side
			for (auto bottomVertex : item.m_bottomVertices) {
				temporaryHeight[bottomVertex] = maxHeight;
			}
		}

	} else if (fromId > toId) {

		// iterate over all MS-faces between from and to
		for (int i = fromId - 1; i >= toId; i--) {
			Striation::Item& item = striationTopBottom[i];
			MsComplex::Face f = msc->face(item.m_face);
			double maxHeight = 0;

			// find the maximum height on the incoming side
			for (auto bottomVertex : item.m_bottomVertices) {
				double h = temporaryHeight[bottomVertex];
				if (h > maxHeight) {
					maxHeight = h;
				}
			}

			// compute sand function
			double thisSand = f.data().volumeAbove(maxHeight);
			if (thisSand < 0) {
				assert(thisSand > -1);
				thisSand = 0; // avoid rounding errors
			}
			sand += thisSand;
			sandCache[fromId][i] = sand;

			// push the maximum height to the outgoing side
			for (auto topVertex : item.m_topVertices) {
				temporaryHeight[topVertex] = maxHeight;
			}
		}
	}
}

void
SandCache::waterFlowSandFunction(
        std::vector<std::vector<double>>& sandCache,
        MsComplex* msc,
        std::vector<Striation::Item>& striationTopBottom,
        int fromId,
        int toId) {

	// store propagated height values on MS-vertices
	std::vector<double> temporaryHeight(msc->vertexCount());
	for (int i = 0; i < msc->vertexCount(); i++) {
		temporaryHeight[i] = msc->vertex(i).data().p.h;
	}

	double sand = 0;
	sandCache[fromId][fromId] = 0;

	// is from above to?
	if (fromId < toId) {

		// iterate over all MS-faces between from and to
		for (int i = fromId; i < toId; i++) {
			Striation::Item& item = striationTopBottom[i];
			MsComplex::Face f = msc->face(item.m_face);
			double maxHeight = 0;

			// find the maximum height on the incoming side
			for (auto topVertex : item.m_topVertices) {
				double h = temporaryHeight[topVertex];
				if (h > maxHeight) {
					maxHeight = h;
				}
			}

			// compute sand function
			double thisSand = f.data().volumeAbove(maxHeight);
			if (thisSand < 0) {
				assert(thisSand > -1);
				thisSand = 0; // avoid rounding errors
			}
			sand += thisSand;
			sandCache[fromId][i + 1] = sand;

			// push the maximum height to the outgoing side
			for (auto bottomVertex : item.m_bottomVertices) {
				if (maxHeight <= temporaryHeight[bottomVertex]) {
					temporaryHeight[bottomVertex] = maxHeight;
				}
			}
		}

	} else if (fromId > toId) {

		// iterate over all MS-faces between from and to
		for (int i = fromId - 1; i >= toId; i--) {
			Striation::Item& item = striationTopBottom[i];
			MsComplex::Face f = msc->face(item.m_face);
			double maxHeight = 0;

			// find the maximum height on the incoming side
			for (auto bottomVertex : item.m_bottomVertices) {
				double h = temporaryHeight[bottomVertex];
				if (h > maxHeight) {
					maxHeight = h;
				}
			}

			// compute sand function
			double thisSand = f.data().volumeAbove(maxHeight);
			if (thisSand < 0) {
				assert(thisSand > -1);
				thisSand = 0; // avoid rounding errors
			}
			sand += thisSand;
			sandCache[fromId][i] = sand;

			// push the maximum height to the outgoing side
			for (auto topVertex : item.m_topVertices) {
				if (maxHeight <= temporaryHeight[topVertex]) {
					temporaryHeight[topVertex] = maxHeight;
				}
			}
		}
	}
}

void
SandCache::symmetricFlowSandFunction(
        std::vector<std::vector<double>>& sandCache,
        MsComplex* msc,
        std::vector<Striation::Item>& striationTopBottom,
        int fromId,
        int toId) {

	double sand = 0;
	sandCache[fromId][fromId] = 0;

	if (toId == fromId) {
		return;
	}
	if (toId < fromId) {
		std::swap(fromId, toId);
	}

	// store propagated height values on MS-vertices
	std::vector<double> temporaryHeight(msc->vertexCount());
	for (int i = 0; i < msc->vertexCount(); i++) {
		temporaryHeight[i] = msc->vertex(i).data().p.h;
	}
	std::vector<double> cutOffHeights(msc->faceCount());

	/* while for the water level and the water flow model we can do the
	 * temporary height computation and the sand function computation at the
	 * same time, for the symmetric flow model we need to propagate in both
	 * directions separately and sum up the sand volumes afterwards */

	// iterate over all MS-faces in top-to-bottom direction
	for (int i = fromId; i < toId; i++) {
		Striation::Item& item = striationTopBottom[i];
		double maxHeight = 0;

		// find the maximum height on the incoming side
		for (auto topVertex : item.m_topVertices) {
			double h = temporaryHeight[topVertex];
			if (h > maxHeight) {
				maxHeight = h;
			}
		}

		cutOffHeights[i] = maxHeight;

		// push the maximum height to the outgoing side
		for (auto bottomVertex : item.m_bottomVertices) {
			if (maxHeight <= temporaryHeight[bottomVertex]) {
				temporaryHeight[bottomVertex] = maxHeight;
			}
		}
	}

	for (int i = 0; i < msc->vertexCount(); i++) {
		temporaryHeight[i] = msc->vertex(i).data().p.h;
	}

	// same in the bottom-to-top direction
	for (int i = toId - 1; i >= fromId; i--) {
		Striation::Item& item = striationTopBottom[i];
		double maxHeight = 0;

		// find the maximum height on the incoming side
		for (auto bottomVertex : item.m_bottomVertices) {
			double h = temporaryHeight[bottomVertex];
			if (h > maxHeight) {
				maxHeight = h;
			}
		}

		cutOffHeights[i] = std::max(cutOffHeights[i], maxHeight);

		// push the maximum height to the outgoing side
		for (auto topVertex : item.m_topVertices) {
			if (maxHeight <= temporaryHeight[topVertex]) {
				temporaryHeight[topVertex] = maxHeight;
			}
		}
	}

	// finally get the result
	sand = 0;
	for (int i = fromId; i < toId; i++) {
		Striation::Item& item = striationTopBottom[i];
		MsComplex::Face f = msc->face(item.m_face);

		double thisSand = f.data().volumeAbove(cutOffHeights[i]);

		if (thisSand < 0) {
			assert(thisSand > -1);
			thisSand = 0; // avoid rounding errors
		}
		sand += thisSand;
	}
	sandCache[fromId][toId] = sand;
	sandCache[toId][fromId] = sand;
}
