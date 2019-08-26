#include <cmath>

#include <QGridLayout>

#include "backgrounddock.h"

#include "unitshelper.h"

BackgroundDock::BackgroundDock(QWidget* parent) :
        QDockWidget("Background settings", parent) {

	settingsWidget = new QWidget(this);
	auto* layout = new QGridLayout();
	settingsWidget->setLayout(layout);
	setWidget(settingsWidget);

	layout->setRowStretch(0, 1);
	layout->setRowStretch(1, 1);
	layout->setRowStretch(2, 0);

	// first column: water level slider and δ-slider
	layout->setColumnStretch(0, 1);

	waterLevelSlider = new QSlider(Qt::Horizontal, settingsWidget);
	waterLevelSlider->setRange(0, 256 * 256 * 256);
	waterLevelSlider->setSingleStep(256 * 8);
	waterLevelSlider->setPageStep(256 * 256 * 8);
	waterLevelSlider->setValue(128 * 256 * 256);
	waterLevelSlider->setToolTip("<p><b>Water level</b></p>"
	                             "<p>This changes the water level, and can be used for inspecting the river DEM.</p>"
	                             "<p>This setting is used only for visualization purposes, and does not influence the resulting network.</p>");
	layout->addWidget(waterLevelSlider, 0, 0, Qt::AlignBottom);

	waterLevelLabel = new QLabel();
	layout->addWidget(waterLevelLabel, 1, 0, Qt::AlignHCenter | Qt::AlignTop);
	connect(waterLevelSlider, &QSlider::valueChanged,
	        this, &BackgroundDock::updateLabels);
	connect(waterLevelSlider, &QSlider::valueChanged,
	        this, &BackgroundDock::waterLevelChanged);

	// second column: water slope dial
	layout->setColumnStretch(1, 0);

	waterSlopeDial = new QDial(settingsWidget);
	waterSlopeDial->setMinimumSize(100, 100);
	waterSlopeDial->setRange(-400, 400);
	waterSlopeDial->setToolTip("<p><b>Water slope</b></p>"
	                           "<p>This changes the water slope, and can be used for detrending the river.</p>"
	                           "<p>This setting is used only for visualization purposes, and does not influence the resulting network.</p>");
	layout->addWidget(waterSlopeDial, 0, 1, 2, 1, Qt::AlignHCenter);

	connect(waterSlopeDial, &QSlider::valueChanged,
	        this, &BackgroundDock::waterSlopeChanged);

	resetButton = new QPushButton(settingsWidget);
	resetButton->setText("Reset");
	resetButton->setToolTip("<p><b>Reset water height and slope</b></p>"
	                        "<p>This resets the water height and the water slope to their original values.</p>");
	layout->addWidget(resetButton, 2, 0, 1, 2, Qt::AlignHCenter);

	connect(resetButton, &QPushButton::pressed,
	        this, &BackgroundDock::resetSettings);

	updateLabels();
}

int BackgroundDock::waterLevel() {
	return waterLevelSlider->value();
}

int BackgroundDock::waterSlope() {
	return waterSlopeDial->value();
}

void BackgroundDock::resetSettings() {
	waterLevelSlider->setValue(128 * 256 * 256);
	waterSlopeDial->setValue(0);
}

void BackgroundDock::setUnits(Units units) {
	m_units = units;
	updateLabels();
}

void BackgroundDock::updateLabels() {
	QString elevationString =
	        UnitsHelper::formatElevation(
	            m_units.toRealElevation(waterLevelSlider->value()));
	waterLevelLabel->setText(QString("Water level: <b>%1</b>")
	                         .arg(elevationString));
}
