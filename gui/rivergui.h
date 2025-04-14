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
#include <QStringList>
#include <QToolBar>
#include <QWidget>

#include <memory>

#include "backgrounddock.h"
#include "coordinatelabel.h"
#include "mergetreedock.h"
#include "progressdock.h"
#include "riverdata.h"
#include "riverwidget.h"
#include "settingsdock.h"
#include "timedock.h"
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
		void openFrames();
		void saveFrame();

		/**
		 * Opens river data with the given file names.
		 */
		void openFramesNamed(QStringList& fileNames);
		std::shared_ptr<RiverFrame> loadFrame(const QString& fileName, Units& units);

		void resetBoundary();
		void openBoundary();
		void saveBoundary();
		void saveImage();

		void saveGraph();
		void saveLinkSequence();

		void closeFrame();

		/**
		 * Shows the about box.
		 */
		void about();

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
		 * Whether the background thread is currently running.
		 */
		bool m_computationRunning = false;

		/**
		 * Starts the background thread to compute the river data.
		 *
		 * \param allFrames If `true`, instructs the background thread to run
		 * the computation for all frames instead of just the active frame.
		 */
		void startComputation(bool allFrames);

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
		void computeFingers();
#endif

		/// The river currently open in the program.
		std::shared_ptr<RiverData> m_riverData;
		/// Index of the frame of \ref m_riverData currently being viewed.
		int m_frame;

		/// Returns the currently shown frame.
		std::shared_ptr<RiverFrame> activeFrame();

		QMenu* fileMenu;
		QMenu* openMenu;
		QMenu* exportMenu;
		QMenu* runMenu;
		QMenu* boundaryMenu;
		QMenu* viewMenu;
		QMenu* toolsMenu;
		QMenu* helpMenu;

		QToolBar* toolBar;

		RiverWidget* map;

		BackgroundDock* backgroundDock;
		SettingsDock* settingsDock;
		UnitsDock* unitsDock;
		ProgressDock* progressDock;
		TimeDock* timeDock;
		MergeTreeDock* mergeTreeDock;

		CoordinateLabel* coordinateLabel;

		// actions

		QAction* openAction;
		QAction* openTimeSeriesAction;
		QAction* saveFrameAction;
		QAction* openBoundaryAction;
		QAction* saveGraphAction;
		QAction* saveLinkSequenceAction;
		QAction* saveBoundaryAction;
		QAction* saveImageAction;
		QAction* closeAction;
		QAction* quitAction;
		QAction* generateBoundaryAction;
		QAction* setSourceSinkAction;
		QAction* resetBoundaryAction;
		QAction* showBackgroundAction;
		QAction* showInputDcelAction;
		QAction* drawGradientPairsAsTreesAction;
		QAction* showCriticalPointsAction;
		QAction* showMsComplexAction;
		QAction* showNetworkAction;
		QAction* msEdgesStraightAction;
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
		QAction* computeFingersAction;
		QAction* showSimplifiedAction;
		QAction* showSpursAction;
		QAction* showFingersAction;
#endif
		QAction* editBoundaryAction;
		QAction* computeAction;
		QAction* computeAllFramesAction;
		QAction* zoomInAction;
		QAction* zoomOutAction;
		QAction* fitToViewAction;
		QAction* aboutAction;
		QAction* aboutQtAction;
};

#endif /* RIVERGUI_H */
