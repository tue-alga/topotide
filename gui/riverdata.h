#ifndef RIVERDATA_H
#define RIVERDATA_H

#include <QImage>
#include <QReadWriteLock>
#include <QStatusBar>

#include <memory>

#include "heightmap.h"
#include "inputdcel.h"
#include "inputgraph.h"
#include "mergetree.h"
#include "mscomplex.h"
#include "networkgraph.h"
#include "units.h"

/**
 * Collection of all data belonging to a single frame of the river.
 *
 * This class manages the creation of the heightmap, DCEL, MS-complex, and so
 * on. This needs to happen in a background thread to not block the GUI while
 * the computation takes place. To ensure thread-safety, this class provides
 * read-write locks (QReadWriteLock). When using a data element (for example,
 * to draw it in the GUI), the corresponding read lock must be acquired first.
 * During the computation of a data element, the corresponding write lock must
 * be acquired before writing back the result. (Note that it does not make
 * sense to keep the write lock during the entire computation, since that
 * defeats the purpose of being able to update the GUI while the computation
 * runs in the background. One should only obtain the write lock to copy back
 * the result.)
 *
 * This ensures that while several threads may read the
 * data simultaneously, it can never be written to at the same time (and it
 * is not possible for several threads to write simultaneously). For more
 * details, see the QReadWriteLock documentation.
 */
class RiverFrame : public QObject {

	Q_OBJECT

	public:

		/// Constructs a new river frame object with the given name and
		/// heightmap.
		RiverFrame(QString name, const HeightMap& heightMap);

		/// The name of this river data set. This is the name of the file
		/// containing height data that has been opened.
		QString m_name;
		/// The river heightmap.
		HeightMap m_heightMap;

		/**
		 * The input graph.
		 *
		 * \note Acquire inputGraphLock before reading / writing to this field.
		 */
		std::shared_ptr<InputGraph> m_inputGraph = nullptr;

		/**
		 * Read-write lock for inputGraph.
		 */
		QReadWriteLock m_inputGraphLock;

		/**
		 * The input DCEL.
		 *
		 * \note Acquire inputDcelLock before reading / writing to this field.
		 */
		std::shared_ptr<InputDcel> m_inputDcel = nullptr;

		/**
		 * Read-write lock for inputDcel.
		 */
		QReadWriteLock m_inputDcelLock;

		std::shared_ptr<MergeTree> m_mergeTree = nullptr;
		QReadWriteLock m_mergeTreeLock;

		/**
		 * The Morse-Smale complex.
		 *
		 * \note Acquire msComplexLock before reading / writing to this field.
		 */
		std::shared_ptr<MsComplex> m_msComplex = nullptr;

		/**
		 * Read-write lock for msComplex.
		 */
		QReadWriteLock m_msComplexLock;

		/**
		 * The graph of the network.
		 *
		 * \note Acquire networkGraphLock before reading / writing to this
		 * field.
		 */
		std::shared_ptr<NetworkGraph> m_networkGraph = nullptr;

		/**
		 * Read-write lock for networkGraph.
		 */
		QReadWriteLock m_networkGraphLock;

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
		/**
		 * The simplified input DCEL (with gradient pairs swapped).
		 *
		 * \note Acquire m_simplifiedInputDcelLock before reading / writing to
		 * this field.
		 */
		std::shared_ptr<InputDcel> m_simplifiedInputDcel = nullptr;

		/**
		 * Read-write lock for m_simplifiedInputDcel.
		 */
		QReadWriteLock m_simplifiedInputDcelLock;

		std::shared_ptr<std::vector<InputDcel::Path>> m_fingers = nullptr;
		QReadWriteLock m_fingersLock;
#endif
};

/**
 * Collection of all data belonging to a river (a time series or a single
 * frame).
 */
class RiverData : public QObject {

	Q_OBJECT

	public:
		/// Creates a new time series with the given dimensions, no frames,
		/// and a default boundary.
		RiverData(int width, int height, Units units);

		/// Returns the width of the frames in this time series.
		int width() const;
		/// Returns the width of the frames in this time series.
		int height() const;

		/// Inserts a new frame at the end of this time series.
		void addFrame(const std::shared_ptr<RiverFrame>& frame);
		/// Returns the `i`th frame.
		std::shared_ptr<RiverFrame> getFrame(int i);
		/// Returns the number of frames in this time series.
		int frameCount() const;

	    Boundary& boundary();
	    Boundary& boundaryRasterized();
	    /// Sets the boundary of the river. This updates both `boundary` and
		/// `boundaryRasterized`.
		void setBoundary(const Boundary& b);

		/// Returns the units.
		Units& units();

		/// Returns the lowest (non-nodata) elevation in this time series.
		double minimumElevation() const;
		/// Returns the highest (non-nodata) elevation in this time series.
		double maximumElevation() const;

	private:
		/// The width in pixels of frames in this time series.
		int m_width;
		/// The height in pixels of frames in this time series.
		int m_height;

		/// The list of frames.
		std::vector<std::shared_ptr<RiverFrame>> m_frames;

		/// The boundary of the river area.
		Boundary m_boundary;
		/// The boundary of the river area in rasterized form (see
		/// `Boundary::rasterize()`).
		Boundary m_boundaryRasterized;

		/// The mapping between our coordinates and real-world units.
		Units m_units;

		/// The minimum elevation across all frames in this time series.
		double m_minElevation = std::numeric_limits<double>::infinity();
		/// The maximum elevation across all frames in this time series.
		double m_maxElevation = -std::numeric_limits<double>::infinity();
};

#endif /* RIVERDATA_H */
