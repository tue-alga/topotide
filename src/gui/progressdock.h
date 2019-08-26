#ifndef PROGRESSDOCK_H
#define PROGRESSDOCK_H

#include <QDockWidget>
#include <QTime>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>

/**
 * A QDockWidget that displays progress information.
 */
class ProgressDock : public QDockWidget {

	public:

		/**
		 * Creates a new progress viewer without any tasks.
		 * \param parent The parent of this widget.
		 */
		ProgressDock(QWidget* parent = 0);

	public slots:

		/**
		 * Adds a new task to the progress viewer.
		 *
		 * \param name The user-visible name of the task.
		 */
		void startTask(const QString& name);

		/**
		 * Sets the progress of a task.
		 *
		 * \param task The name of the task.
		 * \param progress The progress, between 0 and 100 (inclusive).
		 */
		void setProgress(const QString& task, int progress);

		/**
		 * Marks a task as finished. This displays the elapsed time (from the
		 * moment the task was started) in the GUI.
		 *
		 * \param task The name of the task.
		 */
		void endTask(const QString& task);

		/**
		 * Removes all of the tasks from the progress viewer.
		 */
		void reset();

	private:

		/**
		 * The tree widget that displays the tasks.
		 */
		QTreeWidget* tree;

		/**
		 * A task that is displayed in the progress viewer.
		 */
		struct Task {

			/**
			 * The corresponding item in the QTreeWidget.
			 */
			QTreeWidgetItem* item;

			/**
			 * The name of the task.
			 */
			QString name;

			/**
			 * The time this task started (for timing purposes).
			 */
			QTime time;
		};

		/**
		 * A list of the tasks displayed.
		 */
		QVector<Task> tasks;

		/**
		 * Returns the index of the task with the given name.
		 *
		 * \param name The task name.
		 * \return The index in `tasks` of the task with this name, or `-1` if
		 * no such task exists.
		 */
		int taskWithName(const QString& name);
};

#endif // PROGRESSDOCK_H
