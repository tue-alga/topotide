#ifndef NETWORKCREATOR_H
#define NETWORKCREATOR_H

#include <functional>

#include "network.h"
#include "sandcache.h"
#include "striation.h"

/**
 * An algorithm for computing a representative network from a striation.
 *
 * \note This is separate from the Network class itself, so it is
 * possible to obtain progress information while the representative network is
 * being computed.
 */
class NetworkCreator {

	public:

		/**
		 * Creates a representative network creator that can create a
		 * representative network from a striation.
		 *
		 * \note Call create() to actually execute the algorithm.
		 *
		 * \param striation The striation to create a representative network of.
		 * \param msc The Morse-Smale complex.
		 * \param sandCache The sand cache used for computing and storing the
		 * sand function values.
		 * \param sortedPaths The list of striation paths, sorted from low to
		 * high.
		 * \param bidirectional Whether to use the bidirectional (`true`) or
		 * the unidirectional (`false`) sand function.
		 * \param delta The river delta.
		 * \param network An empty network to store the result in.
		 * \param progressListener A function that is called when a progress
		 * update is available.
		 */
		NetworkCreator(
		        Striation& striation,
		        MsComplex& msc,
		        SandCache* sandCache,
		        std::vector<Network::Path>* sortedPaths,
		        bool bidirectional,
		        double delta,
		        Network* network,
		        std::function<void(int)> progressListener = nullptr);

		/**
		 * Creates the representative network.
		 */
		void create();

	private:

		/**
		 * Returns a list of striation paths, ordered from top to bottom.
		 * \return The striation paths.
		 */
		std::vector<Network::Path> getPaths();

		/**
		 * The striation we are going to create a representative network of.
		 */
		Striation& striation;

		/**
		 * The Morse-Smale complex.
		 */
		MsComplex& msComplex;

		/**
		 * The sand cache that computes and stores the sand function values.
		 */
		SandCache* sandCache;

		/**
		 * The list of striation paths, sorted from low to high.
		 */
		std::vector<Network::Path>* sortedPaths;

		/**
		 * Whether to use the bidirectional sand function.
		 */
		bool bidirectional;

		/**
		 * The delta value.
		 */
		double delta;

		/**
		 * The representative network that we are going to store our result in.
		 */
		Network* network;

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

#endif // NETWORKCREATOR_H
