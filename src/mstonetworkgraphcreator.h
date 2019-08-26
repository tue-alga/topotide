#ifndef MSTONETWORKGRAPHCREATOR_H
#define MSTONETWORKGRAPHCREATOR_H

#include <functional>

#include "mscomplex.h"
#include "network.h"
#include "networkgraph.h"

/**
 * An algorithm for converting a Morse-Smale complex into a NetworkGraph.
 */
class MsToNetworkGraphCreator {

	public:

		/**
		 * Creates a network graph creator that can create a graph from a
		 * Morse-Smale complex.
		 *
		 * \note Call create() to actually execute the algorithm.
		 *
		 * \param msc The Morse-Smale complex to convert.
		 * \param networkGraph An empty network graph to store the result in.
		 * \param progressListener A function that is called when a progress
		 * update is available.
		 */
		MsToNetworkGraphCreator(
		        MsComplex& msc,
		        NetworkGraph* networkGraph,
		        std::function<void(int)> progressListener = nullptr);

		/**
		 * Creates the representative network.
		 */
		void create();

	private:

		/**
		 * The Morse-Smale complex.
		 */
		MsComplex& msc;

		/**
		 * The graph that we are going to store our result in.
		 */
		NetworkGraph* networkGraph;

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

#endif // MSTONETWORKGRAPHCREATOR_H
