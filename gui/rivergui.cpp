#include "rivergui.h"

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QIcon>
#include <QImage>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QStatusBar>
#include <QString>
#include <QStyle>

#include "boundaryreader.h"
#include "boundarywriter.h"
#include "io/esrigridreader.h"
#include "io/gdalreader.h"
#include "io/textfilereader.h"
#include "linksequence.h"

#include "backgroundthread.h"
#include "graphwriter.h"
#include "linksequencewriter.h"
#include "uihelper.h"
#include "unitsdock.h"

RiverGui::RiverGui() {
	createGui();
	createActions();
	createMenu();
	createToolBar();
	setWindowIcon(UiHelper::createIcon("topotide"));
	initializeDropping();
	resize(1280, 720);
	updateActions();
}

void RiverGui::createGui() {

	// river map
	map = new RiverWidget();
	setCentralWidget(map);

	// background dock
	backgroundDock = new BackgroundDock(this);
	connect(backgroundDock, &BackgroundDock::showElevationChanged,
	        map, &RiverWidget::setShowElevation);
	map->setShowElevation(backgroundDock->showElevation());
	connect(backgroundDock, &BackgroundDock::themeChanged,
	        map, &RiverWidget::setTheme);
	map->setTheme(backgroundDock->theme());
	connect(backgroundDock, &BackgroundDock::showWaterPlaneChanged,
	        map, &RiverWidget::setShowWaterPlane);
	map->setShowWaterPlane(backgroundDock->showWaterPlane());
	connect(backgroundDock, &BackgroundDock::waterLevelChanged,
	        map, &RiverWidget::setWaterLevel);
	map->setWaterLevel(backgroundDock->waterLevel());
	connect(backgroundDock, &BackgroundDock::showContoursChanged,
	        map, &RiverWidget::setShowOutlines);
	map->setShowOutlines(backgroundDock->showContours());
	connect(backgroundDock, &BackgroundDock::contourCountChanged,
	        map, &RiverWidget::setContourCount);
	map->setContourCount(backgroundDock->contourCount());
	connect(backgroundDock, &BackgroundDock::showShadingChanged,
	        map, &RiverWidget::setShowShading);
	map->setShowShading(backgroundDock->showShading());
	addDockWidget(Qt::TopDockWidgetArea, backgroundDock);

	// units dock
	unitsDock = new UnitsDock(this);
	connect(unitsDock, &UnitsDock::unitsChanged, [this](Units units) {
		bool needToRecompute =
		        m_riverData->units().m_xResolution != units.m_xResolution ||
		        m_riverData->units().m_yResolution != units.m_yResolution;
		m_riverData->units() = units;
		map->setUnits(units);
		settingsDock->setUnits(units);
		coordinateLabel->setUnits(units);
		if (needToRecompute) {
			markComputationNeeded(false);
		}
	});
	addDockWidget(Qt::TopDockWidgetArea, unitsDock);

	// settings dock
	settingsDock = new SettingsDock(this);
	connect(settingsDock, &SettingsDock::msThresholdChanged, [&] {
		map->setNetworkDelta(settingsDock->msThreshold());
	});
	map->setNetworkDelta(settingsDock->msThreshold());
	addDockWidget(Qt::TopDockWidgetArea, settingsDock);

	// progress viewer
	progressDock = new ProgressDock(this);
	addDockWidget(Qt::TopDockWidgetArea, progressDock);

	// time dock
	timeDock = new TimeDock(this);
	addDockWidget(Qt::BottomDockWidgetArea, timeDock);
	connect(timeDock, &TimeDock::frameChanged, [this](int frame) {
		m_frame = frame;
		map->setRiverFrame(activeFrame());
		m_computationNeeded = true;
		updateActions();
	});

	// status bar
	coordinateLabel = new CoordinateLabel(this);
	statusBar()->addPermanentWidget(coordinateLabel);
	connect(map, &RiverWidget::hoveredCoordinateChanged,
	        coordinateLabel, &CoordinateLabel::setCoordinate);
	connect(map, &RiverWidget::mouseLeft,
	        coordinateLabel, &CoordinateLabel::clear);
}

