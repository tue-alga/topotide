#include "unitsdock.h"

#include <QFont>
#include <QFormLayout>
#include <QLabel>

UnitsDock::UnitsDock(Units units,
                     QWidget* parent) :
    QDockWidget("Unit settings", parent) {

	QWidget* unitsWidget = new QWidget(this);
	setWidget(unitsWidget);

	auto* layout = new QFormLayout(unitsWidget);
	unitsWidget->setLayout(layout);

	QFont bold = unitsWidget->font();
	bold.setBold(true);

	QLabel* label = new QLabel("Horizontal resolution:");
	label->setAlignment(Qt::AlignCenter);
	label->setFont(bold);
	layout->addRow(label);

	// QDoubleSpinBox::valueChanged() has two overloads; to make connecting
	// to valueChanged(double) easier, we store that into valueChangedDouble
	auto valueChangedDouble =
	        static_cast<void (QDoubleSpinBox::*)(double)>(
	            &QDoubleSpinBox::valueChanged);

	xResolutionField = new QDoubleSpinBox(unitsWidget);
	xResolutionField->setSuffix(" m / pixel");
	xResolutionField->setDecimals(1);
	xResolutionField->setMinimum(0.1);
	xResolutionField->setMaximum(std::numeric_limits<double>::infinity());
	xResolutionField->setValue(units.m_xResolution);
	connect(xResolutionField, valueChangedDouble,
	        this, &UnitsDock::emitUnitsChanged);
	layout->addRow("x-direction:", xResolutionField);

	yResolutionField = new QDoubleSpinBox(unitsWidget);
	yResolutionField->setSuffix(" m / pixel");
	yResolutionField->setDecimals(1);
	yResolutionField->setMinimum(0.1);
	yResolutionField->setMaximum(std::numeric_limits<double>::infinity());
	yResolutionField->setValue(units.m_yResolution);
	connect(yResolutionField, valueChangedDouble,
	        this, &UnitsDock::emitUnitsChanged);
	layout->addRow("y-direction:", yResolutionField);

	label = new QLabel("Elevation range:");
	label->setAlignment(Qt::AlignCenter);
	label->setFont(bold);
	layout->addRow(label);

	minElevationField = new QDoubleSpinBox(unitsWidget);
	minElevationField->setSuffix(" m");
	minElevationField->setDecimals(1);
	minElevationField->setMinimum(-std::numeric_limits<double>::infinity());
	minElevationField->setMaximum(std::numeric_limits<double>::infinity());
	minElevationField->setValue(units.m_minElevation);
	connect(minElevationField, valueChangedDouble,
	        [this](double v) {
		if (maxElevationField->value() <= v) {
			maxElevationField->setValue(v + 0.1);
		}
	});
	connect(minElevationField, valueChangedDouble,
	        this, &UnitsDock::emitUnitsChanged);
	layout->addRow("Minimum:", minElevationField);

	maxElevationField = new QDoubleSpinBox(unitsWidget);
	maxElevationField->setSuffix(" m");
	maxElevationField->setDecimals(1);
	maxElevationField->setMinimum(-std::numeric_limits<double>::infinity());
	maxElevationField->setMaximum(std::numeric_limits<double>::infinity());
	maxElevationField->setValue(units.m_maxElevation);
	connect(maxElevationField, valueChangedDouble,
	        [this](double v) {
		if (minElevationField->value() >= v) {
			minElevationField->setValue(v - 0.1);
		}
	});
	connect(maxElevationField, valueChangedDouble,
	        this, &UnitsDock::emitUnitsChanged);
	layout->addRow("Maximum:", maxElevationField);
}

void UnitsDock::setUnits(Units units) {
	xResolutionField->setValue(units.m_xResolution);
	yResolutionField->setValue(units.m_yResolution);
	minElevationField->setValue(units.m_minElevation);
	maxElevationField->setValue(units.m_maxElevation);
}

Units UnitsDock::getUnits() {
	Units u;
	u.m_xResolution = xResolutionField->value();
	u.m_yResolution = yResolutionField->value();
	u.m_minElevation = minElevationField->value();
	u.m_maxElevation = maxElevationField->value();
	return u;
}

void UnitsDock::emitUnitsChanged(double value) {
	emit unitsChanged(getUnits());
}
