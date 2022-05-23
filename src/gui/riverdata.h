#ifndef RIVERDATA_H
#define RIVERDATA_H

#include <QImage>
#include <QReadWriteLock>
#include <QStatusBar>

#include "../heightmap.h"
#include "../inputdcel.h"
#include "../inputgraph.h"
#include "../lowestpathtree.h"
#include "../mscomplex.h"
#include "../network.h"
#include "../networkgraph.h"
#include "../sandcache.h"
#include "../striation.h"
#include "../units.h"

#include "progressdock.h"

/**
 * Collection of all data belonging to a river.
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
class RiverData : public QObject {

	Q_OBJECT

	public:

		/**
		 * Constructs a new, empty river data object.
		 */
		RiverData();

		/**
		 * Constructs a new river data object for the given river.
		 *
		 * \param name The name of the data set.
		 * \param image The height map image.
		 */
		RiverData(QString name, const QImage& image);

		/**
		 * Returns whether this river data object contains a river image.
		 *
		 * \return `true` if this river data contains a river; `false` if it
		 * does not contain a river.
		 */
		bool isInitialized();

		/**
		 * The name of this river data set. This is the name of the file
		 * containing height data that has been opened.
		 */
		QString name;

		/**
		 * The height map image.
		 */
		QImage image;

		/**
		 * The mapping between our coordinates and real-world units.
		 */
		Units units;

		/**
		 * The river height map.
		 */
		HeightMap heightMap;

		/**
		 * The boundary of the river area.
		 */
		Boundary boundary;

		/**
		 * The boundary of the river area in rasterized form (see
		 * `Boundary::rasterize()`).
		 */
		Boundary boundaryRasterized;

		/**
		 * The input graph.
		 *
		 * \note Acquire inputGraphLock before reading / writing to this field.
		 */
		InputGraph* inputGraph = nullptr;

		/**
		 * Read-write lock for inputGraph.
		 */
		QReadWriteLock inputGraphLock;

		/**
		 * The input DCEL.
		 *
		 * \note Acquire inputDcelLock before reading / writing to this field.
		 */
		InputDcel* inputDcel = nullptr;

		/**
		 * Read-write lock for inputDcel.
		 */
		QReadWriteLock inputDcelLock;

		/**
		 * The Morse-Smale complex.
		 *
		 * \note Acquire msComplexLock before reading / writing to this field.
		 */
		MsComplex* msComplex = nullptr;

		/**
		 * Read-write lock for msComplex.
		 */
		QReadWriteLock msComplexLock;

		/**
		 * The striation.
		 *
		 * \note Acquire striationLock before reading / writing to this field.
		 */
		Striation* striation = nullptr;

		/**
		 * Read-write lock for striation.
		 */
		QReadWriteLock striationLock;

		/**
		 * The sand cache.
		 *
		 * \note Acquire sandCacheLock before reading / writing to this field.
		 */
		SandCache* sandCache = nullptr;

		/**
		 * Read-write lock for sandCache.
		 */
		QReadWriteLock sandCacheLock;

		/**
		 * A list of the striation paths, sorted from low to high.
		 *
		 * \note Acquire sortedPathsLock before reading / writing to this field.
		 */
		std::vector<Network::Path>* sortedPaths = nullptr;

		/**
		 * Read-write lock for sortedPaths.
		 */
		QReadWriteLock sortedPathsLock;

		/**
		 * The representative network.
		 *
		 * \note Acquire networkLock before reading / writing to this field.
		 */
		Network* network = nullptr;

		/**
		 * Read-write lock for network.
		 */
		QReadWriteLock networkLock;

		/**
		 * The graph of the network.
		 *
		 * \note Acquire networkGraphLock before reading / writing to this
		 * field.
		 */
		NetworkGraph* networkGraph = nullptr;

		/**
		 * Read-write lock for networkGraph.
		 */
		QReadWriteLock networkGraphLock;

		/**
		 * Returns whether the background computation thread is running.
		 */
		bool isThreadRunning() const;

		/**
		 * Deletes all of the data and sets the pointers to `nullptr`.
		 * \param onlyNetwork If \c true, deletes only the network.
		 */
		void deleteEverything(bool onlyNetwork);

		/**
		 * Sets the boundary of thhe river. This updates both `boundary` and
		 * `boundaryRasterized`.
		 *
		 * \param b The new boundary.
		 */
		void setBoundary(Boundary& b);

	public slots:

		/**
		 * Starts the background thread to compute the river data.
		 *
		 * \param dock The ProgressDock that shows the progress.
		 * \param bar The QStatusBar that shows the progress.
		 * \param onlyNetwork If `true`, only the network will be recomputed.
		 * This assumes that all other objects (Morse-Smale complex, striation,
		 * ...) have been computed already.
		 * \param bidirectional Whether to use the bidirectional (`true`) or
		 * the unidirectional (`false`) sand function.
		 * \param simplify Whether to simplify the resulting graph.
		 * \param hybridStriation Whether to use the hybrid striation strategy.
		 * \param delta The δ-value.
		 */
		void startComputation(ProgressDock* dock,
		                      QStatusBar* bar,
		                      bool onlyNetwork,
		                      int sandFunction,
		                      bool bidirectional,
		                      bool simplify,
		                      bool hybridStriation,
		                      double delta,
		                      double msThreshold);

	signals:

		/**
		 * Emitted whenever there is new data ready to show.
		 */
		void dataChanged();

		/**
		 * Emitted whenever the computation starts.
		 */
		void computationStarted();

		/**
		 * Emitted whenever the computation finishes.
		 */
		void computationFinished();

	private:

		/**
		 * Whether the background thread is currently running.
		 */
		bool m_threadRunning = false;

		/**
		 * Called when the computation is finished.
		 */
		void markComputationFinished();
};

#endif /* RIVERDATA_H */