void RiverGui::createActions() {

	openAction = new QAction("&Open DEM...", this);
	openAction->setShortcuts(QKeySequence::Open);
	openAction->setIcon(UiHelper::createIcon("document-open"));
	openAction->setToolTip("Open a river image");
	connect(openAction, &QAction::triggered, this, &RiverGui::openFrame);

	openTimeSeriesAction = new QAction("&Open time series...", this);
	openTimeSeriesAction->setIcon(UiHelper::createIcon("document-open"));
	openTimeSeriesAction->setToolTip("Open a series of river images");
	connect(openTimeSeriesAction, &QAction::triggered, this, &RiverGui::openFrames);

	openBoundaryAction = new QAction("&Open boundary...", this);
	openBoundaryAction->setIcon(UiHelper::createIcon("document-open"));
	openBoundaryAction->setToolTip("Open a river boundary");
	connect(openBoundaryAction, &QAction::triggered, this, &RiverGui::openBoundary);

	saveGraphAction = new QAction("Save graph...", this);
	saveGraphAction->setIcon(UiHelper::createIcon("document-save"));
	saveGraphAction->setToolTip("Save the computed network as a text document");
	connect(saveGraphAction, &QAction::triggered, this, &RiverGui::saveGraph);

	saveLinkSequenceAction = new QAction("&Save link sequence...", this);
	saveLinkSequenceAction->setIcon(UiHelper::createIcon("document-save"));
	saveLinkSequenceAction->setToolTip("Save the link sequence of the computed network as a text document");
	connect(saveLinkSequenceAction, &QAction::triggered, this, &RiverGui::saveLinkSequence);

	saveBoundaryAction = new QAction("&Save boundary...", this);
	saveBoundaryAction->setIcon(UiHelper::createIcon("document-save"));
	saveBoundaryAction->setToolTip("Save a river boundary");
	connect(saveBoundaryAction, &QAction::triggered, this, &RiverGui::saveBoundary);

	quitAction = new QAction("&Quit", this);
	quitAction->setShortcuts(QKeySequence::Quit);
	quitAction->setIcon(UiHelper::createIcon("application-exit"));
	quitAction->setToolTip("Quit the application");
	connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

	showBackgroundAction = new QAction("&Background", this);
	showBackgroundAction->setIcon(UiHelper::createIcon("compass"));
	showBackgroundAction->setToolTip("Show or hide the background");
	showBackgroundAction->setCheckable(true);
	showBackgroundAction->setChecked(true);
	connect(showBackgroundAction, &QAction::triggered, map, &RiverWidget::setDrawBackground);
	connect(showBackgroundAction, &QAction::triggered, this, &RiverGui::updateActions);

	showInputDcelAction = new QAction("Show &input graph", this);
	showInputDcelAction->setIcon(UiHelper::createIcon("color-gradient"));
	showInputDcelAction->setToolTip("Show or hide the input graph");
	showInputDcelAction->setCheckable(true);
	showInputDcelAction->setChecked(false);
	connect(showInputDcelAction, &QAction::toggled, map, &RiverWidget::setShowInputDcel);

	showMsComplexAction = new QAction("Show &MS complex", this);
	showMsComplexAction->setIcon(UiHelper::createIcon("color-gradient"));
	showMsComplexAction->setToolTip("Show or hide the Morse-Smale complex");
	showMsComplexAction->setCheckable(true);
	showMsComplexAction->setChecked(false);
	connect(showMsComplexAction, &QAction::toggled, map, &RiverWidget::setShowMsComplex);

	msEdgesStraightAction = new QAction("&Straight MS edges", this);
	msEdgesStraightAction->setIcon(UiHelper::createIcon("color-gradient"));
	msEdgesStraightAction->setToolTip("Draw MS edges as straight lines");
	msEdgesStraightAction->setCheckable(true);
	msEdgesStraightAction->setChecked(false);
	connect(msEdgesStraightAction, &QAction::toggled, map, &RiverWidget::setMsEdgesStraight);

	editBoundaryAction = new QAction("&Edit boundary", this);
	editBoundaryAction->setIcon(UiHelper::createIcon("draw-freehand"));
	editBoundaryAction->setCheckable(true);
	editBoundaryAction->setToolTip("Delineate the part of the river to be analyzed, by drawing a boundary");
	connect(editBoundaryAction, &QAction::toggled, [&] {
		if (editBoundaryAction->isChecked()) {
			Boundary boundary{m_riverData->boundary()};
			map->startBoundaryEditMode(boundary);
		} else {
			Boundary boundary = map->endBoundaryEditMode();
			m_riverData->setBoundary(boundary);
			markComputationNeeded(true);
		}
		updateActions();
	});

	computeAction = new QAction("&Compute network", this);
	computeAction->setShortcut(QKeySequence("Ctrl+C"));
	computeAction->setIcon(UiHelper::createIcon("run-build"));
	computeAction->setToolTip("Start the computation");
	connect(computeAction, &QAction::triggered,
	        this, &RiverGui::startComputation);

	zoomInAction = new QAction("Zoom &in", this);
	zoomInAction->setShortcuts(QKeySequence::ZoomIn);
	zoomInAction->setIcon(UiHelper::createIcon("zoom-in"));
	zoomInAction->setToolTip("Make the river display larger");
	connect(zoomInAction, &QAction::triggered,
	        map, &RiverWidget::zoomIn);

	zoomOutAction = new QAction("Zoom &out", this);
	zoomOutAction->setShortcuts(QKeySequence::ZoomOut);
	zoomOutAction->setIcon(UiHelper::createIcon("zoom-out"));
	zoomOutAction->setToolTip("Make the river display smaller");
	connect(zoomOutAction, &QAction::triggered,
	        map, &RiverWidget::zoomOut);

	fitToViewAction = new QAction("&Fit to view", this);
	fitToViewAction->setIcon(UiHelper::createIcon("zoom-fit-best"));
	fitToViewAction->setToolTip("Reset the zoom such that the river fits inside the viewport");
	connect(fitToViewAction, &QAction::triggered,
	        map, &RiverWidget::resetTransform);

	aboutAction = new QAction("&About TopoTide...", this);
	aboutAction->setToolTip("Show some information about this application");
	connect(aboutAction, &QAction::triggered, this, &RiverGui::about);

	aboutQtAction = new QAction("About &Qt...", this);
	aboutQtAction->setToolTip("Show the Qt library's About box");
	connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void RiverGui::updateActions() {
	if (m_riverData != nullptr) {
		setWindowTitle(m_riverData->getFrame(m_frame)->m_name + " — TopoTide");
	} else {
		setWindowTitle("TopoTide");
	}

	showInputDcelAction->setEnabled(m_riverData != nullptr && activeFrame()->m_inputDcel != nullptr &&
	                                !map->boundaryEditMode());
	showMsComplexAction->setEnabled(m_riverData != nullptr && activeFrame()->m_msComplex != nullptr &&
	                                !map->boundaryEditMode());

	openBoundaryAction->setEnabled(m_riverData != nullptr && !map->boundaryEditMode());
	saveBoundaryAction->setEnabled(m_riverData != nullptr && !map->boundaryEditMode());
	editBoundaryAction->setEnabled(m_riverData != nullptr);

	saveGraphAction->setEnabled(m_riverData != nullptr && activeFrame()->m_networkGraph != nullptr);
	saveLinkSequenceAction->setEnabled(m_riverData != nullptr && activeFrame()->m_networkGraph != nullptr);
	showBackgroundAction->setEnabled(m_riverData != nullptr);

	computeAction->setEnabled(m_riverData != nullptr && m_computationNeeded &&
	                          !m_computationRunning && !map->boundaryEditMode());

	zoomInAction->setEnabled(m_riverData != nullptr);
	zoomOutAction->setEnabled(m_riverData != nullptr);
	fitToViewAction->setEnabled(m_riverData != nullptr);

	backgroundDock->setVisible(m_riverData != nullptr);
	backgroundDock->setEnabled(m_riverData != nullptr);

	settingsDock->setVisible(m_riverData != nullptr);
	settingsDock->setEnabled(m_riverData != nullptr && !m_computationRunning);

	unitsDock->setVisible(m_riverData != nullptr);
	unitsDock->setEnabled(m_riverData != nullptr && !m_computationRunning);

	progressDock->setVisible(m_riverData != nullptr);

	timeDock->setVisible(m_riverData != nullptr && m_riverData->frameCount() > 1);
}

void RiverGui::createMenu() {
	fileMenu = menuBar()->addMenu("&File");
	fileMenu->addAction(openAction);
	fileMenu->addAction(openTimeSeriesAction);
	fileMenu->addAction(saveGraphAction);
	fileMenu->addAction(saveLinkSequenceAction);
	fileMenu->addSeparator();
	fileMenu->addAction(openBoundaryAction);
	fileMenu->addAction(saveBoundaryAction);
	fileMenu->addSeparator();
	fileMenu->addAction(quitAction);

	editMenu = menuBar()->addMenu("&Edit");
	editMenu->addAction(editBoundaryAction);

	runMenu = menuBar()->addMenu("&Run");
	runMenu->addAction(computeAction);

	viewMenu = menuBar()->addMenu("&View");
	viewMenu->addAction(showBackgroundAction);
	viewMenu->addAction(zoomInAction);
	viewMenu->addAction(zoomOutAction);
	viewMenu->addAction(fitToViewAction);
	viewMenu->addSeparator();
	viewMenu->addAction(showInputDcelAction);
	viewMenu->addAction(showMsComplexAction);

	helpMenu = menuBar()->addMenu("&Help");
	helpMenu->addAction(aboutAction);
	helpMenu->addAction(aboutQtAction);
}

void RiverGui::createToolBar() {
	toolBar = addToolBar("Main toolbar");

	toolBar->addAction(computeAction);
	toolBar->addSeparator();
	toolBar->addAction(showBackgroundAction);
	toolBar->addAction(editBoundaryAction);
	toolBar->addSeparator();
	toolBar->addAction(zoomInAction);
	toolBar->addAction(zoomOutAction);
	toolBar->addAction(fitToViewAction);

	toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

void RiverGui::initializeDropping() {
	setAcceptDrops(true);
}

void RiverGui::dragEnterEvent(QDragEnterEvent *event) {
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	}
}

void RiverGui::dropEvent(QDropEvent *event) {
	QList<QUrl> urls = event->mimeData()->urls();
	QStringList fileNames;
	for (QUrl& url : urls) {
		fileNames << url.toLocalFile();
	}
	openFramesNamed(fileNames);
}

void RiverGui::openFrame() {
	QString fileName = QFileDialog::getOpenFileName(
	            this,
	            "Open river heightmap",
	            ".");
	if (fileName == nullptr) {
		return;
	}

	QStringList fileNames(fileName);
	openFramesNamed(fileNames);
}

void RiverGui::openFrames() {
	QStringList fileNames = QFileDialog::getOpenFileNames(
	            this,
	            "Open river time series",
	            ".");
	if (fileNames.empty()) {
		return;
	}

	openFramesNamed(fileNames);
}

void RiverGui::openFramesNamed(QStringList& fileNames) {
	if (m_computationRunning) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot open river");
		msgBox.setText("<qt>The river cannot be opened.");
		msgBox.setInformativeText("<qt>There is still a running computation. "
		                          "Wait until the computation is finished, and "
		                          "try again.");
		msgBox.exec();
		return;
	}

	fileNames.sort();

	std::shared_ptr<RiverData> riverData;
	Units units;
	for (QString fileName : fileNames) {
		std::shared_ptr<RiverFrame> frame = loadFrame(fileName, units);
		if (!frame) {
			break;
		}
		if (riverData) {
			if (frame->m_heightMap.width() != riverData->width() ||
			    frame->m_heightMap.height() != riverData->height()) {
				return;
			}
		} else {
			riverData =
			    std::make_shared<RiverData>(frame->m_heightMap.width(), frame->m_heightMap.height(), units);
		}
		riverData->addFrame(frame);
	}

	if (!riverData || riverData->frameCount() == 0) {
		m_riverData = nullptr;
		map->setRiverData(nullptr);
		map->setRiverFrame(nullptr);
		updateActions();
		return;
	}
	m_riverData = riverData;
	m_frame = 0;
	map->setRiverData(m_riverData);
	map->setRiverFrame(activeFrame());
	map->resetTransform();
	backgroundDock->setElevationRange(m_riverData->minimumElevation(), m_riverData->maximumElevation());
	unitsDock->setUnits(m_riverData->units());
	progressDock->reset();
	timeDock->setFrame(0);
	timeDock->setFrameCount(m_riverData->frameCount());
	updateActions();
	markComputationNeeded(false);

	if (fileNames.length() == 1) {
		statusBar()->showMessage("Opened river \"" + fileNames[0] + "\"", 5000);
	} else {
		statusBar()->showMessage(
		    QString("Opened river containing %1 frames").arg(fileNames.length()), 5000);
	}
}
  
