#include "backgroundthread.h"

#include <memory>

#include "inputgraph.h"
#include "mergetree.h"
#include "mscomplexcreator.h"
#include "mscomplexsimplifier.h"
#include "mstonetworkgraphcreator.h"

BackgroundThread::BackgroundThread(const std::shared_ptr<RiverData>& data,
                                   const std::shared_ptr<RiverFrame>& frame)
    : m_data(data), m_frame(frame) {}

BackgroundThread::BackgroundThread(const std::shared_ptr<RiverData>& data)
    : m_data(data), m_frame(nullptr) {}

void BackgroundThread::run() {
	if (!m_data->boundaryRasterized().isValid()) {
		emit errorEncountered("The computation cannot run as the boundary is invalid. A valid "
		                      "boundary does not self-intersect and does not visit "
		                      "any points more than once.");
		return;
	}
	if (m_frame) {
		computeForFrame();
	} else {
		for (int i = 0; i < m_data->frameCount(); i++) {
			m_taskPrefix = QString("Frame %1/%2: ").arg(i + 1).arg(m_data->frameCount());
			m_frame = m_data->getFrame(i);
			if (!computeForFrame()) {
				break;
			}
		}
	}
}

bool BackgroundThread::computeForFrame() {
	m_frame->m_inputGraph = nullptr;
	m_frame->m_inputDcel = nullptr;
	m_frame->m_msComplex = nullptr;
	m_frame->m_networkGraph = nullptr;

	computeInputGraph();
	if (m_frame->m_inputGraph->containsNodata()) {
		emit errorEncountered(
			"The computation cannot run as there are nodata values inside the boundary.");
		return false;
	}
	computeInputDcel();
	computeMsComplex();
	computeMergeTree();
	simplifyMsComplex();
	msComplexToNetworkGraph();
	return true;
}

void
BackgroundThread::computeInputGraph() {
	emit taskStarted(m_taskPrefix + "Computing input graph");
	auto inputGraph = std::make_shared<InputGraph>(
	                           m_frame->m_heightMap,
	                           m_data->boundaryRasterized());
	emit progressMade(m_taskPrefix + "Computing input graph", 100);
	{
		QWriteLocker lock(&(m_frame->m_inputGraphLock));
		m_frame->m_inputGraph = inputGraph;
	}
	emit taskEnded(m_taskPrefix + "Computing input graph");
}

void
BackgroundThread::computeInputDcel() {
	emit taskStarted(m_taskPrefix + "Computing input DCEL");
	auto inputDcel = std::make_shared<InputDcel>(*m_frame->m_inputGraph);
	inputDcel->computeGradientFlow();
	emit progressMade(m_taskPrefix + "Computing input DCEL", 100);
	{
		QWriteLocker lock(&(m_frame->m_inputDcelLock));
		m_frame->m_inputDcel = inputDcel;
	}
	emit taskEnded(m_taskPrefix + "Computing input DCEL");
}

void
BackgroundThread::computeMsComplex() {
	emit taskStarted(m_taskPrefix + "Computing MS complex");
	auto msComplex = std::make_shared<MsComplex>();
	MsComplexCreator msCreator(m_frame->m_inputDcel,
	                           msComplex, [this](int progress) {
		emit progressMade(m_taskPrefix + "Computing MS complex", progress);
	});
	msCreator.create();
	{
		QWriteLocker lock(&(m_frame->m_msComplexLock));
		m_frame->m_msComplex = msComplex;
	}
	emit taskEnded(m_taskPrefix + "Computing MS complex");
}

void
BackgroundThread::computeMergeTree() {
	emit taskStarted(m_taskPrefix + "Computing merge tree");
	auto mergeTree = std::make_shared<MergeTree>(m_frame->m_msComplex);
	emit progressMade(m_taskPrefix + "Computing merge tree", 100);
	{
		QWriteLocker lock(&(m_frame->m_mergeTreeLock));
		m_frame->m_mergeTree = mergeTree;
	}
	emit taskEnded(m_taskPrefix + "Computing merge tree");
}

void
BackgroundThread::simplifyMsComplex() {
	emit taskStarted(m_taskPrefix + "Simplifying MS complex");
	auto msSimplified = std::make_shared<MsComplex>(*m_frame->m_msComplex);
	MsComplexSimplifier msSimplifier(msSimplified,
	                                 [this](int progress) {
		emit progressMade(m_taskPrefix + "Simplifying MS complex", progress);
	});
	msSimplifier.simplify();
	emit taskEnded(m_taskPrefix + "Simplifying MS complex");

	emit taskStarted(m_taskPrefix + "Compacting MS complex");

	msSimplified->compact();
	{
		QWriteLocker lock(&(m_frame->m_msComplexLock));
		m_frame->m_msComplex = msSimplified;
	}
	emit taskEnded(m_taskPrefix + "Compacting MS complex");
}

void
BackgroundThread::msComplexToNetworkGraph() {
	emit taskStarted(m_taskPrefix + "Converting MS complex into network");
	auto networkGraph = std::make_shared<NetworkGraph>();
	MsToNetworkGraphCreator networkGraphCreator(
	            m_frame->m_msComplex,
	            networkGraph,
	            [this](int progress) {
		emit progressMade(m_taskPrefix + "Converting MS complex into network", progress);
	});
	networkGraphCreator.create();
	{
		QWriteLocker lock(&(m_frame->m_networkGraphLock));
		m_frame->m_networkGraph = networkGraph;
	}
	emit taskEnded(m_taskPrefix + "Converting MS complex into network");
}
