#include <cassert>
#include <cmath>
#include <iostream>

#include "piecewisecubicfunction.h"

CubicFunction::CubicFunction(double c0, double c1, double c2, double c3) :
        m_coefficients{c0, c1, c2, c3} {
}

double CubicFunction::operator()(double h) {
	return m_coefficients[0]
	        + m_coefficients[1] * h
	        + m_coefficients[2] * h * h
	        + m_coefficients[3] * h * h * h;
}

CubicFunction CubicFunction::add(const CubicFunction& other) const {
	return {m_coefficients[0] + other.m_coefficients[0],
		        m_coefficients[1] + other.m_coefficients[1],
		        m_coefficients[2] + other.m_coefficients[2],
		        m_coefficients[3] + other.m_coefficients[3]};
}

CubicFunction CubicFunction::subtract(const CubicFunction& other) const {
	return {m_coefficients[0] - other.m_coefficients[0],
		        m_coefficients[1] - other.m_coefficients[1],
		        m_coefficients[2] - other.m_coefficients[2],
		        m_coefficients[3] - other.m_coefficients[3]};
}

CubicFunction CubicFunction::multiply(double factor) const {
	return {m_coefficients[0] * factor,
		        m_coefficients[1] * factor,
		        m_coefficients[2] * factor,
		        m_coefficients[3] * factor};
}

PiecewiseCubicFunction::PiecewiseCubicFunction() :
        m_breakpoints{},
        m_functions{CubicFunction()} {
}

PiecewiseCubicFunction::PiecewiseCubicFunction(CubicFunction function) :
        m_breakpoints{},
        m_functions{function} {
}

PiecewiseCubicFunction::PiecewiseCubicFunction(std::vector<double> breakpoints,
                           std::vector<CubicFunction> functions) :
        m_breakpoints(breakpoints), m_functions(functions) {
}

PiecewiseCubicFunction::PiecewiseCubicFunction(Point p1, Point p2, Point p3) {

	double h1 = p1.h;
	double h2 = p2.h;
	double h3 = p3.h;

	assert(h1 <= h2);
	assert(h2 <= h3);

	double t = std::abs(area(p1, p2, p3));

	double c = 0;
	double d = 0;

	if (h1 != h3) {
		Point p = p1 + (p3 - p1) * ((h2 - h1) / (h3 - h1));

		if (h2 != h3) {
			double tP = std::abs(area(p, p2, p3));
			c = tP / (3 * (h3 - h2) * (h3 - h2));
		}
		if (h1 != h2) {
			double tN = std::abs(area(p, p2, p1));
			d = tN / (3 * (h2 - h1) * (h2 - h1));
		}
	}

	CubicFunction tetN = CubicFunction(-d * h1 * h1 * h1,  // 1
	                                   3 * d * h1 * h1,    // h
	                                   -3 * d * h1,        // h^2
	                                   d);                 // h^3

	CubicFunction cub3 = CubicFunction(c * h3 * h3 * h3,
	                                   -3 * c * h3 * h3,
	                                   3 * c * h3,
	                                   -c);

	CubicFunction cub2 = CubicFunction(cub3(h2) + h2 * t - tetN(h2),
	                                   -t)
	                     .add(tetN);

	CubicFunction cub1 = CubicFunction(cub2(h1) + h1 * t,
	                                   -t);

	m_breakpoints = {h1, h2, h3};
	m_functions = {cub1, cub2, cub3, CubicFunction()};
}

double PiecewiseCubicFunction::area(Point p1, Point p2, Point p3) {

	double sum = 0;

	sum += (p2.x - p1.x) * (p1.y + p2.y);
	sum += (p3.x - p2.x) * (p2.y + p3.y);
	sum += (p1.x - p3.x) * (p3.y + p1.y);

	return sum / 2;
}

CubicFunction PiecewiseCubicFunction::functionAt(double h) {

	int i = std::distance(m_breakpoints.begin(),
	                      std::lower_bound(m_breakpoints.begin(),
	                                       m_breakpoints.end(), h));

	assert(i == 0 || m_breakpoints[i - 1] <= h);
	assert(i == m_breakpoints.size() || h <= m_breakpoints[i]);

	return m_functions[i];
}

double PiecewiseCubicFunction::operator()(double h) {
	double value = functionAt(h)(h);
	return value;
}

void PiecewiseCubicFunction::output(std::ostream& out) {
	out << "/" << std::endl;
	out << "| " << m_functions[0] << "  if h < " << m_breakpoints[0] << std::endl;
	for (int i = 0; i < m_breakpoints.size() - 1; i++) {
		out << "| " << m_functions[i + 1] << "  if " << m_breakpoints[i] << " <= h < " << m_breakpoints[i + 1] << std::endl;
	}
	out << "| " << m_functions.back() << "  if " << m_breakpoints.back() << " <= h" << std::endl;
	out << "\\" << std::endl;
}

PiecewiseCubicFunction PiecewiseCubicFunction::add(
        const PiecewiseCubicFunction& other) const {
	std::vector<double> breakpoints;
	std::vector<CubicFunction> functions;
	int i = 0;
	int j = 0;

	while (i < m_breakpoints.size() || j < other.m_breakpoints.size()) {
		functions.push_back(m_functions[i].add(other.m_functions[j]));
		if (j == other.m_breakpoints.size()
		        || (i < m_breakpoints.size() &&
		            m_breakpoints[i] < other.m_breakpoints[j])) {
			breakpoints.push_back(m_breakpoints[i]);
			i++;
		} else {
			breakpoints.push_back(other.m_breakpoints[j]);
			j++;
		}
	}
	functions.push_back(m_functions[i].add(other.m_functions[j]));

	return PiecewiseCubicFunction(breakpoints, functions);
}

PiecewiseCubicFunction PiecewiseCubicFunction::subtract(
        const PiecewiseCubicFunction& other) const {
	return add(other.multiply(-1));
}

PiecewiseCubicFunction PiecewiseCubicFunction::multiply(double factor) const {
	std::vector<CubicFunction> multipliedFunctions;
	for (CubicFunction f : m_functions) {
		multipliedFunctions.push_back(f.multiply(factor));
	}
	return PiecewiseCubicFunction(m_breakpoints, multipliedFunctions);
}

void PiecewiseCubicFunction::prune(double h) {

	// we need the i-th function to calculate the correct value of f(h)
	int i = std::distance(m_breakpoints.begin(),
	                      std::lower_bound(m_breakpoints.begin(),
	                                       m_breakpoints.end(), h));

	if (i == m_functions.size() - 1) {
		// nothing to prune
		return;
	}

	// so we can remove the (i + 1)-th function and further
	m_functions.resize(i + 1);
	m_breakpoints.resize(i);
}
