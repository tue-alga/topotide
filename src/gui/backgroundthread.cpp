#include "../mscomplexcreator.h"
#include "../mscomplexsimplifier.h"
#include "../mstonetworkgraphcreator.h"
#include "../networkcreator.h"
#include "../networkgraphcreator.h"
#include "../sortedpathscreator.h"
#include "../striationcreator.h"

#include "backgroundthread.h"

BackgroundThread::BackgroundThread(RiverData* data,
                                   bool onlyNetwork,
                                   int sandFunction,
                                   bool bidirectional,
                                   bool simplify,
                                   bool hybridStriation,
                                   double delta,
                                   double msThreshold):
    m_data(data), m_onlyNetwork(onlyNetwork),
    m_sandFunction(sandFunction),
    m_bidirectional(bidirectional), m_simplify(simplify),
    m_hybridStriation(hybridStriation), m_delta(delta),
    m_msThreshold(msThreshold) {
}

void BackgroundThread::run() {

	if (m_msThreshold == 0 && m_onlyNetwork) {
		initializeSandCache();
		computeNetwork();
		computeNetworkGraph();
		return;
	}

	computeInputGraph();
	computeInputDcel();
	computeMsComplex();

	if (m_msThreshold == 0) {
		// run the SoCG (striation-based) algorithm
		computeStriation();
		computeSortedPaths();
		initializeSandCache();
		computeNetwork();
		computeNetworkGraph();

	} else {
		// run the persistence-based algorithm
		simplifyMsComplex();
		msComplexToNetworkGraph();
	}
}

void
BackgroundThread::computeInputGraph() {
	emit taskStarted("Computing input graph");
	auto* inputGraph(new InputGraph(
	                           m_data->heightMap,
	                           m_data->boundaryRasterized,
	                           m_data->units));
	emit progressMade("Computing input graph", 100);
	{
		QWriteLocker lock(&(m_data->inputGraphLock));
		m_data->inputGraph = inputGraph;
	}
	emit taskEnded("Computing input graph");
}

void
BackgroundThread::computeInputDcel() {
	emit taskStarted("Computing input DCEL");
	auto* inputDcel(new InputDcel(*m_data->inputGraph));
	emit progressMade("Computing input DCEL", 75);
	inputDcel->splitMonkeySaddles();
	emit progressMade("Computing input DCEL", 100);
	{
		QWriteLocker lock(&(m_data->inputDcelLock));
		m_data->inputDcel = inputDcel;
	}
	emit taskEnded("Computing input DCEL");
}

void
BackgroundThread::computeMsComplex() {
	emit taskStarted("Computing MS complex");
	auto* msComplex(new MsComplex());
	MsComplexCreator msCreator(*m_data->inputDcel,
	                           msComplex, [this](int progress) {
		emit progressMade("Computing MS complex", progress);
	});
	msCreator.create();
	{
		QWriteLocker lock(&(m_data->msComplexLock));
		m_data->msComplex = msComplex;
	}
	emit taskEnded("Computing MS complex");
}

void
BackgroundThread::computeStriation() {
	emit taskStarted("Computing striation");
	auto* striation(new Striation());
	StriationCreator striationCreator(*m_data->msComplex, striation,
	                                  m_data->units,
	                                  m_hybridStriation,
	                                  [this](int progress) {
		emit progressMade("Computing striation", progress);
	});
	striationCreator.create();
	{
		QWriteLocker lock(&(m_data->striationLock));
		m_data->striation = striation;
	}
	emit taskEnded("Computing striation");
}

void
BackgroundThread::computeSortedPaths() {
	emit taskStarted("Sorting striation paths on height");
	auto* sortedPaths(new std::vector<Network::Path>());
	SortedPathsCreator sortedPathsCreator(
	            m_data->msComplex, m_data->striation,
	            sortedPaths,
	            [this](int progress) {
		emit progressMade("Sorting striation paths on height", progress);
	});
	sortedPathsCreator.create();
	{
		QWriteLocker lock(&(m_data->sortedPathsLock));
		m_data->sortedPaths = sortedPaths;
	}
	emit taskEnded("Sorting striation paths on height");
}

void
BackgroundThread::initializeSandCache() {
	emit taskStarted("Initializing sand cache");
	SandCache::SandFunction sandFunction;
	switch (m_sandFunction) {
	case 0:
		sandFunction = &SandCache::waterLevelSandFunction;
		break;
	case 1:
	default:
		sandFunction = &SandCache::waterFlowSandFunction;
		break;
	case 2:
		sandFunction = &SandCache::symmetricFlowSandFunction;
		break;
	}
	SandCache* sandCache = new SandCache(m_data->msComplex,
	                                     m_data->striation,
	                                     sandFunction
	                                     );
	{
		QWriteLocker lock(&(m_data->sandCacheLock));
		m_data->sandCache = sandCache;
	}
	emit taskEnded("Initializing sand cache");
}

void
BackgroundThread::computeNetwork() {
	emit taskStarted("Computing representative network");
	auto* network(new Network());
	NetworkCreator networkCreator(
	            *m_data->striation,
	            *m_data->msComplex,
	            m_data->sandCache,
	            m_data->sortedPaths,
	            m_bidirectional,
	            m_delta,
	            network,
	            [this](int progress) {
		emit progressMade("Computing representative network", progress);
	});
	networkCreator.create();
	{
		QWriteLocker lock(&(m_data->networkLock));
		m_data->network = network;
	}
	emit taskEnded("Computing representative network");
}

void
BackgroundThread::computeNetworkGraph() {
	emit taskStarted("Converting network into graph");
	auto* networkGraph(new NetworkGraph());
	NetworkGraphCreator networkGraphCreator(
	            *m_data->msComplex,
	            *m_data->network,
	            networkGraph,
	            m_simplify,
	            [this](int progress) {
		emit progressMade("Converting network into graph", progress);
	});
	networkGraphCreator.create();
	{
		QWriteLocker lock(&(m_data->networkGraphLock));
		m_data->networkGraph = networkGraph;
	}
	emit taskEnded("Converting network into graph");
}

void
BackgroundThread::simplifyMsComplex() {
	emit taskStarted("Simplifying MS complex");
	auto* msSimplified(new MsComplex(*m_data->msComplex));
	MsComplexSimplifier msSimplifier(*msSimplified,
	                                 [this](int progress) {
		emit progressMade("Simplifying MS complex", progress);
	});
	msSimplifier.simplify();
	emit taskEnded("Simplifying MS complex");

	emit taskStarted("Compacting MS complex");

	msSimplified->compact();
	{
		QWriteLocker lock(&(m_data->msComplexLock));
		m_data->msComplex = msSimplified;
	}
	emit taskEnded("Compacting MS complex");
}

void
BackgroundThread::msComplexToNetworkGraph() {
	emit taskStarted("Converting MS complex into network");
	auto* networkGraph(new NetworkGraph());
	MsToNetworkGraphCreator networkGraphCreator(
	            *m_data->msComplex,
	            networkGraph,
	            [this](int progress) {
		emit progressMade("Converting MS complex into network", progress);
	});
	networkGraphCreator.create();
	{
		QWriteLocker lock(&(m_data->networkGraphLock));
		m_data->networkGraph = networkGraph;
	}
	emit taskEnded("Converting MS complex into network");
}
