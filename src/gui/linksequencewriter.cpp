#include <QFile>

#include "linksequencewriter.h"

void LinkSequenceWriter::writeLinkSequence(LinkSequence& linkSequence,
                                           const Units& units,
                                           const QString& fileName) {

	QFile file(fileName);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);

	out << linkSequence.linkCount() << "\n";
	for (int i = 0; i < linkSequence.linkCount(); i++) {
		LinkSequence::Link& link = linkSequence.link(i);
		out << i << " "
		    << units.toRealVolume(link.delta) << " ";

		for (int j = 0; j < link.path.size(); j++) {
			Point p = link.path[j];
			out << p.x << " " << p.y
			    << (j == link.path.size() - 1 ? "\n" : " ");
		}
	}
}
