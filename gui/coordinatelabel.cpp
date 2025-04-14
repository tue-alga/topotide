#include "coordinatelabel.h"

#include "unitshelper.h"

CoordinateLabel::CoordinateLabel(QWidget *parent) : QLabel(parent) {
	setText("");
}

void CoordinateLabel::setCoordinateAndHeight(HeightMap::Coordinate coord, double height) {
	m_coord = coord;
	m_height = height;
	updateLabel();
}

void CoordinateLabel::setHeight(double height) {
	m_coord = std::nullopt;
	m_height = height;
	updateLabel();
}

void CoordinateLabel::clear() {
	m_coord = std::nullopt;
	m_height = std::nullopt;
	updateLabel();
}

void CoordinateLabel::setUnits(Units units) {
	m_units = units;
	updateLabel();
}

void CoordinateLabel::updateLabel() {
	if (m_coord && m_height) {
		setText(QString("<p>x: <b>%1</b>, y: <b>%2</b>, "
		                "height: <b>%3</b>")
		        .arg(m_coord->m_x).arg(m_coord->m_y)
		        .arg(UnitsHelper::formatElevation(*m_height)));
	} else if (m_height) {
		setText(QString("height: <b>%1</b>")
		        .arg(UnitsHelper::formatElevation(*m_height)));
	} else {
		setText("");
	}
}
