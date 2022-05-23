#include "riverdata.h"

#include <QDebug>

#include "backgroundthread.h"

RiverData::RiverData() {}

RiverData::RiverData(QString name, const QImage& image) :
    name(name), image(image), heightMap(image) {
}

bool RiverData::isInitialized() {
	return image.width() > 0 && image.height() > 0;
}

void RiverData::startComputation(ProgressDock* dock,
                                 QStatusBar* bar,
                                 bool onlyNetwork,
                                 int sandFunction,
                                 bool bidirectional,
                                 bool simplify,
                                 bool hybridStriation,
                                 double delta,
                                 double msThreshold) {

	// sanity check: is the river even initialized?
	if (!isInitialized()) {
		qDebug() << "Tried to start computation on a non-initialized river";
		return;
	}

	// avoid starting several threads
	if (m_threadRunning) {
		qDebug() << "Tried to start computation while another was still running";
		return;
	}
	m_threadRunning = true;

	emit computationStarted();

	deleteEverything(onlyNetwork);
	emit dataChanged();

	auto* thread = new BackgroundThread(
	            this,
	            onlyNetwork,
	            sandFunction,
	            bidirectional,
	            simplify,
	            hybridStriation,
	            delta,
	            msThreshold);

	connect(thread, &BackgroundThread::taskStarted,
			dock, &ProgressDock::startTask);
	connect(thread, SIGNAL(taskStarted(QString)),
			bar, SLOT(showMessage(QString)) );
	connect(thread, &BackgroundThread::progressMade,
			dock, &ProgressDock::setProgress);
	connect(thread, &BackgroundThread::taskEnded,
			dock, &ProgressDock::endTask);
	connect(thread, &BackgroundThread::taskEnded,
			bar, &QStatusBar::clearMessage);

	connect(thread, &BackgroundThread::taskEnded,
			this, &RiverData::dataChanged);

	connect(thread, &BackgroundThread::finished,
	        this, &RiverData::markComputationFinished);

	thread->start();
}

bool RiverData::isThreadRunning() const {
	return m_threadRunning;
}

void RiverData::markComputationFinished() {
	m_threadRunning = false;
	emit computationFinished();
}

void RiverData::deleteEverything(bool onlyNetwork) {

	if (!onlyNetwork) {
		if (inputGraph != nullptr) {
			delete inputGraph;
			inputGraph = nullptr;
		}

		if (inputDcel != nullptr) {
			delete inputDcel;
			inputDcel = nullptr;
		}

		if (msComplex != nullptr) {
			delete msComplex;
			msComplex = nullptr;
		}

		if (striation != nullptr) {
			delete striation;
			striation = nullptr;
		}

		if (sandCache != nullptr) {
			delete sandCache;
			sandCache = nullptr;
		}
	}

	if (network != nullptr) {
		delete network;
		network = nullptr;
	}

	if (networkGraph != nullptr) {
		delete networkGraph;
		networkGraph = nullptr;
	}
}

void RiverData::setBoundary(Boundary& b) {
	boundary = b;
	boundaryRasterized = b.rasterize();
}
