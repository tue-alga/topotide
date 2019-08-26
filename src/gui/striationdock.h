#ifndef STRIATIONDOCK_H
#define STRIATIONDOCK_H

#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "../striation.h"

/**
 * Widget that allows the user to step through a striation.
 */
class StriationDock : public QDockWidget {

	Q_OBJECT

	public:

		/**
		 * Creates a new striation widget.
		 * \param parent The parent of this widget.
		 */
		explicit StriationDock(QWidget *parent = 0);

	signals:

		/**
		 * Emitted when the visible striation item changes.
		 * \param item The ID of the new striation item.
		 */
		void itemChanged(int item);

	public slots:

		/**
		 * Changes the striation that is being visualized.
		 * \param striation The striation, or `nullptr` to disable the widget.
		 */
		void setStriation(Striation* striation);

		/**
		 * Moves to the top leaf of the current striation item.
		 */
		void goToTop();

		/**
		 * Moves to the root of the striation.
		 */
		void goToRoot();

		/**
		 * Moves to the bottom leaf of the current striation item.
		 */
		void goToBottom();

	private:

		/**
		 * The striation that we are visualizing, or `nullptr` if we are not
		 * visualizing a striation.
		 */
		Striation* m_striation = nullptr;

		/**
		 * The ID of the striation item we are currently looking at.
		 */
		int m_item = 0;

		QWidget* striationWidget;
		QLabel* statusLabel;
		QPushButton* toTopButton;
		QPushButton* toRootButton;
		QPushButton* toBottomButton;

		/**
		 * Updates the UI elements.
		 */
		void update();
};

#endif // STRIATIONDOCK_H
