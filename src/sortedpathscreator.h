#ifndef SORTEDPATHSCREATOR_H
#define SORTEDPATHSCREATOR_H

#include <functional>
#include <vector>

#include "mscomplex.h"
#include "network.h"
#include "striation.h"

/**
 * An algorithm for producing a sorted list of striation paths.
 *
 * The paths are sorted from low to high lexicographic height.
 */
class SortedPathsCreator {

	public:
		SortedPathsCreator(MsComplex* msComplex, Striation* striation,
		                   std::vector<Network::Path>* paths,
		                   std::function<void(int)> progressListener = nullptr);

		void create();

	private:

		MsComplex* msComplex;
		Striation* striation;
		std::vector<Network::Path>* paths;

		/**
		 * Calls the progress listener, if one is set.
		 * \param progress The progress value to report.
		 */
		void signalProgress(int progress);

		/**
		 * A function that is called when there is a progress update.
		 */
		std::function<void(int)> progressListener;
};

#endif // SORTEDPATHSCREATOR_H
