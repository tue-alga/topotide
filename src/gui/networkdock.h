#ifndef NETWORKDOCK_H
#define NETWORKDOCK_H

#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "../network.h"

/**
 * Widget that allows the user to inspect the generated network.
 */
class NetworkDock : public QDockWidget {

	Q_OBJECT

	public:

		/**
		 * Creates a new striation widget.
		 * \param parent The parent of this widget.
		 */
		explicit NetworkDock(QWidget *parent = 0);

	signals:

		/**
		 * Emitted when the visible network path changes.
		 * \param path The ID of the new path.
		 */
		void pathChanged(int path);

	public slots:

		/**
		 * Changes the network that is being visualized.
		 * \param network The network, or `nullptr` to disable the widget.
		 */
		void setNetwork(Network* network);

		/**
		 * Moves to the previous network path.
		 */
		void goToPrevious();

		/**
		 * Moves to the next network path.
		 */
		void goToNext();

	private:

		/**
		 * The network that we are visualizing, or `nullptr` if we are not
		 * visualizing a network.
		 */
		Network* m_network = nullptr;

		/**
		 * The ID of the network path we are currently looking at.
		 */
		int m_path = 0;

		QWidget* networkWidget;
		QLabel* statusLabel;
		QPushButton* toPreviousButton;
		QPushButton* toNextButton;

		/**
		 * Updates the UI elements.
		 */
		void update();
};

#endif // NETWORKDOCK_H
