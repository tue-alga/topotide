#include "coordinatelabel.h"

#include "unitshelper.h"

CoordinateLabel::CoordinateLabel(QWidget *parent) : QLabel(parent) {
	setText("");
}

void CoordinateLabel::setCoordinate(int x, int y, int height) {
	m_showing = true;
	m_x = x;
	m_y = y;
	m_height = height;
	updateLabel();
}

void CoordinateLabel::clear() {
	m_showing = false;
	updateLabel();
}

void CoordinateLabel::setUnits(Units units) {
	m_units = units;
	updateLabel();
}

void CoordinateLabel::updateLabel() {
	if (m_showing) {
		setText(QString("<p>x: <b>%1</b>, y: <b>%2</b>, "
		                "height: <b>%3</b> <code>(0x%4)</code>")
		        .arg(m_x).arg(m_y)
		        .arg(UnitsHelper::formatElevation(
		                 m_units.toRealElevation(m_height)))
		        .arg(m_height, 6, 16, QChar('0')));
	} else {
		setText("");
	}
}
