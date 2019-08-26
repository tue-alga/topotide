#include <QFile>

#include "graphwriter.h"
#include "../networkgraph.h"

void GraphWriter::writeGraph(NetworkGraph& graph,
                             const Units& units,
                             const QString& fileName) {

	QFile file(fileName);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);

	out << graph.vertexCount() << "\n";
	for (int i = 0; i < graph.vertexCount(); i++) {
		out << i << " "
		    << graph[i].p.x << " "
		    << graph[i].p.y << "\n";
	}
	out << graph.edgeCount() << "\n";
	for (int i = 0; i < graph.edgeCount(); i++) {
		out << i << " "
		    << graph.edge(i).from << " "
		    << graph.edge(i).to << " "
		    << units.toRealVolume(graph.edge(i).delta) << " ";

		for (int j = 0; j < graph.edge(i).path.size(); j++) {
			Point p = graph.edge(i).path[j];
			out << p.x << " " << p.y
			    << (j == graph.edge(i).path.size() - 1 ? "\n" : " ");
		}
	}
}
