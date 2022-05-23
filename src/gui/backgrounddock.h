#ifndef BACKGROUNDDOCK_H
#define BACKGROUNDDOCK_H

#include <QCheckBox>
#include <QComboBox>
#include <QDial>
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
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
		 * Returns whether the elevation should be shown.
		 * \return The value of the show elevation input.
		 */
		bool showElevation();

		/**
		 * Returns the currently set theme.
		 * \return The theme.
		 */
		QString theme();

		/**
		 * Returns whether the water plane should be shown.
		 * \return The value of the show water plane input.
		 */
		bool showWaterPlane();

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

		/**
		 * Returns whether the contours should be shown.
		 * \return The value of the show contours input.
		 */
		bool showContours();

		/**
		 * Returns whether the shading should be shown.
		 * \return The value of the show shading input.
		 */
		bool showShading();

	public slots:
		void resetSettings();
		void setUnits(Units units);

	signals:
		void showElevationChanged(bool showElevation);
		void showWaterPlaneChanged(bool showWaterPlane);
		void showContoursChanged(bool showContours);
		void showShadingChanged(bool showShading);
		void waterLevelChanged(int level);
		void waterSlopeChanged(int slope);
		void themeChanged(QString theme);

	private:
		QWidget* m_settingsWidget;

		QCheckBox* m_showElevationBox;
		QWidget* m_elevationSettings;
		QComboBox* m_colorSchemeBox;

		QCheckBox* m_showWaterLevelBox;
		QWidget* m_waterLevelSettings;
		QLabel* m_waterLevelLabel;
		QSlider* m_waterLevelSlider;
		QLabel* m_waterLevelValueLabel;
		QDial* m_waterSlopeDial;
		QPushButton* m_waterResetButton;

		QCheckBox* m_showContoursBox;
		QWidget* m_contourSettings;
		QSpinBox* m_contourCountBox;

		QCheckBox* m_showShadingBox;
		QWidget* m_shadingSettings;
		QSlider* m_shadingStrengthSlider;

		Units m_units;

	private slots:
		void updateLabels();
};

#endif // BACKGROUNDDOCK_H
