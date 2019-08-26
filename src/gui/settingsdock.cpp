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

	layout->setRowStretch(0, 0);
	layout->setRowStretch(1, 1);
	layout->setRowStretch(2, 0);

	algorithmBox = new QComboBox(settingsWidget);
	algorithmBox->addItem("Persistence-based algorithm");
	algorithmBox->addItem("Striation-based algorithm");
	algorithmBox->setToolTip("<p><b>Algorithm</b></p>"
	                         "<p>This chooses the algorithm to use. "
	                         "The persistence-based algorithm is recommended, "
	                         "as it is more stable over time.</p>");
	layout->addWidget(algorithmBox, 0, 0, Qt::AlignHCenter);
	connect(algorithmBox,
	        static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
	        [=](const int item) {
		emit msSimplifyChanged(algorithmBox->currentIndex() == 1);
	});

	stack = new QStackedWidget(settingsWidget);
	layout->addWidget(stack, 1, 0, Qt::AlignHCenter);

	// old algorithm settings
	oldAlgorithmSettings = new QWidget(stack);
	auto* oldLayout = new QGridLayout();
	oldAlgorithmSettings->setLayout(oldLayout);

	oldLayout->setRowStretch(0, 1);
	oldLayout->setRowStretch(1, 1);
	oldLayout->setRowStretch(2, 1);
	oldLayout->setRowStretch(3, 1);
	oldLayout->setRowStretch(4, 1);
	oldLayout->setColumnStretch(0, 0);
	oldLayout->setColumnStretch(1, 1);
	oldLayout->setColumnStretch(2, 0);

	QLabel* striationStrategyLabel = new QLabel("Striation:");
	oldLayout->addWidget(striationStrategyLabel, 0, 0, Qt::AlignRight);
	striationStrategyBox = new QComboBox(oldAlgorithmSettings);
	striationStrategyBox->addItem("Highest persistence first");
	striationStrategyBox->addItem("Hybrid");
	striationStrategyBox->setCurrentIndex(0);
	striationStrategyBox->setToolTip("<p><b>Striation</b></p>"
	                            "<p>This chooses the strategy to use for the striation.</p>");
	oldLayout->addWidget(striationStrategyBox, 0, 1, Qt::AlignHCenter);
	connect(striationStrategyBox,
	        static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
	        this,
	        &SettingsDock::striationStrategyChanged);

	deltaSlider = new QSlider(Qt::Horizontal, oldAlgorithmSettings);
	deltaSlider->setRange(0, 150); // δ from 10^0 to 10^15
	deltaSlider->setSingleStep(1);
	deltaSlider->setPageStep(10);
	deltaSlider->setValue(90);
	deltaSlider->setToolTip("<p><b>Network δ value</b></p>"
	                        "<p>This sets the minimum volume of sediment that needs to reside between two paths to consider them significally different. "
	                        "Only significantly different paths are included in the network, so setting δ higher decreases the number of paths, while setting "
	                        "δ lower makes more paths visible.</p>");
	oldLayout->addWidget(deltaSlider, 1, 0, 1, 3, Qt::AlignBottom);

	deltaLabel = new QLabel();
	oldLayout->addWidget(deltaLabel, 2, 0, 1, 3, Qt::AlignHCenter | Qt::AlignTop);

	connect(deltaSlider, &QSlider::valueChanged, this, &SettingsDock::updateLabels);
	connect(deltaSlider, &QSlider::sliderReleased, [=] {
		emit deltaChanged(deltaSlider->value());
	});

	QLabel* sandFunctionLabel = new QLabel("Sand function:");
	oldLayout->addWidget(sandFunctionLabel, 3, 0, Qt::AlignRight);
	sandFunctionBox = new QComboBox(oldAlgorithmSettings);
	sandFunctionBox->addItem("Water level model");
	sandFunctionBox->addItem("Water flow model");
	sandFunctionBox->addItem("Symmetric flow model");
	sandFunctionBox->setCurrentIndex(1);
	sandFunctionBox->setToolTip("<p><b>Sand function</b></p>"
	                            "<p>This chooses the sand function to use. "
	                            "The sand function is the function that determines how large the volume of sand between two paths is.</p>");
	oldLayout->addWidget(sandFunctionBox, 3, 1, Qt::AlignHCenter);
	connect(sandFunctionBox,
	        static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
	        this,
	        &SettingsDock::sandFunctionChanged);

	bidirectionalBox = new QCheckBox(oldAlgorithmSettings);
	bidirectionalBox->setChecked(false);
	bidirectionalBox->setText("Bidirectional");
	bidirectionalBox->setToolTip("<p><b>Bidirectional sand function</b></p>"
	                             "<p>This setting changes the way the amount of sediment between two paths is computed. "
	                             "If it is unchecked, the sediment is computed only from the lower to the higher path, not the other way round. "
	                             "If it is checked, the sediment is computed in both directions, and the largest of the two is taken.</p>"
	                             "<p>Checking this box may result in better networks, that usually have fewer paths. "
	                             "In practice however the improvement is not very large.</p>");
	oldLayout->addWidget(bidirectionalBox, 3, 2, Qt::AlignHCenter);

	connect(bidirectionalBox, &QCheckBox::stateChanged,
	        this, &SettingsDock::bidirectionalChanged);

	simplifyBox = new QCheckBox(oldAlgorithmSettings);
	simplifyBox->setChecked(true);
	simplifyBox->setText("Simplify network");
	simplifyBox->setToolTip("<p><b>Simplify network</b></p>"
	                        "<p>This setting simplifies the resulting network by removing degree-2 vertices.</p>");
	oldLayout->addWidget(simplifyBox, 4, 1, Qt::AlignHCenter);

	connect(simplifyBox, &QCheckBox::stateChanged,
	        this, &SettingsDock::simplifyChanged);

	// new algorithm settings
	newAlgorithmSettings = new QWidget(stack);
	auto* newLayout = new QGridLayout();
	newAlgorithmSettings->setLayout(newLayout);
	newLayout->setColumnStretch(0, 1);

	msThresholdSlider = new QSlider(Qt::Horizontal, newAlgorithmSettings);
	msThresholdSlider->setRange(0, 150); // threshold from 10^0 to 10^15
	msThresholdSlider->setSingleStep(1);
	msThresholdSlider->setPageStep(10);
	msThresholdSlider->setValue(50);
	msThresholdSlider->setToolTip("<p><b>Morse-Smale threshold</b></p>"
	                              "<p>This value determines how much the Morse-Smale complex is simplified. "
	                              "If the sand volume on any side of a saddle stays below this threshold, the saddle is removed.</p>");
	newLayout->addWidget(msThresholdSlider, 0, 0, Qt::AlignBottom);

	msThresholdLabel = new QLabel();
	newLayout->addWidget(msThresholdLabel, 1, 0, Qt::AlignHCenter | Qt::AlignTop);
	connect(msThresholdSlider, &QSlider::valueChanged, this, &SettingsDock::updateLabels);
	connect(msThresholdSlider, &QSlider::valueChanged, [=] {
		emit msThresholdChanged(msThresholdSlider->value());
	});

	// set up the QStackedWidget
	stack->addWidget(newAlgorithmSettings);
	stack->addWidget(oldAlgorithmSettings);
	connect(algorithmBox,
	        static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
	        stack,
	        &QStackedWidget::setCurrentIndex);

	updateLabels();
}

