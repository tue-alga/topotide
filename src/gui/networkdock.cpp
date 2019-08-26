#include "networkdock.h"
#include "uihelper.h"

#include <QGridLayout>

NetworkDock::NetworkDock(QWidget *parent) :
        QDockWidget("Network details", parent) {

	networkWidget = new QWidget(this);
	auto* layout = new QGridLayout();
	networkWidget->setLayout(layout);
	setWidget(networkWidget);

	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 0);
	layout->setColumnStretch(2, 1);
	layout->setHorizontalSpacing(10);

	statusLabel = new QLabel();
	layout->addWidget(statusLabel, 0, 1, Qt::AlignHCenter);

	toPreviousButton = new QPushButton;
	toPreviousButton->setIcon(UiHelper::createIcon("go-previous"));
	layout->addWidget(toPreviousButton, 0, 0, Qt::AlignRight);
	connect(toPreviousButton, &QPushButton::pressed,
	        this, &NetworkDock::goToPrevious);

	toNextButton = new QPushButton;
	toNextButton->setIcon(UiHelper::createIcon("go-next"));
	layout->addWidget(toNextButton, 0, 2, Qt::AlignLeft);
	connect(toNextButton, &QPushButton::pressed,
	        this, &NetworkDock::goToNext);
}

void NetworkDock::setNetwork(Network* network) {
	m_network = network;
	m_path = 0;

	if (network == nullptr) {
		setEnabled(false);
	} else {
		setEnabled(true);
		show();
		update();
	}
}

void NetworkDock::goToPrevious() {
	m_path--;
	update();
	emit pathChanged(m_path);
}

void NetworkDock::goToNext() {
	m_path++;
	update();
	emit pathChanged(m_path);
}

void NetworkDock::update() {
	statusLabel->setText(QString("Network path <b>%1</b>")
	                     .arg(m_path));
	toPreviousButton->setEnabled(m_path > 0);
	toNextButton->setEnabled(m_path + 1 < m_network->paths().size());
}
