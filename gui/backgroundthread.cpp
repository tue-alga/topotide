#include "backgroundthread.h"

#include <memory>

#include "inputgraph.h"
#include "mscomplexcreator.h"
#include "mscomplexsimplifier.h"
#include "mstonetworkgraphcreator.h"

BackgroundThread::BackgroundThread(const std::shared_ptr<RiverData>& data,
                                   const std::shared_ptr<RiverFrame>& frame, double msThreshold)
    : m_data(data), m_frame(frame), m_msThreshold(msThreshold) {}

void BackgroundThread::run() {
	computeInputGraph();
	computeInputDcel();
	computeMsComplex();
	simplifyMsComplex();
	msComplexToNetworkGraph();
}

void
BackgroundThread::computeInputGraph() {
	emit taskStarted("Computing input graph");
	auto inputGraph = std::make_shared<InputGraph>(
	                           m_frame->m_heightMap,
	                           m_data->boundaryRasterized(),
	                           m_data->units());
	emit progressMade("Computing input graph", 100);
	{
		QWriteLocker lock(&(m_frame->m_inputGraphLock));
		m_frame->m_inputGraph = inputGraph;
	}
	emit taskEnded("Computing input graph");
}

void
BackgroundThread::computeInputDcel() {
	emit taskStarted("Computing input DCEL");
	auto inputDcel = std::make_shared<InputDcel>(*m_frame->m_inputGraph);
	emit progressMade("Computing input DCEL", 100);
	{
		QWriteLocker lock(&(m_frame->m_inputDcelLock));
		m_frame->m_inputDcel = inputDcel;
	}
	emit taskEnded("Computing input DCEL");
}

void
BackgroundThread::computeMsComplex() {
	emit taskStarted("Computing MS complex");
	auto msComplex = std::make_shared<MsComplex>();
	MsComplexCreator msCreator(m_frame->m_inputDcel,
	                           msComplex, [this](int progress) {
		emit progressMade("Computing MS complex", progress);
	});
	msCreator.create();
	{
		QWriteLocker lock(&(m_frame->m_msComplexLock));
		m_frame->m_msComplex = msComplex;
	}
	emit taskEnded("Computing MS complex");
}

void
BackgroundThread::simplifyMsComplex() {
	emit taskStarted("Simplifying MS complex");
	auto msSimplified = std::make_shared<MsComplex>(*m_frame->m_msComplex);
	MsComplexSimplifier msSimplifier(msSimplified,
	                                 [this](int progress) {
		emit progressMade("Simplifying MS complex", progress);
	});
	msSimplifier.simplify();
	emit taskEnded("Simplifying MS complex");

	emit taskStarted("Compacting MS complex");

	msSimplified->compact();
	{
		QWriteLocker lock(&(m_frame->m_msComplexLock));
		m_frame->m_msComplex = msSimplified;
	}
	emit taskEnded("Compacting MS complex");
}

void
BackgroundThread::msComplexToNetworkGraph() {
	emit taskStarted("Converting MS complex into network");
	auto networkGraph = std::make_shared<NetworkGraph>();
	MsToNetworkGraphCreator networkGraphCreator(
	            m_frame->m_msComplex,
	            networkGraph,
	            [this](int progress) {
		emit progressMade("Converting MS complex into network", progress);
	});
	networkGraphCreator.create();
	{
		QWriteLocker lock(&(m_frame->m_networkGraphLock));
		m_frame->m_networkGraph = networkGraph;
	}
	emit taskEnded("Converting MS complex into network");
}
