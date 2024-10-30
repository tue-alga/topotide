#ifndef UIHELPER_H
#define UIHELPER_H

#include <QIcon>
#include <QString>

/**
 * Class containing some helper functions for building the user interface.
 */
class UiHelper {

	public:

		/**
		 * Creates an icon, either by taking it from the system theme, or if
		 * that is not supported, by taking it from the resources directory.
		 *
		 * \param name The icon name (without `.svg` appended).
		 * \return The icon.
		 */
		static QIcon createIcon(const QString& name);
};

#endif // UIHELPER_H
