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
#include <QToolButton>

#include "backgroundthread.h"
#include "boundaryreader.h"
#include "boundarywriter.h"
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
#include "fingerfinder.h"
#include "gradientfieldsimplifier.h"
#endif
#include "graphwriter.h"
#include "io/esrigridreader.h"
#include "io/esrigridwriter.h"
#include "io/gdalreader.h"
#include "io/textfilereader.h"
#include "linksequence.h"
#include "linksequencewriter.h"
#include "mergetreedock.h"
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
	connect(map, &RiverWidget::boundaryEdited, [&](std::optional<Boundary> boundary) {
		if (boundary) {
			m_riverData->setBoundary(*boundary);
		}
		updateActions();
	});
	connect(map, &RiverWidget::statusMessage, statusBar(), [this](QString message) {
		if (message == "") {
			statusBar()->clearMessage();
		} else {
			statusBar()->showMessage(message);
		}
	});

	// background dock
	backgroundDock = new BackgroundDock(this);
	connect(backgroundDock, &BackgroundDock::showElevationChanged,
	        map, &RiverWidget::setShowElevation);
	map->setShowElevation(backgroundDock->showElevation());
	connect(backgroundDock, &BackgroundDock::colorRampChanged,
	        map, &RiverWidget::setColorRamp);
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
		m_riverData->units() = units;
		map->setUnits(units);
		settingsDock->setUnits(units);
		coordinateLabel->setUnits(units);
	});
	addDockWidget(Qt::TopDockWidgetArea, unitsDock);

	// merge tree dock
	mergeTreeDock = new MergeTreeDock(this);
	addDockWidget(Qt::RightDockWidgetArea, mergeTreeDock);
	mergeTreeDock->setColorRamp(backgroundDock->colorRamp());
	connect(backgroundDock, &BackgroundDock::colorRampChanged,
	        mergeTreeDock, &MergeTreeDock::setColorRamp);
	connect(mergeTreeDock, &MergeTreeDock::pointToHighlightChanged,
	        map, &RiverWidget::setPointToHighlight);
	connect(mergeTreeDock, &MergeTreeDock::hoveredHeightChanged,
	        map, &RiverWidget::setContourLevel);
	connect(mergeTreeDock, &MergeTreeDock::hoveredSubtreeMaskChanged,
	        map, &RiverWidget::setContourMask);
	mergeTreeDock->hide();

	// settings dock
	settingsDock = new SettingsDock(this);
	connect(settingsDock, &SettingsDock::msThresholdChanged, [&] {
		map->setNetworkDelta(settingsDock->msThreshold());
		mergeTreeDock->setDelta(settingsDock->msThreshold());
	});
	map->setNetworkDelta(settingsDock->msThreshold());
	mergeTreeDock->setDelta(settingsDock->msThreshold());
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
		mergeTreeDock->setMergeTree(activeFrame()->m_mergeTree);
		updateActions();
	});

	// status bar
	coordinateLabel = new CoordinateLabel(this);
	statusBar()->addPermanentWidget(coordinateLabel);
	connect(map, &RiverWidget::hoveredCoordinateChanged,
	        coordinateLabel, &CoordinateLabel::setCoordinateAndHeight);
	connect(mergeTreeDock, &MergeTreeDock::hoveredHeightChanged,
	        coordinateLabel, &CoordinateLabel::setHeight);
	connect(map, &RiverWidget::mouseLeft,
	        coordinateLabel, &CoordinateLabel::clear);
}

