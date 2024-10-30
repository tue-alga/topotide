#include "point.h"

Point::Point() : x(0), y(0), h(0) {
}

Point::Point(double x, double y, double h) : x(x), y(y), h(h) {
}

double Point::distanceTo(Point p) {
	double dx = p.x - x;
	double dy = p.y - y;
	return sqrt(dx * dx + dy * dy);
}

bool Point::compareNeighbors(Point p1, Point p2) {
	double h1 = p1.h - h;
	double h2 = p2.h - h;
	double l1 = distanceTo(p1);
	double l2 = distanceTo(p2);

	// break ties by operator<
	if (h1 * l2 == h2 * l1) {
		return p1 < p2;
	}

	// otherwise, pick the steepest edge
	return h1 * l2 < h2 * l1;
}
