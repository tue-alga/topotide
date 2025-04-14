#ifndef COORDINATELABEL_H
#define COORDINATELABEL_H

#include <QLabel>
#include <QWidget>

#include <optional>

#include "heightmap.h"
#include "units.h"

/**
 * A label that shows the currently hovered coordinate.
 */
class CoordinateLabel : public QLabel {

	Q_OBJECT

	public:

		/**
		 * Creates a new coordinate label.
		 * \param parent The parent of this widget.
		 */
		CoordinateLabel(QWidget *parent = 0);

	public slots:

		/**
		 * Sets the coordinate and height that is shown in this label.
		 *
		 * \param coord The x- and y-coordinates.
		 * \param height The height value.
		 */
		void setCoordinateAndHeight(HeightMap::Coordinate coord, double height);

		/**
		 * Sets the height that is shown in this label, and clears the
		 * coordinate.
		 *
		 * \param height The height value.
		 */
		void setHeight(double height);

		/**
		 * Sets the unit convertor used for displaying the coordinates in this
		 * label.
		 *
		 * \param units The new units.
		 */
		void setUnits(Units units);

		/**
		 * Clears the coordinate label.
		 */
		void clear();

	private:
		Units m_units;
		std::optional<HeightMap::Coordinate> m_coord;
		std::optional<double> m_height;

		void updateLabel();
};

#endif // COORDINATELABEL_H
