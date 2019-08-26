#ifndef RIVERGUI_H
#define RIVERGUI_H

#include <QAction>
#include <QDial>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QSlider>
#include <QToolBar>
#include <QWidget>

#include "../units.h"
#include "backgrounddock.h"
#include "coordinatelabel.h"
#include "networkdock.h"
#include "progressdock.h"
#include "riverdata.h"
#include "riverwidget.h"
#include "settingsdock.h"
#include "striationdock.h"
#include "unitsdock.h"

/**
 * The main window of the GUI of the river application.
 */
class RiverGui : public QMainWindow {

	Q_OBJECT

	public:

		/**
		 * Constructs the window.
		 */
		RiverGui();

	public slots:

		/**
		 * Shows an open dialog for the user to select a river image, and opens
		 * it.
		 */
		void openFrame();

		/**
		 * Opens a river image with the given file name.
		 * @param fileName The name of the file to open.
		 */
		void openFrameNamed(const QString& fileName);

		void openBoundary();

		void saveIpeImage();
		void saveGraph();
		void saveLinkSequence();

		/**
		 * Shows the about box.
		 */
		void about();

	signals:
		void unitsChanged(Units& units);

	protected:
		void dragEnterEvent(QDragEnterEvent *event) override;
		void dropEvent(QDropEvent *event) override;

	private:

		/**
		 * Constructs the main GUI elements.
		 */
		void createGui();

		/**
		 * Constructs the actions (QAction) for the GUI.
		 */
		void createActions();

		/**
		 * Updates the enabled / disabled state of all actions.
		 */
		void updateActions();

		/**
		 * Creates the menu bar and adds it to the GUI.
		 */
		void createMenu();

		/**
		 * Creates the tool bar and adds it to the GUI.
		 */
		void createToolBar();

		/**
		 * Initializes the window such that it accepts dropped river images and
		 * opens them.
		 */
		void initializeDropping();

		/**
		 * Indicates that a re-computation is needed.
		 *
		 * \param onlyNetwork If only the network should be recomputed (see
		 * RiverData::startComputation() for details).
		 */
		void markComputationNeeded(bool onlyNetwork);

		bool m_computationNeeded = false;
		bool m_onlyNetworkNeeded = false;

		/**
		 * Starts the computation by calling `startComputation()` on the
		 * RiverData with the parameters set in the GUI.
		 *
		 * The computation is run only if needed (see `markComputationNeeded()`)
		 * and if indicated there, only the network is recomputed.
		 */
		void startComputation();

		/**
		 * The river currently open in the program.
		 */
		RiverData riverData;

		QMenu* fileMenu;
		QMenu* runMenu;
		QMenu* viewMenu;
		QMenu* toolsMenu;
		QMenu* helpMenu;

		QToolBar* toolBar;

		RiverWidget* map;

		BackgroundDock* backgroundDock;
		SettingsDock* settingsDock;
		UnitsDock* unitsDock;
		ProgressDock* progressDock;

		StriationDock* striationDock;
		NetworkDock* networkDock;

		CoordinateLabel* coordinateLabel;

		// actions

		QAction* openAction;
		QAction* openBoundaryAction;
		QAction* saveIpeAction;
		QAction* saveGraphAction;
		QAction* saveLinkSequenceAction;
		QAction* quitAction;
		QAction* showMapAction;
		QAction* showWaterPlaneAction;
		QAction* showOutlinesAction;
		QAction* showShadingAction;
		QAction* showInputDcelAction;
		QAction* showMsComplexAction;
		QAction* msEdgesStraightAction;
		QAction* showStriationAction;
		QAction* showNetworkAction;
		QAction* computeAction;
		QAction* zoomInAction;
		QAction* zoomOutAction;
		QAction* fitToViewAction;
		QAction* aboutAction;
		QAction* aboutQtAction;
};

#endif /* RIVERGUI_H */
