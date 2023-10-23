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

#include <iostream>

#include "../boundaryreader.h"
#include "../boundarywriter.h"
#include "../linksequence.h"
#include "../textfilereader.h"

#include "backgroundthread.h"
#include "graphwriter.h"
#include "ipewriter.h"
#include "linksequencewriter.h"
#include "uihelper.h"
#include "unitsdock.h"

RiverGui::RiverGui() {

	createGui();

	createActions();
	//updateActions();

	createMenu();
	createToolBar();

	initializeDropping();

	resize(1280, 720);
	setWindowTitle("TTGA");

	updateActions();
}

void RiverGui::createGui() {

	// river map
	map = new RiverWidget(&riverData);
	connect(this, &RiverGui::unitsChanged,
	        map, &RiverWidget::setUnits);
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
	connect(this, &RiverGui::unitsChanged,
	        backgroundDock, &BackgroundDock::setUnits);

	// units dock
	unitsDock = new UnitsDock(riverData.units, this);
	connect(unitsDock, &UnitsDock::unitsChanged,
	        [this](Units units) {
		bool needToRecompute =
		        riverData.units.m_xResolution != units.m_xResolution ||
		        riverData.units.m_yResolution != units.m_yResolution;
		riverData.units = units;
		emit unitsChanged(riverData.units);
		if (needToRecompute) {
			markComputationNeeded(false);
		}
	});
	addDockWidget(Qt::TopDockWidgetArea, unitsDock);

	// settings dock
	settingsDock = new SettingsDock(this);
	connect(settingsDock, &SettingsDock::striationStrategyChanged, [&] {
		markComputationNeeded(false);
	});
	connect(settingsDock, &SettingsDock::deltaChanged, [&] {
		markComputationNeeded(true);
	});
	connect(settingsDock, &SettingsDock::sandFunctionChanged, [&] {
		markComputationNeeded(true);
	});
	connect(settingsDock, &SettingsDock::bidirectionalChanged, [&] {
		markComputationNeeded(true);
	});
	connect(settingsDock, &SettingsDock::simplifyChanged, [&] {
		markComputationNeeded(true);
	});
	connect(settingsDock, &SettingsDock::msSimplifyChanged, [&] {
		markComputationNeeded(false);
	});
	connect(settingsDock, &SettingsDock::msThresholdChanged, [&] {
		map->setNetworkDelta(settingsDock->msThreshold());
	});
	map->setNetworkDelta(settingsDock->msThreshold());
	connect(this, &RiverGui::unitsChanged,
	        settingsDock, &SettingsDock::setUnits);
	addDockWidget(Qt::TopDockWidgetArea, settingsDock);

	// progress viewer
	progressDock = new ProgressDock(this);
	addDockWidget(Qt::TopDockWidgetArea, progressDock);

	// striation viewer
	striationDock = new StriationDock(this);
	striationDock->hide();
	connect(striationDock, &StriationDock::itemChanged,
	        map, &RiverWidget::setStriationItem);
	addDockWidget(Qt::BottomDockWidgetArea, striationDock);

	// network viewer
	networkDock = new NetworkDock(this);
	networkDock->hide();
	connect(networkDock, &NetworkDock::pathChanged,
	        map, &RiverWidget::setNetworkPath);
	addDockWidget(Qt::BottomDockWidgetArea, networkDock);

	// status bar
	coordinateLabel = new CoordinateLabel(this);
	statusBar()->addPermanentWidget(coordinateLabel);
	connect(map, &RiverWidget::hoveredCoordinateChanged,
	        coordinateLabel, &CoordinateLabel::setCoordinate);
	connect(map, &RiverWidget::mouseLeft,
	        coordinateLabel, &CoordinateLabel::clear);
	connect(this, &RiverGui::unitsChanged,
	        coordinateLabel, &CoordinateLabel::setUnits);

	// connect docks to the computation
	connect(&riverData, &RiverData::computationStarted, [&] {
		updateActions();
	});
	connect(&riverData, &RiverData::dataChanged, [&] {
		map->update();
		if (showStriationAction->isChecked()) {
			striationDock->setStriation(riverData.striation);
		} else {
			striationDock->setStriation(nullptr);
		}
		if (showNetworkAction->isChecked()) {
			networkDock->setNetwork(riverData.network);
		} else {
			networkDock->setNetwork(nullptr);
		}
		updateActions();
	});
	connect(&riverData, &RiverData::computationFinished, [&] {
		updateActions();
		setCursor(Qt::ArrowCursor);
	});
}