void RiverGui::createActions() {

	openAction = new QAction("&Open single DEM...", this);
	openAction->setShortcuts(QKeySequence::Open);
	openAction->setIcon(UiHelper::createIcon("document-open"));
	openAction->setToolTip("Open a file containing elevation data");
	connect(openAction, &QAction::triggered, this, &RiverGui::openFrame);

	openTimeSeriesAction = new QAction("&Open time series...", this);
	openTimeSeriesAction->setIcon(UiHelper::createIcon("document-open"));
	openTimeSeriesAction->setToolTip("Open a series of elevation data files");
	connect(openTimeSeriesAction, &QAction::triggered, this, &RiverGui::openFrames);

	saveFrameAction = new QAction("&Save DEM...", this);
	saveFrameAction->setShortcuts(QKeySequence::Save);
	saveFrameAction->setIcon(UiHelper::createIcon("document-save"));
	saveFrameAction->setToolTip("Save an elevation data file");
	connect(saveFrameAction, &QAction::triggered, this, &RiverGui::saveFrame);

	openBoundaryAction = new QAction("&Open boundary...", this);
	openBoundaryAction->setIcon(UiHelper::createIcon("document-open"));
	openBoundaryAction->setToolTip("Open a boundary of the region to analyze");
	connect(openBoundaryAction, &QAction::triggered, this, &RiverGui::openBoundary);

	saveGraphAction = new QAction("Export graph...", this);
	saveGraphAction->setIcon(UiHelper::createIcon("document-export"));
	saveGraphAction->setToolTip("Save the computed network as a text document");
	connect(saveGraphAction, &QAction::triggered, this, &RiverGui::saveGraph);

	saveLinkSequenceAction = new QAction("&Export link sequence...", this);
	saveLinkSequenceAction->setIcon(UiHelper::createIcon("document-export"));
	saveLinkSequenceAction->setToolTip("Save the link sequence of the computed network as a text document");
	connect(saveLinkSequenceAction, &QAction::triggered, this, &RiverGui::saveLinkSequence);

	saveBoundaryAction = new QAction("&Save boundary...", this);
	saveBoundaryAction->setIcon(UiHelper::createIcon("document-save"));
	saveBoundaryAction->setToolTip("Save the drawn boundary to a file");
	connect(saveBoundaryAction, &QAction::triggered, this, &RiverGui::saveBoundary);

	saveImageAction = new QAction("&Export image...", this);
	saveImageAction->setIcon(UiHelper::createIcon("document-export"));
	saveImageAction->setToolTip("Save the currently visible area of the DEM as an image");
	connect(saveImageAction, &QAction::triggered, this, &RiverGui::saveImage);

	closeAction = new QAction("&Close", this);
	closeAction->setIcon(UiHelper::createIcon("document-close"));
	closeAction->setToolTip("Close the currently opened DEM");
	connect(closeAction, &QAction::triggered, this, &RiverGui::closeFrame);

	quitAction = new QAction("&Quit", this);
	quitAction->setShortcuts(QKeySequence::Quit);
	quitAction->setIcon(UiHelper::createIcon("application-exit"));
	quitAction->setToolTip("Quit the application");
	connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

	generateBoundaryAction = new QAction("&Auto-generate boundary", this);
	generateBoundaryAction->setIcon(UiHelper::createIcon("autocorrection"));
	generateBoundaryAction->setToolTip("Draw the boundary around one connected area of surrounded by nodata");
	connect(generateBoundaryAction, &QAction::triggered, [&] {
		map->startBoundaryGenerateMode();
		updateActions();
	});

	setSourceSinkAction = new QAction("&Set source and sink", this);
	setSourceSinkAction->setToolTip("Sets the parts of the boundary that serve as the source and sink");
	connect(setSourceSinkAction, &QAction::triggered, [&] {
		map->startSetSourceSinkMode();
		updateActions();
	});

	resetBoundaryAction = new QAction("&Reset boundary", this);
	resetBoundaryAction->setIcon(UiHelper::createIcon("edit-reset"));
	resetBoundaryAction->setToolTip("Reset the boundary to one that surrounds the entire DEM");
	connect(resetBoundaryAction, &QAction::triggered, this, &RiverGui::resetBoundary);

	showBackgroundAction = new QAction("&Background", this);
	showBackgroundAction->setIcon(UiHelper::createIcon("compass"));
	showBackgroundAction->setToolTip("Show or hide the background");
	showBackgroundAction->setCheckable(true);
	showBackgroundAction->setChecked(true);
	connect(showBackgroundAction, &QAction::triggered, map, &RiverWidget::setDrawBackground);
	connect(showBackgroundAction, &QAction::triggered, this, &RiverGui::updateActions);

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	showSimplifiedAction = new QAction("&Simplified", this);
	showSimplifiedAction->setIcon(UiHelper::createIcon("compass"));
	showSimplifiedAction->setToolTip("Show the gradient pairs and critical points in their simplified form");
	showSimplifiedAction->setCheckable(true);
	connect(showSimplifiedAction, &QAction::triggered, map, &RiverWidget::setShowSimplified);
	connect(showSimplifiedAction, &QAction::triggered, this, &RiverGui::updateActions);
#endif

	showInputDcelAction = new QAction("Show &gradient pairs", this);
	showInputDcelAction->setIcon(UiHelper::createIcon("color-gradient"));
	showInputDcelAction->setToolTip("Show or hide the gradient pairs");
	showInputDcelAction->setCheckable(true);
	showInputDcelAction->setChecked(false);
	connect(showInputDcelAction, &QAction::toggled, map, &RiverWidget::setShowInputDcel);

	drawGradientPairsAsTreesAction = new QAction("As trees", this);
	drawGradientPairsAsTreesAction->setIcon(UiHelper::createIcon("color-gradient"));
	drawGradientPairsAsTreesAction->setToolTip("Draws the gradient pairs as trees instead of arrows");
	drawGradientPairsAsTreesAction->setCheckable(true);
	drawGradientPairsAsTreesAction->setChecked(false);
	connect(drawGradientPairsAsTreesAction, &QAction::toggled, map, &RiverWidget::setDrawGradientPairsAsTrees);

	showCriticalPointsAction = new QAction("Show &critical points", this);
	showCriticalPointsAction->setIcon(UiHelper::createIcon("color-gradient"));
	showCriticalPointsAction->setToolTip("Show or hide the critical points");
	showCriticalPointsAction->setCheckable(true);
	showCriticalPointsAction->setChecked(false);
	connect(showCriticalPointsAction, &QAction::toggled, map, &RiverWidget::setShowCriticalPoints);

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	showSpursAction = new QAction("Show spurs", this);
	showSpursAction->setIcon(UiHelper::createIcon("color-gradient"));
	showSpursAction->setToolTip("Show or hide the spurs used in the finger computation");
	showSpursAction->setCheckable(true);
	showSpursAction->setChecked(false);
	connect(showSpursAction, &QAction::toggled, map, &RiverWidget::setShowSpurs);
#endif

	showMsComplexAction = new QAction("Show &MS complex", this);
	showMsComplexAction->setIcon(UiHelper::createIcon("color-gradient"));
	showMsComplexAction->setToolTip("Show or hide the Morse-Smale complex");
	showMsComplexAction->setCheckable(true);
	showMsComplexAction->setChecked(false);
	connect(showMsComplexAction, &QAction::toggled, map, &RiverWidget::setShowMsComplex);

	showNetworkAction = new QAction("Show &network", this);
	showNetworkAction->setIcon(UiHelper::createIcon("color-gradient"));
	showNetworkAction->setToolTip("Show or hide the generated network");
	showNetworkAction->setCheckable(true);
	showNetworkAction->setChecked(true);
	connect(showNetworkAction, &QAction::toggled, map, &RiverWidget::setShowNetwork);

	msEdgesStraightAction = new QAction("&Straight MS edges", this);
	msEdgesStraightAction->setIcon(UiHelper::createIcon("color-gradient"));
	msEdgesStraightAction->setToolTip("Draw MS edges as straight lines");
	msEdgesStraightAction->setCheckable(true);
	msEdgesStraightAction->setChecked(false);
	connect(msEdgesStraightAction, &QAction::toggled, map, &RiverWidget::setMsEdgesStraight);

	showNetworkAction = new QAction("Show &network", this);
	showNetworkAction->setIcon(UiHelper::createIcon("color-gradient"));
	showNetworkAction->setToolTip("Show or hide the generated network");
	showNetworkAction->setCheckable(true);
	showNetworkAction->setChecked(true);
	connect(showNetworkAction, &QAction::toggled, map, &RiverWidget::setShowNetwork);

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	showFingersAction = new QAction("Show &fingers", this);
	showFingersAction->setIcon(UiHelper::createIcon("color-gradient"));
	showFingersAction->setToolTip("Show or hide the generated fingers");
	showFingersAction->setCheckable(true);
	showFingersAction->setChecked(true);
	connect(showFingersAction, &QAction::toggled, map, &RiverWidget::setShowFingers);
#endif

	editBoundaryAction = new QAction("&Edit boundary", this);
	editBoundaryAction->setIcon(UiHelper::createIcon("draw-freehand"));
	editBoundaryAction->setCheckable(true);
	editBoundaryAction->setToolTip("Delineate the part of the river to be analyzed, by drawing a boundary");
	connect(editBoundaryAction, &QAction::toggled, [&] {
		if (editBoundaryAction->isChecked()) {
			map->startBoundaryEditMode();
		} else {
			map->endBoundaryEditMode();
		}
		updateActions();
	});

	computeAction = new QAction("&Compute network", this);
	computeAction->setShortcut(QKeySequence("Ctrl+C"));
	computeAction->setIcon(UiHelper::createIcon("run-build"));
	computeAction->setToolTip("Start the computation");
	connect(computeAction, &QAction::triggered, this, [this]() { startComputation(false); });

	computeAllFramesAction = new QAction("&Compute for all frames", this);
	computeAllFramesAction->setIcon(UiHelper::createIcon("run-build-configure"));
	computeAllFramesAction->setToolTip("Start the computation for all frames");
	connect(computeAllFramesAction, &QAction::triggered, this, [this]() { startComputation(true); });

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	computeFingersAction = new QAction("&Compute fingers", this);
	computeFingersAction->setShortcut(QKeySequence("Ctrl+G"));
	computeFingersAction->setIcon(UiHelper::createIcon("run-build"));
	computeFingersAction->setToolTip("Computes and draws fingers");
	connect(computeFingersAction, &QAction::triggered,
	        this, &RiverGui::computeFingers);
#endif

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
	drawGradientPairsAsTreesAction->setEnabled(m_riverData != nullptr && activeFrame()->m_inputDcel != nullptr &&
	                                !map->boundaryEditMode());
	showCriticalPointsAction->setEnabled(
	    m_riverData != nullptr && activeFrame()->m_inputDcel != nullptr && !map->boundaryEditMode());
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	showSpursAction->setEnabled(m_riverData != nullptr && activeFrame()->m_inputDcel != nullptr &&
	                               !map->boundaryEditMode() && showSimplifiedAction->isChecked());
#endif
	showMsComplexAction->setEnabled(m_riverData != nullptr && activeFrame()->m_msComplex != nullptr &&
	                                !map->boundaryEditMode());
	showNetworkAction->setEnabled(m_riverData != nullptr && activeFrame()->m_networkGraph != nullptr &&
	                              !map->boundaryEditMode());
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	showFingersAction->setEnabled(m_riverData != nullptr && activeFrame()->m_fingers != nullptr);
#endif

	openAction->setEnabled(!map->boundaryEditMode());
	openTimeSeriesAction->setEnabled(!map->boundaryEditMode());
	openBoundaryAction->setEnabled(m_riverData != nullptr && !map->boundaryEditMode());
	saveBoundaryAction->setEnabled(m_riverData != nullptr && !map->boundaryEditMode());
	saveImageAction->setEnabled(m_riverData != nullptr && !map->boundaryEditMode());
	saveFrameAction->setEnabled(m_riverData != nullptr && !map->boundaryEditMode());
	generateBoundaryAction->setEnabled(m_riverData != nullptr && !map->boundaryEditMode());
	setSourceSinkAction->setEnabled(m_riverData != nullptr && !map->boundaryEditMode());
	editBoundaryAction->setEnabled(m_riverData != nullptr && (editBoundaryAction->isChecked() || !map->boundaryEditMode()));
	resetBoundaryAction->setEnabled(m_riverData != nullptr && !map->boundaryEditMode());

	saveGraphAction->setEnabled(m_riverData != nullptr && activeFrame()->m_networkGraph != nullptr);
	saveLinkSequenceAction->setEnabled(m_riverData != nullptr && activeFrame()->m_networkGraph != nullptr);
	showBackgroundAction->setEnabled(m_riverData != nullptr);
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	showSimplifiedAction->setEnabled(m_riverData != nullptr && activeFrame()->m_simplifiedInputDcel != nullptr);
#endif

	closeAction->setEnabled(m_riverData != nullptr && !map->boundaryEditMode());

	computeAction->setEnabled(m_riverData != nullptr && !m_computationRunning &&
	                          !map->boundaryEditMode());
	computeAllFramesAction->setEnabled(m_riverData != nullptr && !m_computationRunning &&
	                                   m_riverData->frameCount() > 1 && !map->boundaryEditMode());
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	computeFingersAction->setEnabled(m_riverData != nullptr && activeFrame()->m_inputDcel != nullptr);
#endif

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

	boundaryMenu->setEnabled(m_riverData != nullptr);
	exportMenu->setEnabled(m_riverData != nullptr);
	runMenu->setEnabled(m_riverData != nullptr);
	viewMenu->setEnabled(m_riverData != nullptr);
}

