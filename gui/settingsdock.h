#ifndef SETTINGSDOCK_H
#define SETTINGSDOCK_H

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QSlider>
#include <QStackedWidget>
#include <QWidget>

#include "units.h"

/**
 * A QDockWidget that allows the user to change settings related to the network
 * computation.
 */
class SettingsDock : public QDockWidget {

	Q_OBJECT

	public:

		/**
		 * Creates a new settings dock.
		 * \param parent The parent of this widget.
		 */
		SettingsDock(QWidget* parent = 0);

		/**
		 * Returns the currently set threshold for the Morse-Smale complex
		 * simplification.
		 *
		 * \return The threshold.
		 */
		double msThreshold();

	public slots:
		void setUnits(Units units);

	signals:
		void msThresholdChanged(int threshold);

	private:
		QWidget* settingsWidget;
		QLabel* msThresholdLabel;
		QSlider* msThresholdSlider;

		Units m_units;

	private slots:
		void updateLabels();
};

#endif // SETTINGSDOCK_H
