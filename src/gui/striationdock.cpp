#include "striationdock.h"

#include <QGridLayout>

StriationDock::StriationDock(QWidget *parent) :
        QDockWidget("Striation details", parent) {

	striationWidget = new QWidget(this);
	auto* layout = new QGridLayout();
	striationWidget->setLayout(layout);
	setWidget(striationWidget);

	statusLabel = new QLabel();

	layout->addWidget(statusLabel, 0, 0, 1, 3, Qt::AlignHCenter);

	toTopButton = new QPushButton("View top part");
	toTopButton->setStyleSheet("background-color: #eecc66;");
	layout->addWidget(toTopButton, 1, 0, Qt::AlignHCenter);
	connect(toTopButton, &QPushButton::pressed,
	        this, &StriationDock::goToTop);

	toRootButton = new QPushButton("Back to root");
	layout->addWidget(toRootButton, 1, 1, Qt::AlignHCenter);
	connect(toRootButton, &QPushButton::pressed,
	        this, &StriationDock::goToRoot);

	toBottomButton = new QPushButton("View bottom part");
	toBottomButton->setStyleSheet("background-color: #88dd77;");
	layout->addWidget(toBottomButton, 1, 2, Qt::AlignHCenter);
	connect(toBottomButton, &QPushButton::pressed,
	        this, &StriationDock::goToBottom);
}

void StriationDock::setStriation(Striation* striation) {
	m_striation = striation;
	m_item = 0;

	if (striation == nullptr) {
		setEnabled(false);
	} else {
		setEnabled(true);
		show();
		update();
	}
}

void StriationDock::goToTop() {
	m_item = m_striation->item(m_item).m_topItem;
	update();
	emit itemChanged(m_item);
}

void StriationDock::goToRoot() {
	m_item = 0;
	update();
	emit itemChanged(m_item);
}

void StriationDock::goToBottom() {
	m_item = m_striation->item(m_item).m_bottomItem;
	update();
	emit itemChanged(m_item);
}

void StriationDock::update() {
	statusLabel->setText(QString("Carving face <b>%1</b>")
	                     .arg(m_striation->item(m_item).m_face));
	toTopButton->setEnabled(m_striation->item(m_item).m_topItem != -1);
	toBottomButton->setEnabled(m_striation->item(m_item).m_bottomItem != -1);
}
