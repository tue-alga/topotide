#include <cmath>

#include <QGridLayout>

#include "settingsdock.h"

#include "unitshelper.h"

SettingsDock::SettingsDock(QWidget* parent) :
        QDockWidget("Algorithm settings", parent) {

	settingsWidget = new QWidget(this);
	setWidget(settingsWidget);

	auto* layout = new QGridLayout();
	settingsWidget->setLayout(layout);

	layout->setRowStretch(0, 1);
	layout->setRowStretch(1, 1);

	msThresholdSlider = new QSlider(Qt::Horizontal, settingsWidget);
	msThresholdSlider->setRange(0, 800); // threshold from 10^0 to 10^8
	msThresholdSlider->setSingleStep(1);
	msThresholdSlider->setPageStep(10);
	msThresholdSlider->setValue(400);
	msThresholdSlider->setToolTip("<p><b>Morse-Smale threshold</b></p>"
	                              "<p>This value determines how much the Morse-Smale complex is simplified. "
	                              "If the sand volume on any side of a saddle stays below this threshold, the saddle is removed.</p>");
	layout->addWidget(msThresholdSlider, 0, 0, Qt::AlignBottom);

	msThresholdLabel = new QLabel();
	layout->addWidget(msThresholdLabel, 1, 0, Qt::AlignHCenter | Qt::AlignTop);
	connect(msThresholdSlider, &QSlider::valueChanged, this, &SettingsDock::updateLabels);
	connect(msThresholdSlider, &QSlider::valueChanged,
	        [this] { emit msThresholdChanged(msThresholdSlider->value()); });

	updateLabels();
}

double SettingsDock::msThreshold() {
	return pow(10, msThresholdSlider->value() / 100.0);
}

void SettingsDock::setUnits(Units units) {
	m_units = units;
	updateLabels();
}

void SettingsDock::updateLabels() {
	QString thresholdString =
	        UnitsHelper::formatVolume(
	            m_units.toRealVolume(msThreshold()));
	msThresholdLabel->setText(
	            QString("Simplification threshold (δ): <b>%1</b>")
	            .arg(thresholdString));
}