void RiverGui::createMenu() {
	fileMenu = menuBar()->addMenu("&File");
	openMenu = fileMenu->addMenu("Open DEM");
	openMenu->setIcon(UiHelper::createIcon("document-open"));
	openMenu->addAction(openAction);
	openMenu->addAction(openTimeSeriesAction);
	fileMenu->addAction(saveFrameAction);
	fileMenu->addSeparator();
	exportMenu = fileMenu->addMenu("Export");
	exportMenu->setIcon(UiHelper::createIcon("document-export"));
	exportMenu->addAction(saveImageAction);
	exportMenu->addAction(saveGraphAction);
	exportMenu->addAction(saveLinkSequenceAction);
	fileMenu->addSeparator();
	fileMenu->addAction(closeAction);
	fileMenu->addAction(quitAction);

	boundaryMenu = menuBar()->addMenu("&Boundary");
	boundaryMenu->addAction(generateBoundaryAction);
	boundaryMenu->addAction(setSourceSinkAction);
	boundaryMenu->addAction(editBoundaryAction);
	boundaryMenu->addAction(resetBoundaryAction);
	boundaryMenu->addSeparator();
	boundaryMenu->addAction(openBoundaryAction);
	boundaryMenu->addAction(saveBoundaryAction);

	runMenu = menuBar()->addMenu("&Run");
	runMenu->addAction(computeAction);
	runMenu->addAction(computeAllFramesAction);
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	runMenu->addAction(computeFingersAction);
#endif

	viewMenu = menuBar()->addMenu("&View");
	viewMenu->addAction(showBackgroundAction);
	viewMenu->addSeparator();
	viewMenu->addAction(zoomInAction);
	viewMenu->addAction(zoomOutAction);
	viewMenu->addAction(fitToViewAction);
	viewMenu->addSeparator();
	viewMenu->addAction(showInputDcelAction);
	viewMenu->addAction(drawGradientPairsAsTreesAction);
	viewMenu->addAction(showCriticalPointsAction);
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	viewMenu->addAction(showSimplifiedAction);
	viewMenu->addAction(showSpursAction);
#endif
	viewMenu->addAction(showMsComplexAction);
	viewMenu->addAction(showNetworkAction);
	viewMenu->addAction(showNetworkAction);
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	viewMenu->addAction(showFingersAction);
#endif

	helpMenu = menuBar()->addMenu("&Help");
	helpMenu->addAction(aboutAction);
	helpMenu->addAction(aboutQtAction);
}

