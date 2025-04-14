#ifndef MSCOMPLEXCREATOR_H
#define MSCOMPLEXCREATOR_H

#include <functional>
#include <memory>

#include "inputdcel.h"
#include "mscomplex.h"

/// An algorithm for computing the descending Morse-Smale complex from an
/// InputDcel.
/// 
/// A descending Morse-Smale complex as represented by this class as a DCEL
/// whose vertices represent the critical points (minima and saddles) of the
/// InputDcel, and edges represent paths between minima and saddles.
/// Additionally, there is one vertex (in fact, the one with index 0) that
/// represents the outer face of the InputDcel. This "outer face minimum" is
/// connected to all saddles whose gradient path leads to somewhere on the
/// boundary of the InputDcel.
/// 
/// \todo Modify the representation such that vertices represent minima (not
/// saddles), and edges represent entire minimum-saddle-minimum paths.
/// 
/// Typical usage would be:
///
/// ```
/// // assuming some InputDcel& dcel
/// std::shared_ptr<MsComplex> msc = std::make_shared<MsComplex>();
/// MsComplexCreator creator(dcel, msc);
/// creator.create(msc);
/// ```
///
/// \note This is separate from the MsComplex class itself, so it is possible to
/// obtain progress information while the Morse-Smale complex is being computed.
class MsComplexCreator {

	public:
		/// Creates a Morse-Smale complex creator that can create a Morse-Smale
		/// complex from a DCEL.
		///
		/// This method assumes that there are no monkey saddles in the DCEL.
		/// If there are monkey saddles, the behaviour is undefined (read: it
		/// will crash the application). If needed, monkey saddles can be
		/// eliminated beforehand using InputDcel::splitMonkeySaddles().
		///
		/// \note Call create() to actually execute the algorithm.
		///
		/// \param dcel The DCEL to create a Morse-Smale complex from.
		/// \param msc An empty Morse-Smale complex to store the result in.
		/// \param progressListener A function that is called when a progress
		/// update is available.
		MsComplexCreator(const std::shared_ptr<InputDcel>& dcel,
		                 const std::shared_ptr<MsComplex>& msc,
		                 std::function<void(int)> progressListener = nullptr);

		/// Creates the Morse-Smale complex.
		void create();

	private:

		/// Adds all Morse-Smale edges connected to the given minimum.
		///
		/// This method creates all half-edges in the Morse-Smale complex around
		/// the minimum, and sets all next / previous pointers around the
		/// minimum, but leaves the next / previous pointers on the other side
		/// of the edges (that is, at the saddle) unset.
		void addEdgesFromMinimum(MsComplex::Vertex m);

		void addEdgesFromBoundaryMinimum(MsComplex::Vertex boundaryMinimum);

		void addEdgesFromMinimum(MsComplex::Vertex m, std::vector<InputDcel::Path> order);

		/// Returns the saddle order around the given minimum.
		///
		/// \return A list with saddles, in counter-clockwise order. Every
		/// saddle is represented by a path consisting of the half-edges in the
		/// saddle-to-minimum Morse-Smale edge.
		std::vector<InputDcel::Path> saddleOrder(InputDcel::Vertex m);

		/// Computes a part of the saddle order around a minimum, starting
		/// the search from the given vertex, and adds them to the list.
		///
		/// \param wedgeSteepestDescentEdge The steepest-descent half-edge that
		/// we used to arrive here (whose origin is `v`).
		/// \param order The list to add to.
		/// 
		/// \note Helper method for \c saddleOrder().
		void saddleOrderRecursive(InputDcel::HalfEdge wedgeSteepestDescentEdge,
		                          std::vector<InputDcel::Path>& order);

		/// Sets the `faces` set and the `maximum` pointer of the given face.
		void setDcelFacesOfFace(MsComplex::Face f);
		/// Finds the InputDcel face that is the maximum of the given MS-face.
		/// This can also be the outer face of the InputDcel.
		InputDcel::Face findFaceMaximum(MsComplex::Face f);

		/// Computes the sand function of the given face, and stores it in the
		/// face.
		void setSandFunctionOfFace(MsComplex::Face f);

		/// The DCEL we are going to compute a Morse-Smale complex for.
		std::shared_ptr<InputDcel> m_dcel;
		/// The Morse-Smale complex that we are going to store our result in.
		std::shared_ptr<MsComplex> m_msc;

		/// Calls the progress listener, if one is set.
		/// \param progress The progress value to report.
		void signalProgress(int progress);

		/// A function that is called when there is a progress update.
		std::function<void(int)> m_progressListener;
};

#endif // MSCOMPLEXCREATOR_H
