#include <cmath>

#include <QGridLayout>

#include "timedock.h"

TimeDock::TimeDock(QWidget* parent) :
        QDockWidget("Time", parent) {

	container = new QWidget(this);
	setWidget(container);

	auto* layout = new QGridLayout();
	container->setLayout(layout);

	frameSlider = new QSlider(Qt::Horizontal, container);
	frameSlider->setRange(1, m_frameCount); // threshold from 10^0 to 10^15
	frameSlider->setSingleStep(1);
	frameSlider->setValue(1);
	frameSlider->setToolTip(
	    "<p><b>Time slider</b></p>"
	    "<p>This slider allows you to browse through the frames in the time series.</p>");
	layout->addWidget(frameSlider, 0, 0);

	connect(frameSlider, &QSlider::valueChanged,
	        [this] { emit frameChanged(frameSlider->value() - 1); });
}

void TimeDock::setFrame(int frame) {
	frameSlider->setValue(frame);
}

void TimeDock::setFrameCount(int count) {
	frameSlider->setMaximum(count);
}
