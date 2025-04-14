#include <cassert>
#include <cmath>
#include <iostream>

#include "piecewiselinearfunction.h"

LinearFunction::LinearFunction(double c0, double c1) : m_coefficients{c0, c1} {}

double LinearFunction::operator()(double h) {
	return m_coefficients[0] + m_coefficients[1] * h;
}

LinearFunction LinearFunction::add(const LinearFunction& other) const {
	return LinearFunction{m_coefficients[0] + other.m_coefficients[0],
	                      m_coefficients[1] + other.m_coefficients[1]};
}

LinearFunction LinearFunction::subtract(const LinearFunction& other) const {
	return LinearFunction{m_coefficients[0] - other.m_coefficients[0],
	                      m_coefficients[1] - other.m_coefficients[1]};
}

LinearFunction LinearFunction::multiply(double factor) const {
	return LinearFunction{m_coefficients[0] * factor, m_coefficients[1] * factor};
}

double LinearFunction::heightForVolume(double volume) {
	if (m_coefficients[1] == 0) {
		return std::numeric_limits<double>::quiet_NaN();
	}
	return (volume - m_coefficients[0]) / m_coefficients[1];
}

PiecewiseLinearFunction::PiecewiseLinearFunction() :
        m_breakpoints{},
        m_functions{LinearFunction()} {
}

PiecewiseLinearFunction::PiecewiseLinearFunction(LinearFunction function) :
        m_breakpoints{},
        m_functions{function} {
}

PiecewiseLinearFunction::PiecewiseLinearFunction(std::vector<double> breakpoints,
                           std::vector<LinearFunction> functions) :
        m_breakpoints(breakpoints), m_functions(functions) {
}

PiecewiseLinearFunction::PiecewiseLinearFunction(Point p1) {
	if (std::isnan(p1.h)) {
		m_breakpoints = {};
		m_functions = {LinearFunction{0, 0}};
		return;
	}
	m_breakpoints = {p1.h};
	m_functions = {LinearFunction{0.25 * p1.h, -0.25}, LinearFunction{}};
}

LinearFunction PiecewiseLinearFunction::functionAt(double h) {

	int i = std::distance(m_breakpoints.begin(),
	                      std::lower_bound(m_breakpoints.begin(),
	                                       m_breakpoints.end(), h));

	assert(i == 0 || m_breakpoints[i - 1] <= h);
	assert(i == m_breakpoints.size() || h <= m_breakpoints[i]);

	return m_functions[i];
}

double PiecewiseLinearFunction::operator()(double h) {
	double value = functionAt(h)(h);
	return value;
}

void PiecewiseLinearFunction::output(std::ostream& out) {
	out << "/" << std::endl;
	out << "| " << m_functions[0] << "  if h < " << m_breakpoints[0] << std::endl;
	for (int i = 0; i < m_breakpoints.size() - 1; i++) {
		out << "| " << m_functions[i + 1] << "  if " << m_breakpoints[i] << " <= h < " << m_breakpoints[i + 1] << std::endl;
	}
	out << "| " << m_functions.back() << "  if " << m_breakpoints.back() << " <= h" << std::endl;
	out << "\\" << std::endl;
}

[[nodiscard]] PiecewiseLinearFunction PiecewiseLinearFunction::add(
        const PiecewiseLinearFunction& other) const {
	std::vector<double> breakpoints;
	std::vector<LinearFunction> functions;
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

	return PiecewiseLinearFunction(breakpoints, functions);
}

PiecewiseLinearFunction PiecewiseLinearFunction::subtract(
        const PiecewiseLinearFunction& other) const {
	return add(other.multiply(-1));
}

PiecewiseLinearFunction PiecewiseLinearFunction::multiply(double factor) const {
	std::vector<LinearFunction> multipliedFunctions;
	for (LinearFunction f : m_functions) {
		multipliedFunctions.push_back(f.multiply(factor));
	}
	return PiecewiseLinearFunction(m_breakpoints, multipliedFunctions);
}

void PiecewiseLinearFunction::prune(double h) {

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

void PiecewiseLinearFunction::setToZeroAbove(double h) {
	prune(h);
	m_functions.emplace_back(0, 0);
	m_breakpoints.push_back(h);
}

double PiecewiseLinearFunction::heightForVolume(double volume) {
	assert(m_functions.size() == m_breakpoints.size() + 1);
	for (int i = 0; i < m_breakpoints.size(); i++) {
		double breakpoint = m_breakpoints[i];
		double volumeAtBreakpoint = m_functions[i + 1](breakpoint);
		if (volumeAtBreakpoint < volume) {
			double height = m_functions[i].heightForVolume(volume);
			height = std::min(height, m_breakpoints[i]);
			if (i > 0) {
				height = std::max(height, m_breakpoints[i - 1]);
			}
			return height;
		}
	}

	double height = m_functions.back().heightForVolume(volume);
	if (!m_breakpoints.empty()) {
		height = std::max(height, m_breakpoints.back());
	}
	return height;
}