void RiverGui::createToolBar() {
	toolBar = addToolBar("Main toolbar");

	QToolButton* computeButton = new QToolButton(toolBar);
	computeButton->setText("Compute network");
	computeButton->setIcon(UiHelper::createIcon("run-build"));
	computeButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
	computeButton->setPopupMode(QToolButton::ToolButtonPopupMode::DelayedPopup);
	computeButton->addAction(computeAction);
	computeButton->addAction(computeAllFramesAction);
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	computeButton->addAction(computeFingersAction);
#endif
	computeButton->setDefaultAction(computeAction);
	toolBar->addWidget(computeButton);
	toolBar->addSeparator();
	toolBar->addAction(editBoundaryAction);
	toolBar->addSeparator();
	toolBar->addAction(showInputDcelAction);
	toolBar->addAction(showCriticalPointsAction);
	toolBar->addAction(showMsComplexAction);
	toolBar->addAction(showNetworkAction);
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
	toolBar->addAction(showFingersAction);
#endif
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

	// try if the dropped file is a boundary file
	if (m_riverData != nullptr && fileNames.size() == 1) {
		QString error = "";
		Boundary boundary = BoundaryReader::readBoundary(fileNames[0], m_riverData->width(),
		                                                 m_riverData->height(), error);
		if (error == "") {
			m_riverData->setBoundary(boundary);
			map->update();
			return;
		}
	}

	// otherwise, try opening it as a DEM
	openFramesNamed(fileNames);
}