std::shared_ptr<RiverFrame> RiverGui::loadFrame(const QString& fileName, Units& units) {
	HeightMap heightMap;
	QString error = "[no error given]";
	if (fileName.endsWith(".txt")) {
		heightMap = TextFileReader::readTextFile(fileName, error, units);
	} else if (fileName.endsWith(".ascii") || fileName.endsWith(".asc")) {
		heightMap = EsriGridReader::readGridFile(fileName, error, units);
	} else {
		heightMap = GdalReader::readGdalFile(fileName, error, units);
	}

	if (heightMap.isEmpty()) {
		// something went wrong
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot open river");
		msgBox.setText(QString("<qt>The river <code>%1</code> cannot be opened.").arg(fileName));
		msgBox.setInformativeText("<qt><p>This file does not seem to be an image file "
								"or a valid text file containing elevation "
								"data.</p>");
		msgBox.setDetailedText("Reading the file failed due to the "
							"following error:\n    " + error);
		msgBox.exec();
		return nullptr;
	}

	return std::make_shared<RiverFrame>(fileName, heightMap);
}

void RiverGui::openBoundary() {
	QString fileName = QFileDialog::getOpenFileName(
	            this,
	            "Open river boundary",
	            ".",
	            "Boundary text files (*.txt)");
	if (fileName == nullptr) {
		return;
	}

	if (m_computationRunning) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot open boundary");
		msgBox.setText("<qt>The river boundary cannot be opened.");
		msgBox.setInformativeText("<qt>There is still a running computation. "
		                          "Wait until the computation is finished, and "
		                          "try again.");
		msgBox.exec();
		return;
	}

	QString error = "";
	Boundary boundary =
	    BoundaryReader::readBoundary(fileName, m_riverData->width(), m_riverData->height(), error);

	if (error != "") {
		// something went wrong
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot open boundary");
		msgBox.setText(QString("<qt>The river boundary <code>%1</code> cannot be opened.").arg(fileName));
		msgBox.setInformativeText("<qt><p>This file does not seem to be "
		                          "a valid text file containing a river "
		                          "boundary.</p>");
		msgBox.setDetailedText("Reading the text file failed due to the "
		                       "following error:\n    " + error);
		msgBox.exec();
		return;
	}

	m_riverData->setBoundary(boundary);
	map->update();
	markComputationNeeded(false);
}

