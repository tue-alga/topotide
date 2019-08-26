#ifndef BACKGROUNDDOCK_H
#define BACKGROUNDDOCK_H

#include <QDial>
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

#include "../units.h"

/**
 * A QDockWidget that allows the user to set the water level
 * and the water slope.
 */
class BackgroundDock : public QDockWidget {

	Q_OBJECT

	public:

		/**
		 * Creates a new progress viewer without any tasks.
		 * \param parent The parent of this widget.
		 */
		BackgroundDock(QWidget* parent = 0);

		/**
		 * Returns the currently set water level.
		 * \return The water level.
		 */
		int waterLevel();

		/**
		 * Returns the currently set water slope.
		 * \return The water slope.
		 */
		int waterSlope();

	public slots:
		void resetSettings();
		void setUnits(Units units);

	signals:
		void waterLevelChanged(int level);
		void waterSlopeChanged(int slope);

	private:
		QWidget* settingsWidget;
		QLabel* waterLevelLabel;
		QSlider* waterLevelSlider;
		QDial* waterSlopeDial;
		QPushButton* resetButton;

		Units m_units;

	private slots:
		void updateLabels();
};

#endif // BACKGROUNDDOCK_H