void RiverGui::openFrame() {
	QString fileName = QFileDialog::getOpenFileName(
	            this,
	            "Open DEM",
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
	            "Open time series",
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
		msgBox.setWindowTitle("Cannot open elevation data");
		msgBox.setText("<qt>The file cannot be opened.");
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
	mergeTreeDock->setMergeTree(nullptr);
	mergeTreeDock->setElevationRange(m_riverData->minimumElevation(), m_riverData->maximumElevation());
	mergeTreeDock->setMapSize(m_riverData->width(), m_riverData->height());
	unitsDock->setUnits(m_riverData->units());
	progressDock->reset();
	timeDock->setFrame(0);
	timeDock->setFrameCount(m_riverData->frameCount());
	updateActions();

	if (fileNames.length() == 1) {
		statusBar()->showMessage("Opened elevation data \"" + fileNames[0] + "\"", 5000);
	} else {
		statusBar()->showMessage(
		    QString("Opened time series consisting of %1 frames").arg(fileNames.length()), 5000);
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
		msgBox.setWindowTitle("Cannot open elevation data");
		msgBox.setText(QString("<qt>The file <code>%1</code> cannot be opened.").arg(fileName));
		msgBox.setInformativeText("<qt><p>This file does not seem to be a type of DEM "
		                          "that TopoTide supports. TopoTide supports, among others, "
		                          "GeoTIFF and ESRI grid files.</p>");
		msgBox.setDetailedText("Reading the file failed due to the "
		                       "following error:\n    " +
		                       error);
		msgBox.exec();
		return nullptr;
	}

	return std::make_shared<RiverFrame>(fileName, heightMap);
}

void RiverGui::saveFrame() {

	QString fileName = QFileDialog::getSaveFileName(this,
	        "Save DEM",
	        ".",
	        "ESRI grid files (*.ascii)");
	if (fileName == nullptr) {
		return;
	}

	EsriGridWriter::writeGridFile(activeFrame()->m_heightMap, fileName, m_riverData->units());
}

void RiverGui::resetBoundary() {
	if (m_computationRunning) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot reset boundary");
		msgBox.setText("<qt>The boundary cannot be reset.");
		msgBox.setInformativeText("<qt>There is still a running computation. "
		                          "Wait until the computation is finished, and "
		                          "try again.");
		msgBox.exec();
		return;
	}

	m_riverData->setBoundary(Boundary{m_riverData->width(), m_riverData->height()});
	map->update();
}

