#ifndef BACKGROUNDTHREAD_H
#define BACKGROUNDTHREAD_H

#include <QThread>

#include "riverdata.h"

/**
 * The background thread that computes the representative network.
 */
class BackgroundThread : public QThread {

		Q_OBJECT

	public:

		/**
		 * Creates the background thread.
		 *
		 * \param data The river data object.
		 * \param onlyNetwork If `true`, we assume that the sand cache and the
		 * striation are already up-to-date, and only compute the network.
		 * \param bidirectional Whether to use the bidirectional sand function.
		 * \param simplify Whether to simplify the resulting network.
		 * \param hybridStriation Whether to use the hybrid striation method.
		 * \param delta The delta value.
		 */
		BackgroundThread(RiverData* data, bool onlyNetwork,
		                 int sandFunction,
		                 bool bidirectional, bool simplify,
		                 bool hybridStriation,
		                 double delta, double msThreshold);

		void run() override;

	signals:

		/**
		 * Called when a new task is started.
		 * \param name The name of the task.
		 */
		void taskStarted(QString name);

		/**
		 * Called when progress has been made.
		 *
		 * \param name The name of the task.
		 * \param progress The progress percentage (in [0-100]).
		 */
		void progressMade(QString name, int progress);

		/**
		 * Called when a task is finished.
		 * \param name The name of the task.
		 */
		void taskEnded(QString name);

	private:

		/**
		 * The river data we are computing on.
		 */
		RiverData* m_data;

		/**
		 * If `true`, we assume that the sand cache and the striation are
		 * already up-to-date, and only compute the network.
		 */
		bool m_onlyNetwork;

		/**
		 * The sand function to use.
		 */
		int m_sandFunction;

		/**
		 * Whether to use the bidirectional sand function.
		 */
		bool m_bidirectional;

		/**
		 * Whether to simplify the resulting graph.
		 */
		bool m_simplify;

		/**
		 * Whether to use the hybrid striation method (consider only faces along
		 * the lowest path).
		 */
		bool m_hybridStriation;

		/**
		 * The delta value.
		 */
		double m_delta;

		/**
		 * The sand threshold for simplifying the Morse-Smale complex.
		 */
		double m_msThreshold;

		void computeInputGraph();
		void computeInputDcel();
		void computeMsComplex();

		void computeStriation();
		void computeSortedPaths();
		void initializeSandCache();
		void computeNetwork();
		void computeNetworkGraph();

		void simplifyMsComplex();
		void msComplexToNetworkGraph();
};

#endif // BACKGROUNDTHREAD_H
