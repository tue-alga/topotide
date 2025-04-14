#include <QApplication>
#include <QCoreApplication>

#ifdef WITH_KCRASH
#include <KCrash>
#endif

#include "rivercli.h"
#include "rivergui.h"

/**
 * The main method of TopoTide. This launches the GUI if there are no
 * command-line arguments, or the batch mode if there are arguments.
 *
 * \param argc The number of arguments.
 * \param argv The array of arguments.
 * \return The exit code.
 */
int main(int argc, char* argv[]) {

	const QString APP_VERSION = "2.1.0";

	if (argc <= 1) {
		QApplication a(argc, argv);
		a.setApplicationName("TopoTide");
		a.setApplicationVersion(APP_VERSION);

#ifdef WITH_KCRASH
		KCrash::setDrKonqiEnabled(true);
#endif

		RiverGui mainWindow;
		mainWindow.show();
		return a.exec();

	} else {
		QCoreApplication a(argc, argv);
		a.setApplicationName("TopoTide");
		a.setApplicationVersion(APP_VERSION);
		return RiverCli::runComputation(a.arguments());
	}
}
