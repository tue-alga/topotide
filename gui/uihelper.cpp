#include "uihelper.h"

#include <QPainter>
#include <QSvgRenderer>

QPixmap generatePixmap(const QString& name, int size, int devicePixelRatio) {
	QPixmap pixmap(QSize{size * devicePixelRatio, size * devicePixelRatio});
	pixmap.fill(Qt::transparent);
	QSvgRenderer renderer(QString{":/res/icons/%1/%2.svg"}.arg(size).arg(name));
	QPainter painter(&pixmap);
	renderer.render(&painter, pixmap.rect());
	pixmap.setDevicePixelRatio(devicePixelRatio);
	return pixmap;
}

QIcon UiHelper::createIcon(const QString& name) {
	QIcon icon;
	icon.addPixmap(generatePixmap(name, 16, 1));
	icon.addPixmap(generatePixmap(name, 16, 2));
	icon.addPixmap(generatePixmap(name, 22, 1));
	icon.addPixmap(generatePixmap(name, 22, 2));
	icon.addPixmap(generatePixmap(name, 24, 1));
	icon.addPixmap(generatePixmap(name, 24, 2));
	return icon;
}
