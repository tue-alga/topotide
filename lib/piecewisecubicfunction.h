#ifndef PIECEWISECUBICFUNCTION_H
#define PIECEWISECUBICFUNCTION_H

#include <array>
#include <vector>

#include "point.h"

/**
 * A cubic function.
 */
class CubicFunction {

	public:

		/**
		 * \short Constructs a new cubic function.
		 *
		 * Constructs a new cubic function of the form
		 *
		 * ```
		 * c0 + c1 * x + c2 * x^2 + c3 * x^3
		 * ```
		 *
		 * \param c0 The constant coefficient.
		 * \param c1 The linear coefficient.
		 * \param c2 The quadratic coefficient.
		 * \param c3 The cubic coefficient.
		 */
		CubicFunction(double c0 = 0,
		              double c1 = 0,
		              double c2 = 0,
		              double c3 = 0);

		/**
		 * Evaluates the function at a certain *h*-value.
		 *
		 * \param h The *h*-value.
		 * \return The function value at `h`.
		 */
		double operator()(double h);

		/**
		 * Adds a function to this function and returns the result.
		 *
		 * \param other The function to add.
		 * \return The resulting function.
		 */
		CubicFunction add(const CubicFunction& other) const;

		/**
		 * Subtracts a function from this function and returns the result.
		 *
		 * \param other The function to subtract.
		 * \return The resulting function.
		 */
		CubicFunction subtract(const CubicFunction& other) const;

		/**
		 * Multiplies this function by a factor and returns the result.
		 *
		 * \param factor The factor to multiply by.
		 * \return The resulting function.
		 */
		CubicFunction multiply(double factor) const;

		/**
		 * Outputs a representation of a cubic function to the given output
		 * stream.
		 *
		 * @param os The output stream.
		 * @param f The cubic function to output.
		 * @return The output stream.
		 */
		friend std::ostream& operator<<(std::ostream& os,
		                                CubicFunction const& f) {
			os << f.m_coefficients[0] << " + "
			   << f.m_coefficients[1] << " h + "
			   << f.m_coefficients[2] << " h^2 + "
			   << f.m_coefficients[3] << " h^3";
			return os;
		}

	private:

		/**
		 * An array containing the coefficients `c0`, `c1`, `c2` and `c3`, in
		 * order.
		 */
		std::array<double, 4> m_coefficients;
};

/**
 * A piecewise cubic function that consists of a sequence of cubic functions,
 * with breakpoints between them.
 */
class PiecewiseCubicFunction {

	public:

		/**
		 * Creates a piecewise cubic function that evaluates to zero everywhere.
		 */
		PiecewiseCubicFunction();

		/**
		 * Creates a piecewise cubic function that is defined by just one
		 * cubic function.
		 *
		 * \param function The cubic function.
		 */
		PiecewiseCubicFunction(CubicFunction function);

		/**
		 * Creates a piecewise cubic function that is defined by a sequence of
		 * cubic functions, with breakpoints between them.
		 *
		 * \param breakpoints A list of breakpoints, in ascending order.
		 * \param functions A list of cubic functions, where `functions[0]` is
		 * the function used for `h < breakpoints[0]`, `functions[1]` is the
		 * function used for `breakpoints[0] < h < breakpoints[1]`, and so on.
		 */
		PiecewiseCubicFunction(std::vector<double> breakpoints,
		             std::vector<CubicFunction> functions);

		/**
		 * Creates a piecewise cubic function representing the volume of sand
		 * above height *h* in the given triangle.
		 *
		 * \param p1 The lowest point of the triangle.
		 * \param p2 The middle point of the triangle.
		 * \param p3 The highest point of the triangle.
		 */
		PiecewiseCubicFunction(Point p1, Point p2, Point p3);

		/**
		 * Creates a piecewise cubic function representing the volume above
		 * height *h* of a quarter pillar with the height of the given point.
		 *
		 * \param p The center point of the pillar.
		 */
		PiecewiseCubicFunction(Point p);

		/**
		 * Evaluates the function at a certain *h*-value.
		 *
		 * This is equivalent to `functionAt(h)(h)`.
		 *
		 * \param h The *h*-value.
		 * \return The function value at `h`.
		 */
		double operator()(double h);

		/**
		 * Outputs a representation of this piecewise cubic function.
		 * \param out The output stream to write to.
		 */
		void output(std::ostream& out);

		/**
		 * Returns the cubic function that is used to compute the function value
		 * of this piecewise cubic function at a certain *h*-value.
		 *
		 * \param h The *h*-value.
		 * \return The cubic function.
		 */
		CubicFunction functionAt(double h);

		/**
		 * Adds a function to this function and returns the result.
		 *
		 * \param other The function to add.
		 * \return The resulting function.
		 */
		PiecewiseCubicFunction add(const PiecewiseCubicFunction& other) const;

		/**
		 * Subtracts a function to this function and returns the result.
		 *
		 * \param other The function to subtract.
		 * \return The resulting function.
		 */
		PiecewiseCubicFunction subtract(const PiecewiseCubicFunction& other) const;

		/**
		 * Multiplies this function by a factor and returns the result.
		 *
		 * \param factor The factor to multiply by.
		 * \return The resulting function.
		 */
		PiecewiseCubicFunction multiply(double factor) const;

		/**
		 * Prunes the piecewise function by removing pieces above the given
		 * *h*-value. After pruning, calling the function with a parameter
		 * at most the pruning value will give exactly the same result; however
		 * for parameters larger than the pruning value, the result may change.
		 *
		 * \param h The *h*-value to prune at.
		 */
		void prune(double h);

		/**
		 * Returns the signed area of the triangle spanned by the three
		 * given points.
		 *
		 * \param p1 The first point.
		 * \param p2 The second point.
		 * \param p3 The third point.
		 * \return The signed area.
		 */
		static double area(Point p1, Point p2, Point p3);

	private:

		/**
		 * The list of breakpoints, in ascending order.
		 */
		std::vector<double> m_breakpoints;

		/**
		 * The list of functions, where `functions[0]` is the function used for
		 * `h < breakpoints[0]`, `functions[1]` is the function used for
		 * `breakpoints[0] < h < breakpoints[1]`, and so on.
		 */
		std::vector<CubicFunction> m_functions;
};

#endif // PIECEWISECUBICFUNCTION_H
