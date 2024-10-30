#ifndef UNITSDOCK_H
#define UNITSDOCK_H

#include <QDockWidget>
#include <QDoubleSpinBox>

#include "units.h"

/**
 * A window that allows setting the real-world units of the river.
 */
class UnitsDock : public QDockWidget {

	Q_OBJECT

	public:
		explicit UnitsDock(QWidget* parent = nullptr);
		explicit UnitsDock(Units units, QWidget* parent = nullptr);

	public slots:
		void setUnits(Units units);

	signals:
		void unitsChanged(Units units);

	private slots:
		/**
		 * Helper function that emits the unitsChanged() signal with the
		 * currently set unit values.
		 *
		 * \param value Ignored (but needed to simplify connecting to
		 * `QDoubleSpinBox::valueChanged(double)`).
		 */
		void emitUnitsChanged(double value);

	private:
		Units getUnits();

		QDoubleSpinBox* xResolutionField;
		QDoubleSpinBox* yResolutionField;
};

#endif // UNITSDOCK_H
