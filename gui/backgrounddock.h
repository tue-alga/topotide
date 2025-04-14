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

#include "colorramp.h"

class ColorRampViewer : public QWidget {
	Q_OBJECT

	public:
		ColorRampViewer(ColorRamp ramp, QWidget* parent = nullptr);

	public slots:
		void setColorRamp(ColorRamp ramp);

	protected:
		void paintEvent(QPaintEvent *event) override;

	private:
		ColorRamp m_ramp;
};

/**
 * A QDockWidget that allows the user to change how the background map is
 * visualized.
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
		 * Returns the currently set color ramp.
		 * \return The color ramp.
		 */
		ColorRamp colorRamp();

		/**
		 * Returns whether the water plane should be shown.
		 * \return The value of the show water plane input.
		 */
		bool showWaterPlane();

		/**
		 * Returns the currently set water level.
		 * \return The water level.
		 */
		double waterLevel();

		/**
		 * Returns whether the contours should be shown.
		 * \return The value of the show contours input.
		 */
		bool showContours();

		/**
		 * Returns the number of contours that should be drawn.
		 * \return The value of the number-of-contours input.
		 */
		int contourCount();

		/**
		 * Returns whether the shading should be shown.
		 * \return The value of the show shading input.
		 */
		bool showShading();

	public slots:
		void setElevationRange(double minElevation, double maxElevation);

	signals:
		void showElevationChanged(bool showElevation);
		void showWaterPlaneChanged(bool showWaterPlane);
		void showContoursChanged(bool showContours);
		void contourCountChanged(int contourCount);
		void showShadingChanged(bool showShading);
		void waterLevelChanged(double waterLevel);
		void colorRampChanged(ColorRamp colorRamp);

	private:
		QWidget* m_settingsWidget;

		QCheckBox* m_showElevationBox;
		QWidget* m_elevationSettings;
		QComboBox* m_colorSchemeBox;
		ColorRampViewer* m_colorRampViewer;

		QCheckBox* m_showWaterLevelBox;
		QWidget* m_waterLevelSettings;
		QLabel* m_waterLevelLabel;
		QSlider* m_waterLevelSlider;
		QLabel* m_waterLevelValueLabel;

		QCheckBox* m_showContoursBox;
		QWidget* m_contourSettings;
		QSpinBox* m_contourCountBox;

		QCheckBox* m_showShadingBox;
		QWidget* m_shadingSettings;
		QSlider* m_shadingStrengthSlider;

		double m_minElevation;
		double m_maxElevation;
		double m_waterLevel;

	private slots:
		void updateLabels();
};

#endif // BACKGROUNDDOCK_H
