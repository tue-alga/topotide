#ifndef MSCOMPLEXCREATOR_H
#define MSCOMPLEXCREATOR_H

#include <functional>
#include <memory>

#include "inputdcel.h"
#include "mscomplex.h"

/**
 * An algorithm for computing the descending Morse-Smale complex from an
 * InputDcel.
 *
 * Typical usage would be:
 *
 * ```
 * // assuming some InputDcel& dcel
 * std::shared_ptr<MsComplex> msc = std::make_shared<MsComplex>();
 * MsComplexCreator creator(dcel, msc);
 * creator.create(msc);
 * ```
 *
 * \note This is separate from the MsComplex class itself, so it is possible
 * to obtain progress information while the Morse-Smale complex is being
 * computed.
 */
class MsComplexCreator {

	public:

		/**
		 * Creates a Morse-Smale complex creator that can create a Morse-Smale
		 * complex from a DCEL.
		 *
		 * This method assumes that there are no monkey saddles in the DCEL.
		 * If there are monkey saddles, the behaviour is undefined (read: it
		 * will crash the application). If needed, monkey saddles can be
		 * eliminated beforehand using InputDcel::splitMonkeySaddles().
		 *
		 * \note Call create() to actually execute the algorithm.
		 *
		 * \param dcel The DCEL to create a Morse-Smale complex from.
		 * \param msc An empty Morse-Smale complex to store the result in.
		 * \param progressListener A function that is called when a progress
		 * update is available.
		 */
		MsComplexCreator(const std::shared_ptr<InputDcel>& dcel, const std::shared_ptr<MsComplex>& msc,
		                 std::function<void(int)> progressListener = nullptr);

		/**
		 * Creates the Morse-Smale complex.
		 */
		void create();

	private:

		/**
		 * Adds all Morse-Smale edges connected to the given minimum.
		 *
		 * This method creates all half-edges in the Morse-Smale complex around
		 * the minimum, and sets all next / previous pointers around the
		 * minimum, but leaves the next / previous pointers on the other side
		 * of the edges (that is, at the saddle) unset.
		 *
		 * \param m The minimum.
		 */
		void addEdgesFromMinimum(MsComplex::Vertex m);

		/**
		 * Returns the saddle order around a minimum.
		 *
		 * \param m The minimum.
		 * \return A list with saddles, in counter-clockwise order. Every
		 * saddle is represented by a path consisting of the half-edges in the
		 * saddle-to-minimum Morse-Smale edge.
		 */
	    std::vector<InputDcel::Path> saddleOrder(InputDcel::Vertex m);

	    /**
		 * Computes a part of the saddle order around a minimum, starting
		 * the search from the given vertex, and adds them to the list.
		 *
		 * \param v The vertex.
		 * \param wedgeSteepestDescentEdge The steepest-descent half-edge that
		 * we used to arrive here (whose origin is `v`).
		 * \param order The list to add to.
		 *
		 * \note Helper method for \c saddleOrder().
		 */
	    void saddleOrderRecursive(InputDcel::HalfEdge wedgeSteepestDescentEdge,
	                              std::vector<InputDcel::Path>& order);

		/**
		 * Sets the `faces` set and the `maximum` pointer of the given face.
		 *
		 * \param f The face to set the pointers of.
		 */
		void setDcelFacesOfFace(MsComplex::Face f);

		/**
		 * For every face in the Morse-Smale complex, compute its persistence
		 * value (and store it in the face data).
		 */
		void computePersistence();

		/**
		 * Computes the sand function of a face, and stores it in the face.
		 * \param f The face.
		 */
		void setSandFunctionOfFace(MsComplex::Face f);

		/**
		 * The DCEL we are going to compute a Morse-Smale complex for.
		 */
		std::shared_ptr<InputDcel> dcel;

		/**
		 * The Morse-Smale complex that we are going to store our result in.
		 */
		std::shared_ptr<MsComplex> msc;

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

#endif // MSCOMPLEXCREATOR_H
