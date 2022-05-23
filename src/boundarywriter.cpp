#include <QFile>

#include "boundarywriter.h"

void BoundaryWriter::writeBoundary(Boundary& boundary,
                                   const QString& fileName) {

	QFile file(fileName);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);

	out << boundary.m_source.length() + 1 << "\n";
	out << boundary.m_top.length() + 1 << "\n";
	out << boundary.m_sink.length() + 1 << "\n";
	out << boundary.m_bottom.length() + 1 << "\n";

	writePath(out, boundary.m_source);
	writePath(out, boundary.m_top);
	writePath(out, boundary.m_sink);
	writePath(out, boundary.m_bottom);
}

void BoundaryWriter::writePath(QTextStream& out, Path& path) {
	for (int i = 0; i <= path.length(); i++) {
		out << path.m_points[i].m_x << " "
		    << path.m_points[i].m_y << "\n";
	}
}
