#ifndef MSCOMPLEXSIMPLIFIER_H
#define MSCOMPLEXSIMPLIFIER_H

#include <memory>

#include "mscomplex.h"

/**
 * Implementation of an algorithm that simplifies a Morse-Smale complex by
 * removing saddle points (and merging their adjacent cells) if the sand volume
 * above the height of the saddle point on either side is below a given
 * threshold.
 *
 * Executing a simplification does not actually change the input Morse-Smale
 * complex. Instead it produces a list of δ-values `δ(e)` for each Morse-Smale
 * edge `e`. The network for any δ-value can then be reconstructed by dropping
 * any edge `e` with `δ(e) < δ`.
 */
class MsComplexSimplifier {

	public:

		/**
		 * Creates a Morse-Smale complex simplifier that can simplify the given
		 * Morse-Smale complex with a given threshold.
		 *
		 * \note Call simplify() to actually execute the simplification.
		 *
		 * \param msc The Morse-Smale complex to simplify.
		 * \param progressListener A function that is called when a progress
		 * update is available.
		 */
		MsComplexSimplifier(const std::shared_ptr<MsComplex>& msc,
		                    std::function<void(int)> progressListener = nullptr);

		/**
		 * Simplifies the Morse-Smale complex.
		 */
		void simplify();

	private:

		/**
		 * Computes the significance of a saddle, that is, the minimum of the
		 * two volumes on both sides.
		 *
		 * This method also returns which face has the most volume. This is the
		 * face that should be retained when merging its adjacent faces to
		 * continue the simplification.
		 *
		 * \param saddle The saddle to check.
		 * \return A pair containing two elements:
		 *
		 *  * the minimum of the two
		 *
		 *  * the outgoing edge of `saddle` whose incident face has the largest
		 *    volume above `saddle`.
		 */
		std::pair<double, MsComplex::HalfEdge>
		computeSaddleSignificance(MsComplex::Vertex saddle);

		/**
		 * The Morse-Smale complex we need to compute the δ-values for.
		 */
		std::shared_ptr<MsComplex> msc;

		/**
		 * The Morse-Smale complex we're going to simplify (remove saddles
		 * and edges from).
		 */
		MsComplex mscCopy;

		/**
		 * Calls the progress listener, if one is set.
		 * \param progress The progress value to report.
		 */
		void signalProgress(int progress);

		/**
		 * A function that is called when there is a progress update.
		 */
		std::function<void(int)> progressListener;
};

#endif // MSCOMPLEXSIMPLIFIER_H
