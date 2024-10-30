#ifndef BACKGROUNDTHREAD_H
#define BACKGROUNDTHREAD_H

#include <QThread>

#include <memory>

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
		 */
		BackgroundThread(const std::shared_ptr<RiverData>& data,
						const std::shared_ptr<RiverFrame>& frame, double msThreshold);

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
		std::shared_ptr<RiverData> m_data;
		std::shared_ptr<RiverFrame> m_frame;

		/**
		 * The sand threshold for simplifying the Morse-Smale complex.
		 */
		double m_msThreshold;

		void computeInputGraph();
		void computeInputDcel();
		void computeMsComplex();
		void simplifyMsComplex();
		void msComplexToNetworkGraph();
};

#endif // BACKGROUNDTHREAD_H