void RiverGui::saveGraph() {

	QString fileName = QFileDialog::getSaveFileName(this,
	        "Save graph",
	        ".",
	        "Text files (*.txt)");
	if (fileName == nullptr) {
		return;
	}

	if (m_computationRunning) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot write graph");
		msgBox.setText("<qt>The graph cannot be saved.");
		msgBox.setInformativeText("<qt>There is still a running computation."
		                          "Wait until the computation is finished, and "
		                          "try again.");
		msgBox.exec();
		return;
	}

	QReadLocker lock(&activeFrame()->m_networkGraphLock);

	NetworkGraph graph = *activeFrame()->m_networkGraph;
	graph.filterOnDelta(settingsDock->msThreshold());
	GraphWriter::writeGraph(graph, m_riverData->units(), fileName);

	statusBar()->showMessage("Saved graph as \"" + fileName + "\"", 5000);
}

void RiverGui::saveLinkSequence() {

	QString fileName = QFileDialog::getSaveFileName(this,
	        "Save link sequence",
	        ".",
	        "Text files (*.txt)");
	if (fileName == nullptr) {
		return;
	}

	if (m_computationRunning) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot write link sequence");
		msgBox.setText("<qt>The link sequence cannot be saved.");
		msgBox.setInformativeText("<qt>There is still a running computation."
		                          "Wait until the computation is finished, and "
		                          "try again.");
		msgBox.exec();
		return;
	}

	QReadLocker lock(&activeFrame()->m_networkGraphLock);

	LinkSequence links(*activeFrame()->m_networkGraph);
	LinkSequenceWriter::writeLinkSequence(links, m_riverData->units(), fileName);

	statusBar()->showMessage("Saved link sequence as \"" +
	                         fileName + "\"", 5000);
}