void RiverGui::createActions() {

	openAction = new QAction("&Open DEM...", this);
	openAction->setShortcuts(QKeySequence::Open);
	openAction->setIcon(UiHelper::createIcon("document-open"));
	openAction->setToolTip("Open a river image");
	connect(openAction, &QAction::triggered, this, &RiverGui::openFrame);

	openBoundaryAction = new QAction("&Open boundary...", this);
	openBoundaryAction->setIcon(UiHelper::createIcon("document-open"));
	openBoundaryAction->setToolTip("Open a river boundary");
	connect(openBoundaryAction, &QAction::triggered, this, &RiverGui::openBoundary);

	saveIpeAction = new QAction("Save Ipe image...", this);
	saveIpeAction->setShortcuts(QKeySequence::Save);
	saveIpeAction->setIcon(UiHelper::createIcon("document-save"));
	saveIpeAction->setToolTip("Save the computed network as an Ipe image");
	connect(saveIpeAction, &QAction::triggered, this, &RiverGui::saveIpeImage);

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

	showStriationAction = new QAction("Show &striation", this);
	showStriationAction->setIcon(UiHelper::createIcon("color-gradient"));
	showStriationAction->setToolTip("Show or hide the striation");
	showStriationAction->setCheckable(true);
	showStriationAction->setChecked(false);
	connect(showStriationAction, &QAction::toggled, map, &RiverWidget::setShowStriation);
	connect(showStriationAction, &QAction::toggled, [&] {
		if (showStriationAction->isChecked()) {
			striationDock->setStriation(riverData.striation);
		} else {
			striationDock->setStriation(nullptr);
		}
	});

	showNetworkAction = new QAction("Show &network", this);
	showNetworkAction->setIcon(UiHelper::createIcon("color-gradient"));
	showNetworkAction->setToolTip("Show or hide the representative network");
	showNetworkAction->setCheckable(true);
	showNetworkAction->setChecked(true);
	connect(showNetworkAction, &QAction::toggled, map, &RiverWidget::setShowNetwork);
	connect(showNetworkAction, &QAction::toggled, [&] {
		if (showNetworkAction->isChecked()) {
			networkDock->setNetwork(riverData.network);
		} else {
			networkDock->setNetwork(nullptr);
		}
	});

	editBoundaryAction = new QAction("&Edit boundary", this);
	editBoundaryAction->setIcon(UiHelper::createIcon("draw-freehand"));
	editBoundaryAction->setCheckable(true);
	editBoundaryAction->setToolTip("Delineate the part of the river to be analyzed, by drawing a boundary");
	connect(editBoundaryAction, &QAction::toggled, [&] {
		if (editBoundaryAction->isChecked()) {
			map->startBoundaryEditMode();
		} else {
			riverData.boundaryRasterized = riverData.boundary.rasterize();
			map->endBoundaryEditMode();
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

	aboutAction = new QAction("&About TTGA...", this);
	aboutAction->setToolTip("Show some information about this application");
	connect(aboutAction, &QAction::triggered, this, &RiverGui::about);

	aboutQtAction = new QAction("About &Qt...", this);
	aboutQtAction->setToolTip("Show the Qt library's About box");
	connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void RiverGui::updateActions() {
	if (riverData.isInitialized()) {
		setWindowTitle(riverData.name + " — TTGA");
	} else {
		setWindowTitle("TTGA");
	}

	showInputDcelAction->setEnabled(riverData.isInitialized() &&
	                                riverData.inputDcel != nullptr &&
	                                !map->boundaryEditMode());
	showMsComplexAction->setEnabled(riverData.isInitialized() &&
	                                riverData.msComplex != nullptr &&
	                                !map->boundaryEditMode());
	showStriationAction->setEnabled(riverData.isInitialized() &&
	                                riverData.striation != nullptr &&
	                                !map->boundaryEditMode());
	showNetworkAction->setEnabled(riverData.isInitialized() &&
	                              riverData.networkGraph != nullptr &&
	                                !map->boundaryEditMode());

	openBoundaryAction->setEnabled(riverData.isInitialized() &&
	                                !map->boundaryEditMode());
	saveBoundaryAction->setEnabled(riverData.isInitialized() &&
	                                !map->boundaryEditMode());
	editBoundaryAction->setEnabled(riverData.isInitialized());

	saveIpeAction->setEnabled(riverData.isInitialized() &&
	                          riverData.networkGraph != nullptr);
	saveGraphAction->setEnabled(riverData.isInitialized() &&
	                            riverData.networkGraph != nullptr);
	saveLinkSequenceAction->setEnabled(riverData.isInitialized() &&
	                            riverData.networkGraph != nullptr);
	showBackgroundAction->setEnabled(riverData.isInitialized());

	computeAction->setEnabled(riverData.isInitialized() &&
	                          m_computationNeeded &&
	                          !riverData.isThreadRunning() &&
	                          !map->boundaryEditMode());

	zoomInAction->setEnabled(riverData.isInitialized());
	zoomOutAction->setEnabled(riverData.isInitialized());
	fitToViewAction->setEnabled(riverData.isInitialized());

	backgroundDock->setVisible(riverData.isInitialized());
	backgroundDock->setEnabled(riverData.isInitialized());

	settingsDock->setVisible(riverData.isInitialized());
	settingsDock->setEnabled(riverData.isInitialized() &&
	                         !riverData.isThreadRunning());

	unitsDock->setVisible(riverData.isInitialized());
	unitsDock->setEnabled(riverData.isInitialized() &&
	                      !riverData.isThreadRunning());

	progressDock->setVisible(riverData.isInitialized());
}

void RiverGui::createMenu() {
	fileMenu = menuBar()->addMenu("&File");
	fileMenu->addAction(openAction);
	fileMenu->addAction(saveIpeAction);
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
	viewMenu->addAction(showNetworkAction);
	QMenu* intermediateMenu = new QMenu("Intermediate data structures",
	                                    viewMenu);
	intermediateMenu->addAction(showInputDcelAction);
	intermediateMenu->addAction(showMsComplexAction);
	intermediateMenu->addAction(showStriationAction);
	viewMenu->addMenu(intermediateMenu);

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
	if (urls.size() > 1) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot open river");
		msgBox.setText(QString("You dragged %1 images.").arg(urls.size()));
		msgBox.setInformativeText("Please drop exactly one river image for the "
		                          "program to open.");
		msgBox.exec();
		return;
	}
	QUrl url = urls[0];
	openFrameNamed(url.toLocalFile());
}

void RiverGui::openFrame() {
	QString fileName = QFileDialog::getOpenFileName(
	            this,
	            "Open river frame",
	            ".",
	            "Image files (*.png *.jpg *.bmp);;"
	            "River text files (*.txt)");
	if (fileName == nullptr) {
		return;
	}

	openFrameNamed(fileName);
}

void RiverGui::openFrameNamed(const QString& fileName) {
	if (riverData.isThreadRunning()) {
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

	// get all locks to be sure
	QWriteLocker lock1(&riverData.inputGraphLock);
	QWriteLocker lock2(&riverData.inputDcelLock);
	QWriteLocker lock3(&riverData.msComplexLock);
	QWriteLocker lock4(&riverData.striationLock);
	QWriteLocker lock5(&riverData.networkLock);

	riverData.deleteEverything(false);

	// is it an image?
	QString error = "[no error given]";
	riverData.image = QImage(fileName);
	riverData.name = fileName;
	if (!riverData.isInitialized()) {
		// is it a text file?
		riverData.image = TextFileReader::readTextFile(
		                      fileName, error, riverData.units);
		unitsDock->setUnits(riverData.units);
	}
	riverData.heightMap = HeightMap(riverData.image);
	updateActions();

	if (!riverData.isInitialized()) {
		// something went wrong
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot open river");
		msgBox.setText(QString("<qt>The river <code>%1</code> cannot be opened.").arg(fileName));
		msgBox.setInformativeText("<qt><p>This file does not seem to be an image file "
		                          "or a valid text file containing elevation "
		                          "data.</p>");
		msgBox.setDetailedText("Reading the text file failed due to the "
		                       "following error:\n    " + error);
		msgBox.exec();
		return;
	}

	Boundary boundary(riverData.heightMap);
	riverData.setBoundary(boundary);

	emit unitsChanged(riverData.units);

	map->updateTexture();
	map->resetTransform();
	progressDock->reset();
	markComputationNeeded(false);

	statusBar()->showMessage("Opened river \"" + fileName + "\"", 5000);
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

	if (riverData.isThreadRunning()) {
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
	        BoundaryReader::readBoundary(fileName, riverData.heightMap, error);

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

	riverData.setBoundary(boundary);
	markComputationNeeded(false);
}

void RiverGui::saveIpeImage() {
#ifndef WITH_IPELIB
	QMessageBox::critical(
	            nullptr, "Ipe support unavailable",
	            "As this program has not been compiled with Ipelib, support "
	            "for writing Ipe files is unavailable.");
#else
	QString fileName = QFileDialog::getSaveFileName(this,
	        "Save Ipe file",
	        ".",
	        "Ipe files (*.ipe)");
	if (fileName == nullptr) {
		return;
	}

	if (riverData.isThreadRunning()) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Cannot write Ipe image");
		msgBox.setText("<qt>The Ipe image cannot be saved.");
		msgBox.setInformativeText("<qt>There is still a running computation."
		                          "Wait until the computation is finished, and "
		                          "try again.");
		msgBox.exec();
		return;
	}

	QReadLocker lock1(&riverData.msComplexLock);
	QReadLocker lock2(&riverData.networkLock);
	QReadLocker lock3(&riverData.networkGraphLock);

	NetworkGraph graph = *riverData.networkGraph;
	graph.filterOnDelta(settingsDock->msThreshold());

	IpeWriter::writeIpeFile(riverData.heightMap, *riverData.networkGraph,
	                        riverData.units, fileName);

	statusBar()->showMessage("Saved Ipe file as \"" + fileName + "\"", 5000);
#endif
}

void RiverGui::saveGraph() {

	QString fileName = QFileDialog::getSaveFileName(this,
	        "Save graph",
	        ".",
	        "Text files (*.txt)");
	if (fileName == nullptr) {
		return;
	}

	if (riverData.isThreadRunning()) {
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

	QReadLocker lock(&riverData.networkGraphLock);

	NetworkGraph graph = *riverData.networkGraph;
	graph.filterOnDelta(settingsDock->msThreshold());
	GraphWriter::writeGraph(graph, riverData.units, fileName);

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

	if (riverData.isThreadRunning()) {
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

	QReadLocker lock(&riverData.networkGraphLock);

	LinkSequence links(*riverData.networkGraph);
	LinkSequenceWriter::writeLinkSequence(links, riverData.units, fileName);

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

	BoundaryWriter::writeBoundary(riverData.boundary, fileName);

	statusBar()->showMessage("Saved boundary as \"" +
	                         fileName + "\"", 5000);
}

void RiverGui::about() {
	QMessageBox msgBox;
	msgBox.setWindowTitle("About TTGA");
	msgBox.setText("<qt><b>TTGA " +
	               QApplication::applicationVersion() + " &ndash; "
	               "Topological Tools for Geomorphological Analysis</b>");
	msgBox.setInformativeText(
	            QString("<qt><p>TTGA was written by Tim Ophelders, "
	                    "Willem Sonke and Kevin Verbeek.</p>"
	                    "<p>This program is free software: you can redistribute it and/or modify "
	                    "it under the terms of the GNU General Public License as published by "
	                    "the Free Software Foundation, either version 3 of the License, or "
	                    "(at your option) any later version.</p>"
	                    "<p>It was compiled with the following options:"
	                    "<ul>"
#ifdef WITH_IPELIB
	                    "<li>Ipe export support "
	                    "(<code>WITH_IPELIB</code>)</li>"
#endif
#ifdef WITH_KCRASH
	                    "<li>support for KCrash / Dr. Konqi "
	                    "(<code>WITH_KCRASH</code>)</li>"
#endif
	                    "</ul></p>"
	                    "<p>This program uses "
	                    "<a href='https://github.com/KDE/breeze-icons'>Breeze "
	                    "icons</a>, "
	                    "licensed under the LGPL.</p>"));
	msgBox.setIcon(QMessageBox::Information);
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
		return;
	}
	if (!riverData.boundaryRasterized.isValid()) {
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
	riverData.startComputation(progressDock, statusBar(),
	                           m_onlyNetworkNeeded,
	                           settingsDock->sandFunction(),
	                           settingsDock->bidirectional(),
	                           settingsDock->simplify(),
	                           settingsDock->striationStrategy(),
	                           pow(10, settingsDock->delta() / 10.0),
	                           settingsDock->msSimplify() ? pow(10, settingsDock->msThreshold() / 10.0) : 0);
	m_computationNeeded = false;

	setCursor(Qt::BusyCursor);
}
