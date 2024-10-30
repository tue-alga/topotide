#include "uihelper.h"

QIcon UiHelper::createIcon(const QString& name) {
	return QIcon::fromTheme(name, QIcon(":res/icons/" + name + ".svg"));
}
