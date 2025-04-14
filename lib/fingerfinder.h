#ifndef FINGERFINDER_H
#define FINGERFINDER_H

#include <memory>
#include <optional>
#include <utility>

#include "mscomplex.h"

class FingerFinder {

	public:
		FingerFinder(std::shared_ptr<InputDcel>& dcel, const std::shared_ptr<MsComplex>& msComplex,
		             double delta, std::function<void(int)> progressListener = nullptr);

		std::vector<InputDcel::Path> findFingers();

	private:
		std::shared_ptr<InputDcel> m_dcel;
		const std::shared_ptr<MsComplex> m_msComplex;
		double m_delta;

		/**
		 * Returns a piecewise cubic function representing the volume of sand
		 * above any faces reachable from this halfedge by following edge-face
		 * gradient pairs.
		 *
		 * This method assumes that the volumeAbove fields of all descendants
		 * have already been set.
		 *
		 * \param edge The halfedge to return the sand function of.
		 * \return The resulting sand function.
		 */
		PiecewiseLinearFunction volumeAbove(InputDcel::HalfEdge edge);

	    double maximumVertexHeight(InputDcel::Face face);

	    /**
		 * Computes the edge volumes (see \ref volumeAbove(InputDcel::HalfEdge)
		 * for the red tree emanating from maximum \c f.
		 */
		void computeVolumesForRedTree(InputDcel::Face f);

		std::pair<InputDcel::HalfEdge, std::vector<int>> computeTopEdge(InputDcel::Face f, double delta);
		std::vector<InputDcel::Face> findSignificantLeafOrder(InputDcel::Face f);
		void findSignificantLeafOrderForSubtree(InputDcel::HalfEdge e,
												std::vector<InputDcel::Face>& order);
		std::optional<InputDcel::Vertex> findFingerTip(InputDcel::Face leaf1, InputDcel::Face leaf2);
	    void computeSpurBoundary(InputDcel::HalfEdge topEdge, std::vector<int>& result);

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

#endif // FINGERFINDER_H
