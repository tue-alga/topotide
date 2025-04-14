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
		 * Creates a background thread that computes the network for one river
		 * frame.
		 */
		BackgroundThread(const std::shared_ptr<RiverData>& data,
						const std::shared_ptr<RiverFrame>& frame);
		/**
		 * Creates a background thread that computes the network for all river
		 * frames.
		 */
		BackgroundThread(const std::shared_ptr<RiverData>& data);

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

		/**
		 * Called when there was an error during the computation.
		 * \param error A description of the error.
		 */
		void errorEncountered(QString error);

	private:

		bool computeForFrame();

		/**
		 * The river data we are computing on.
		 */
		std::shared_ptr<RiverData> m_data;
		std::shared_ptr<RiverFrame> m_frame;

		QString m_taskPrefix = "";

		void computeInputGraph();
		void computeInputDcel();
		void computeMsComplex();
		void computeMergeTree();
		void simplifyMsComplex();
		void msComplexToNetworkGraph();
};

#endif // BACKGROUNDTHREAD_H
