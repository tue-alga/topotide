#include <cmath>

#include <QGridLayout>

#include "backgrounddock.h"

#include "unitshelper.h"

BackgroundDock::BackgroundDock(QWidget* parent) :
        QDockWidget("Background settings", parent) {

	m_settingsWidget = new QWidget(this);
	auto* layout = new QGridLayout();
	m_settingsWidget->setLayout(layout);
	setWidget(m_settingsWidget);

	m_showElevationBox = new QCheckBox("Show elevation", m_settingsWidget);
	m_showElevationBox->setChecked(true);
	connect(m_showElevationBox, &QCheckBox::stateChanged,
	        this, &BackgroundDock::showElevationChanged);
	layout->addWidget(m_showElevationBox, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);

	m_elevationSettings = new QWidget(this);
	layout->addWidget(m_elevationSettings, 1, 0, Qt::AlignLeft | Qt::AlignVCenter);

	QHBoxLayout* elevationLayout = new QHBoxLayout(m_elevationSettings);
	elevationLayout->setContentsMargins(30, 0, 0, 0);
	QLabel* colorSchemeLabel = new QLabel("Color scheme:");
	connect(m_showElevationBox, &QCheckBox::stateChanged,
	        colorSchemeLabel, &QLabel::setEnabled);
	elevationLayout->addWidget(colorSchemeLabel);
	m_colorSchemeBox = new QComboBox(m_elevationSettings);
	connect(m_showElevationBox, &QCheckBox::stateChanged,
	        m_colorSchemeBox, &QComboBox::setEnabled);
	m_colorSchemeBox->addItem("Grayscale", "grayscale");
	m_colorSchemeBox->addItem("Blue to yellow", "blue-yellow");
	m_colorSchemeBox->addItem("Purple", "purples");
	elevationLayout->addWidget(m_colorSchemeBox);
	connect(m_colorSchemeBox,
	        static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
	        [=](const int item) {
				emit themeChanged(theme());
			});

	m_showWaterLevelBox = new QCheckBox("Show water level", m_settingsWidget);
	m_showWaterLevelBox->setChecked(true);
	connect(m_showWaterLevelBox, &QCheckBox::stateChanged,
	        this, &BackgroundDock::showWaterPlaneChanged);
	layout->addWidget(m_showWaterLevelBox, 2, 0, Qt::AlignLeft | Qt::AlignVCenter);

	m_waterLevelSettings = new QWidget(this);
	layout->addWidget(m_waterLevelSettings, 3, 0, Qt::AlignLeft | Qt::AlignVCenter);

	QHBoxLayout* waterLevelLayout = new QHBoxLayout(m_waterLevelSettings);
	waterLevelLayout->setContentsMargins(30, 0, 0, 0);
	QLabel* waterLevelLabel = new QLabel("Level:");
	connect(m_showWaterLevelBox, &QCheckBox::stateChanged,
	        waterLevelLabel, &QLabel::setEnabled);
	waterLevelLayout->addWidget(waterLevelLabel);
	m_waterLevelSlider = new QSlider(Qt::Horizontal, m_settingsWidget);
	connect(m_showWaterLevelBox, &QCheckBox::stateChanged,
	        m_waterLevelSlider, &QSlider::setEnabled);
	m_waterLevelSlider->setRange(0, 256 * 256 * 256);
	m_waterLevelSlider->setSingleStep(256 * 8);
	m_waterLevelSlider->setPageStep(256 * 256 * 8);
	m_waterLevelSlider->setValue(128 * 256 * 256);
	m_waterLevelSlider->setToolTip("<p><b>Water level</b></p>"
	                             "<p>This changes the water level, and can be used for inspecting the river DEM.</p>"
	                             "<p>This setting is used only for visualization purposes, and does not influence the resulting network.</p>");
	waterLevelLayout->addWidget(m_waterLevelSlider);

	m_waterLevelValueLabel = new QLabel();
	connect(m_showWaterLevelBox, &QCheckBox::stateChanged,
	        m_waterLevelValueLabel, &QLabel::setEnabled);
	waterLevelLayout->addWidget(m_waterLevelValueLabel);
	connect(m_waterLevelSlider, &QSlider::valueChanged,
	        this, &BackgroundDock::updateLabels);
	connect(m_waterLevelSlider, &QSlider::valueChanged,
	        this, &BackgroundDock::waterLevelChanged);
	connect(m_colorSchemeBox,
	        static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
	        [=](const int item) {
				emit themeChanged(theme());
			});

	m_waterResetButton = new QPushButton(m_settingsWidget);
	connect(m_showWaterLevelBox, &QCheckBox::stateChanged,
	        m_waterResetButton, &QPushButton::setEnabled);
	m_waterResetButton->setText("Reset");
	m_waterResetButton->setToolTip("<p><b>Reset water height and slope</b></p>"
							"<p>This resets the water height and the water "
							"slope to their original values.</p>");
	waterLevelLayout->addWidget(m_waterResetButton);

	connect(m_waterResetButton, &QPushButton::pressed, this,
			&BackgroundDock::resetSettings);

	m_showContoursBox = new QCheckBox("Show contours", m_settingsWidget);
	m_showContoursBox->setChecked(true);
	connect(m_showContoursBox, &QCheckBox::stateChanged,
	        this, &BackgroundDock::showContoursChanged);
	layout->addWidget(m_showContoursBox, 5, 0, Qt::AlignLeft | Qt::AlignVCenter);

	m_showShadingBox = new QCheckBox("Show shading", m_settingsWidget);
	connect(m_showShadingBox, &QCheckBox::stateChanged,
	        this, &BackgroundDock::showShadingChanged);
	layout->addWidget(m_showShadingBox, 7, 0, Qt::AlignLeft | Qt::AlignVCenter);

	layout->setRowStretch(8, 1);

	updateLabels();
}

bool BackgroundDock::showElevation() {
	return m_showElevationBox->isChecked();
}

QString BackgroundDock::theme() {
	return m_colorSchemeBox->currentData().toString();
}

bool BackgroundDock::showWaterPlane() {
	return m_showWaterLevelBox->isChecked();
}

int BackgroundDock::waterLevel() {
	return m_waterLevelSlider->value();
}

int BackgroundDock::waterSlope() {
	return 0;
}

bool BackgroundDock::showContours() {
	return m_showContoursBox->isChecked();
}

bool BackgroundDock::showShading() {
	return m_showShadingBox->isChecked();
}

void BackgroundDock::resetSettings() {
	m_waterLevelSlider->setValue(128 * 256 * 256);
}

void BackgroundDock::setUnits(Units units) {
	m_units = units;
	updateLabels();
}

void BackgroundDock::updateLabels() {
	QString elevationString =
	        UnitsHelper::formatElevation(
	            m_units.toRealElevation(m_waterLevelSlider->value()));
	m_waterLevelValueLabel->setText(QString("%1")
	                         .arg(elevationString));
}
