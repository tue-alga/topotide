#ifndef SETTINGSDOCK_H
#define SETTINGSDOCK_H

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QSlider>
#include <QStackedWidget>
#include <QWidget>

#include <../units.h>

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
		 * Returns whether the user wants to use the hybrid striation strategy.
		 *
		 * \return `true` if the hybrid striation strategy is chosen; `false`
		 * otherwise (that is, the highest persistence first strategy).
		 */
		bool striationStrategy();

		/**
		 * Returns whether the user wants the MS-complex to be simplified.
		 *
		 * \return `true` if the simplify option is set; `false` otherwise.
		 */
		bool msSimplify();

		/**
		 * Returns the currently set δ-value.
		 * \return The δ-value.
		 */
		double delta();

		/**
		 * Returns the index of the sand function the user wants to use.
		 *
		 * `0`: water level
		 * `1`: water flow
		 * `2`: symmetric flow
		 *
		 * \return The sand function.
		 */
		int sandFunction();

		/**
		 * Returns whether the user wants a bidirectional sand function.
		 *
		 * \return `true` if the bidirectional sand function is set; `false`
		 * otherwise.
		 */
		bool bidirectional();

		/**
		 * Returns whether the user wants the network to be simplified.
		 *
		 * \return `true` if the simplify option is set; `false` otherwise.
		 */
		bool simplify();

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
		void striationStrategyChanged(bool strategy);
		void deltaChanged(int delta);
		void sandFunctionChanged(int sandFunction);
		void bidirectionalChanged(bool bidirectional);
		void simplifyChanged(bool simplify);
		void msSimplifyChanged(bool simplify);
		void msThresholdChanged(int threshold);

	private:
		QWidget* settingsWidget;
		QComboBox* algorithmBox;
		QStackedWidget* stack;

		QWidget* oldAlgorithmSettings;
		QComboBox* striationStrategyBox;
		QLabel* deltaLabel;
		QSlider* deltaSlider;
		QComboBox* sandFunctionBox;
		QCheckBox* bidirectionalBox;
		QCheckBox* simplifyBox;

		QWidget* newAlgorithmSettings;
		QLabel* msThresholdLabel;
		QSlider* msThresholdSlider;

		Units m_units;

	private slots:
		void updateLabels();
};

#endif // SETTINGSDOCK_H
