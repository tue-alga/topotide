#ifndef COORDINATELABEL_H
#define COORDINATELABEL_H

#include <QLabel>
#include <QWidget>

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
		 * Sets the coordinate that is shown in this label.
		 *
		 * \param x The x-coordinate.
		 * \param y The y-coordinate.
		 * \param height The height value.
		 */
		void setCoordinate(int x, int y, double height);

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
		bool m_showing = false;
		int m_x;
		int m_y;
		double m_height;

		void updateLabel();
};

#endif // COORDINATELABEL_H
