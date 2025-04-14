#ifndef GRADIENTFIELDSIMPLIFIER_H
#define GRADIENTFIELDSIMPLIFIER_H

#include <memory>

#include "mscomplex.h"

class GradientFieldSimplifier {

	public:

		// updates gradient pairing from `dcel`, assumes `msComplex` already
		// has delta values
		GradientFieldSimplifier(std::shared_ptr<InputDcel>& dcel,
		                        const std::shared_ptr<MsComplex>& msComplex, double delta,
		                        std::function<void(int)> progressListener = nullptr);

		void simplify();

	private:
		std::shared_ptr<InputDcel> m_dcel;
		const std::shared_ptr<MsComplex> m_msComplex;
		double m_delta;

		InputDcel::Face findMaximumFromSaddle(InputDcel::HalfEdge s);

		/**
		 * Calls the progress listener, if one is set.
		 * \param progress The progress value to report.
		 */
		void signalProgress(int progress);

		/**
		 * A function that is called when there is a progress update.
		 */
		std::function<void(int)> m_progressListener;
};

#endif // GRADIENTFIELDSIMPLIFIER_H