void RiverGui::saveBoundary() {

	QString fileName = QFileDialog::getSaveFileName(this,
	        "Save boundary",
	        ".",
	        "Boundary text files (*.txt)");
	if (fileName == nullptr) {
		return;
	}

	BoundaryWriter::writeBoundary(m_riverData->boundary(), fileName);

	statusBar()->showMessage("Saved boundary as \"" +
	                         fileName + "\"", 5000);
}

void RiverGui::about() {
	QMessageBox msgBox;
	msgBox.setWindowTitle("About TopoTide");
	msgBox.setText("<qt><h2 style=\"font-weight: normal;\"><b>TopoTide " +
	               QApplication::applicationVersion() + "</b><br>"
	               "Topological Tools for Network Extraction</h2>");
	msgBox.setInformativeText(QString(
	    "<qt>"
	    "<style>a { color: #00a2de; font-weight: bold; }</style>"
	    "<p>GitHub: <a "
	    "href=\"https://github.com/tue-alga/topotide\">https://github.com/tue-alga/topotide</a></p>"
	    "<p>TopoTide is a tool which helps the analysis of river systems, in particular, braided "
	    "rivers and estuaries. The focus of the tool is the computation of networks from a "
	    "digital elevation model (DEM) of the river bed. Such a network can then be used as "
	    "input for further analyses, such as computing the length or average elevation of "
	    "channels.</p>"
	    "<p>This program is free software: you can redistribute it and/or modify "
	    "it under the terms of the GNU General Public License as published by "
	    "the Free Software Foundation, either version 3 of the License, or "
	    "(at your option) any later version.</p>"
	    "<p>This program uses the following libraries:</p>"
	    "<ul>"
	    "<li><a href='https://qt.io'>Qt</a> " QT_VERSION_STR "</li>"
	    "<li><a href='https://gdal.org'>GDAL</a></li>"
	    "<li><a href='https://invent.kde.org/frameworks/breeze-icons'>Breeze icons</a></li>"
	    "</ul>"));
	msgBox.setIconPixmap(QPixmap(":res/icons/topotide-background.svg"));
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.exec();
}

