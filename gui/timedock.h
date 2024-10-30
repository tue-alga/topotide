#ifndef TIMEDOCK_H
#define TIMEDOCK_H

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QSlider>
#include <QStackedWidget>
#include <QWidget>

#include "units.h"

/**
 * A QDockWidget that allows the user to select a frame from the river time
 * series.
 */
class TimeDock : public QDockWidget {

	Q_OBJECT

	public:

		/**
		 * Creates a new time dock.
		 * \param parent The parent of this widget.
		 */
		TimeDock(QWidget* parent = 0);

		/**
		 * Returns the currently set frame ID.
		 * \return The threshold.
		 */
		int frame();

	public slots:
		void setFrame(int frame);
		void setFrameCount(int count);

	signals:
		void frameChanged(int frame);

	private:
		QWidget* container;
		QSlider* frameSlider;

		int m_frameCount;
};

#endif // TIMEDOCK_H
