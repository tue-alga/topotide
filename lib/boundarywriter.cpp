#include <QFile>

#include "boundarywriter.h"

void BoundaryWriter::writeBoundary(Boundary& boundary,
                                   const QString& fileName) {
	assert(boundary.permeableRegions().size() == 2);

	QFile file(fileName);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);

	Boundary::Region source = boundary.permeableRegions()[0];
	Boundary::Region sink = boundary.permeableRegions()[1];

	const Path& path = boundary.path();

	out << regionLength(path, source.m_start, source.m_end) << "\n";
	out << regionLength(path, source.m_end, sink.m_start) << "\n";
	out << regionLength(path, sink.m_start, sink.m_end) << "\n";
	out << regionLength(path, sink.m_end, source.m_start) << "\n";

	writeRegion(out, path, source.m_start, source.m_end);
	writeRegion(out, path, source.m_end, sink.m_start);
	writeRegion(out, path, sink.m_start, sink.m_end);
	writeRegion(out, path, sink.m_end, source.m_start);
}

int BoundaryWriter::regionLength(const Path& path, int start, int end) {
	if (start <= end) {
		return end - start + 1;
	} else {
		return path.length() - (start - end) + 1;
	}
}

void BoundaryWriter::writeRegion(QTextStream& out, const Path& path, int start, int end) {
	for (int i = start; i != end; i = (i + 1) % path.length()) {
		out << path.m_points[i].m_x << " " << path.m_points[i].m_y << "\n";
	}
	out << path.m_points[end].m_x << " " << path.m_points[end].m_y << "\n";
}
