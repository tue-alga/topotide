#include "riverdata.h"

#include <QDebug>

#include "boundary.h"

RiverFrame::RiverFrame(QString name, const HeightMap& heightMap) :
    m_name(name), m_heightMap(heightMap) {
}

RiverData::RiverData(int width, int height, Units units)
    : m_width(width), m_height(height), m_units(units) {
	setBoundary(Boundary{width, height});
}

int RiverData::width() const {
	return m_width;
}

int RiverData::height() const {
	return m_height;
}

void RiverData::addFrame(const std::shared_ptr<RiverFrame>& frame) {
	assert(frame->m_heightMap.width() == m_width);
	assert(frame->m_heightMap.height() == m_height);
	m_frames.push_back(frame);
	m_minElevation = std::min(m_minElevation, frame->m_heightMap.minimumElevation());
	m_maxElevation = std::max(m_minElevation, frame->m_heightMap.maximumElevation());
}

std::shared_ptr<RiverFrame> RiverData::getFrame(int i) {
	assert(i >= 0 && i < frameCount());
	return m_frames[i];
}

int RiverData::frameCount() const {
	return m_frames.size();
}

Boundary& RiverData::boundary() {
	return m_boundary;
}

Boundary& RiverData::boundaryRasterized() {
	return m_boundaryRasterized;
}

void RiverData::setBoundary(const Boundary& b) {
	m_boundary = b;
	m_boundaryRasterized = b.rasterize();
}

Units& RiverData::units() {
	return m_units;
}

double RiverData::minimumElevation() const {
	return m_minElevation;
}

double RiverData::maximumElevation() const {
	return m_maxElevation;
}