bool SettingsDock::striationStrategy() {
	return striationStrategyBox->currentIndex() == 0;
}

// FIXME it is highly misleading that this returns a double, while this actually
// just returns the slider value - not the converted 10^... value
double SettingsDock::delta() {
	return deltaSlider->value();
}

int SettingsDock::sandFunction() {
	return sandFunctionBox->currentIndex();
}

bool SettingsDock::bidirectional() {
	return bidirectionalBox->isChecked();
}

bool SettingsDock::simplify() {
	return simplifyBox->isChecked();
}

bool SettingsDock::msSimplify() {
	return algorithmBox->currentIndex() == 0;
}

double SettingsDock::msThreshold() {
	return pow(10, msThresholdSlider->value() / 10.0);
}

void SettingsDock::setUnits(Units units) {
	m_units = units;
	updateLabels();
}

void SettingsDock::updateLabels() {

	QString deltaString =
	        UnitsHelper::formatVolume(
	            m_units.toRealVolume(pow(10, deltaSlider->value() / 10.0)));
	deltaLabel->setText(
	            QString("Network δ: <b>%1</b>")
	            .arg(deltaString));

	QString thresholdString =
	        UnitsHelper::formatVolume(
	            m_units.toRealVolume(msThreshold()));
	msThresholdLabel->setText(
	            QString("Simplification threshold (δ): <b>%1</b>")
	            .arg(thresholdString));
}