void RiverGui::markComputationNeeded(bool onlyNetwork) {
	if (!m_computationNeeded) {
		m_onlyNetworkNeeded = onlyNetwork;
	} else {
		m_onlyNetworkNeeded = m_onlyNetworkNeeded && onlyNetwork;
	}
	m_computationNeeded = true;
	updateActions();
}

void RiverGui::startComputation() {
	if (!m_computationNeeded) {
		qDebug() << "Tried to start computation while none was needed";
		return;
	}
	if (m_computationRunning) {
		qDebug() << "Tried to start computation while another was still running";
		return;
	}
	
	if (!m_riverData->boundaryRasterized().isValid()) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Boundary invalid");
		msgBox.setText("<qt>The computation cannot run as the boundary is invalid.");
		msgBox.setInformativeText("<qt>A valid boundary does not self-intersect and does not visit "
		                          "any points more than once. "
		                          "Edit the boundary and try again.");
		msgBox.exec();
		return;
	}
	progressDock->reset();
	m_computationRunning = true;
	m_computationNeeded = false;

	activeFrame()->m_inputGraph = nullptr;
	activeFrame()->m_inputDcel = nullptr;
	activeFrame()->m_msComplex = nullptr;
	activeFrame()->m_networkGraph = nullptr;
	map->update();
	updateActions();

	auto* thread = new BackgroundThread(m_riverData, activeFrame(),
	                                    pow(10, settingsDock->msThreshold() / 10.0));

	connect(thread, &BackgroundThread::taskStarted, this, [this](QString task) {
		QThread::currentThread();
		progressDock->startTask(task);
		statusBar()->showMessage(task);
		map->update();
		updateActions();
	});

	connect(thread, &BackgroundThread::progressMade, this, [this](QString task, int progress) {
		progressDock->setProgress(task, progress);
	});

	connect(thread, &BackgroundThread::taskEnded, this, [this](QString task) {
		progressDock->endTask(task);
		statusBar()->clearMessage();
		map->update();
		updateActions();
	});

	connect(thread, &QThread::finished, thread, &QThread::deleteLater);
	connect(thread, &QThread::finished, this, [&] {
		m_computationRunning = false;
		statusBar()->showMessage("Computation finished");
		map->update();
		updateActions();
		setCursor(Qt::ArrowCursor);
	});

	thread->start();
	setCursor(Qt::BusyCursor);
}

std::shared_ptr<RiverFrame> RiverGui::activeFrame() {
	return m_riverData->getFrame(m_frame);
}
