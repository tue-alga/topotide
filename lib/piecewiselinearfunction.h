#ifndef PIECEWISECUBICFUNCTION_H
#define PIECEWISECUBICFUNCTION_H

#include <array>
#include <vector>

#include "point.h"

/**
 * A linear function.
 */
class LinearFunction {

	public:

		/**
		 * \short Constructs a new linear function.
		 *
		 * Constructs a new linear function of the form
		 *
		 * ```
		 * c0 + c1 * x
		 * ```
		 *
		 * \param c0 The constant coefficient.
		 * \param c1 The linear coefficient.
		 */
		explicit LinearFunction(double c0 = 0, double c1 = 0);

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
		LinearFunction add(const LinearFunction& other) const;

		/**
		 * Subtracts a function from this function and returns the result.
		 *
		 * \param other The function to subtract.
		 * \return The resulting function.
		 */
		LinearFunction subtract(const LinearFunction& other) const;

		/**
		 * Multiplies this function by a factor and returns the result.
		 *
		 * \param factor The factor to multiply by.
		 * \return The resulting function.
		 */
		LinearFunction multiply(double factor) const;

		/**
		 * Computes the height *h* such that this function evaluates to the
		 * given volume. In case c1 == 0, this returns NaN.
		 *
		 * \param volume The volume to search for.
		 * \return The height *h* such that `(*this)(h) == volume` (modulo
		 * rounding error).
		 */
		double heightForVolume(double volume);

		/**
		 * Outputs a representation of a linear function to the given output
		 * stream.
		 *
		 * @param os The output stream.
		 * @param f The linear function to output.
		 * @return The output stream.
		 */
		friend std::ostream& operator<<(std::ostream& os,
		                                LinearFunction const& f) {
			os << f.m_coefficients[0] << " + "
			   << f.m_coefficients[1] << " h";
			return os;
		}

	private:

		/**
		 * An array containing the coefficients `c0` and `c1`, in order.
		 */
		std::array<double, 2> m_coefficients;
};

/**
 * A piecewise linear function that consists of a sequence of linear functions,
 * with breakpoints between them.
 */
class PiecewiseLinearFunction {

	public:

		/**
		 * Creates a piecewise linear function that evaluates to zero everywhere.
		 */
		PiecewiseLinearFunction();

		/**
		 * Creates a piecewise linear function that is defined by just one
		 * linear function.
		 *
		 * \param function The linear function.
		 */
		PiecewiseLinearFunction(LinearFunction function);

		/**
		 * Creates a piecewise linear function that is defined by a sequence of
		 * linear functions, with breakpoints between them.
		 *
		 * \param breakpoints A list of breakpoints, in ascending order.
		 * \param functions A list of linear functions, where `functions[0]` is
		 * the function used for `h < breakpoints[0]`, `functions[1]` is the
		 * function used for `breakpoints[0] < h < breakpoints[1]`, and so on.
		 */
		PiecewiseLinearFunction(std::vector<double> breakpoints,
		             std::vector<LinearFunction> functions);

		/**
		 * Creates a piecewise linear function representing the volume above
		 * height *h* of a quarter pillar with the height of the given point.
		 *
		 * \param p The center point of the pillar.
		 */
		PiecewiseLinearFunction(Point p);

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
		 * Outputs a representation of this piecewise linear function.
		 * \param out The output stream to write to.
		 */
		void output(std::ostream& out);

		/**
		 * Returns the linear function that is used to compute the function value
		 * of this piecewise linear function at a certain *h*-value.
		 *
		 * \param h The *h*-value.
		 * \return The linear function.
		 */
		LinearFunction functionAt(double h);

		/**
		 * Adds a function to this function and returns the result.
		 *
		 * \param other The function to add.
		 * \return The resulting function.
		 */
		[[nodiscard]] PiecewiseLinearFunction add(const PiecewiseLinearFunction& other) const;

		/**
		 * Subtracts a function to this function and returns the result.
		 *
		 * \param other The function to subtract.
		 * \return The resulting function.
		 */
		[[nodiscard]] PiecewiseLinearFunction subtract(const PiecewiseLinearFunction& other) const;

		/**
		 * Multiplies this function by a factor and returns the result.
		 *
		 * \param factor The factor to multiply by.
		 * \return The resulting function.
		 */
		[[nodiscard]] PiecewiseLinearFunction multiply(double factor) const;

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
		 * For all values above the given *h*-value, overwrites the value of
		 * this piecewise function to be zero.
		 *
		 * \param h The *h*-value to prune at.
		 */
		void setToZeroAbove(double h);

		/**
		 * Computes the height *h* such that this function evaluates to the
		 * given volume. This assumes that this function is decreasing.
		 *
		 * \param volume The volume to search for.
		 * \return The height *h* such that `(*this)(h) == volume` (modulo
		 * rounding error).
		 */
		double heightForVolume(double volume);

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
		std::vector<LinearFunction> m_functions;
};

#endif // PIECEWISECUBICFUNCTION_H