void RiverGui::openBoundary() {
	QString fileName = QFileDialog::getOpenFileName(
	            this,
	            "Open boundary",
	            ".",
	            "Boundary text files (*.txt)");
	if (fileName == nullptr) {
		return;
	}

	if (m_computationRunning) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot open boundary");
		msgBox.setText("<qt>The boundary cannot be opened.");
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
		msgBox.setText(QString("<qt>The boundary <code>%1</code> cannot be opened.").arg(fileName));
		msgBox.setInformativeText("<qt><p>This file does not seem to be "
		                          "a valid text file containing a boundary.</p>");
		msgBox.setDetailedText("Reading the text file failed due to the "
		                       "following error:\n    " +
		                       error);
		msgBox.exec();
		return;
	}

	m_riverData->setBoundary(boundary);
	map->update();
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

void RiverGui::saveImage() {

	QString fileName = QFileDialog::getSaveFileName(this,
	        "Save DEM as image",
	        ".",
	        "Images (*.png)");
	if (fileName == nullptr) {
		return;
	}

	QImage background = map->grabFramebuffer();
	background.save(fileName);

	statusBar()->showMessage("Saved image \"" + fileName + "\"", 5000);
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

void RiverGui::startComputation(bool allFrames) {
	if (m_computationRunning) {
		qDebug() << "Tried to start computation while another was still running";
		return;
	}
	
	progressDock->reset();
	m_computationRunning = true;

	map->update();
	updateActions();

	auto* thread = allFrames ? new BackgroundThread(m_riverData)
	                         : new BackgroundThread(m_riverData, activeFrame());

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
		mergeTreeDock->setMergeTree(activeFrame()->m_mergeTree);
		updateActions();
	});
	
	connect(thread, &BackgroundThread::errorEncountered, this, [this](QString error) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Error encountered during computation");
		msgBox.setText("<qt>" + error);
		msgBox.exec();
	});

	connect(thread, &QThread::finished, thread, &QThread::deleteLater);
	connect(thread, &QThread::finished, this, [&] {
		m_computationRunning = false;
		map->update();
		updateActions();
		setCursor(Qt::ArrowCursor);
	});

	thread->start();
	setCursor(Qt::BusyCursor);
}

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
void RiverGui::computeFingers() {
	const std::shared_ptr<RiverFrame>& frame = activeFrame();
	QWriteLocker lock1(&(frame->m_inputDcelLock));
	QWriteLocker lock2(&(frame->m_msComplexLock));
	QWriteLocker lock3(&(frame->m_simplifiedInputDcelLock));
	frame->m_simplifiedInputDcel = std::make_shared<InputDcel>(*frame->m_inputDcel);

	GradientFieldSimplifier simplifier(frame->m_simplifiedInputDcel, frame->m_msComplex,
	                                   settingsDock->msThreshold());
	simplifier.simplify();

	FingerFinder finder(frame->m_simplifiedInputDcel, frame->m_msComplex,
	                    settingsDock->msThreshold());
	frame->m_fingers = std::make_shared<std::vector<InputDcel::Path>>(finder.findFingers());
	map->update();
	updateActions();
}
#endif

std::shared_ptr<RiverFrame> RiverGui::activeFrame() {
	return m_riverData->getFrame(m_frame);
}

void RiverGui::closeFrame() {
	if (m_computationRunning) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot close DEM");
		msgBox.setText("<qt>The DEM cannot be closed.");
		msgBox.setInformativeText("<qt>There is still a running computation. "
		                          "Wait until the computation is finished, and "
		                          "try again.");
		msgBox.exec();
		return;
	}

	m_riverData = nullptr;
	map->setRiverData(nullptr);
	map->setRiverFrame(nullptr);
	map->update();
	updateActions();
}
