#include "mergetreedock.h"

#include <QMouseEvent>

MergeTreeDock::MergeTreeDock(QWidget *parent) : QDockWidget("Merge tree", parent) {
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	setMouseTracking(true);
}

QSize MergeTreeDock::minimumSizeHint() const {
	return QSize(50, 100);
}

QSize MergeTreeDock::sizeHint() const {
	return QSize(100, 200);
}

void MergeTreeDock::setMergeTree(std::shared_ptr<MergeTree> mergeTree) {
	m_mergeTree = mergeTree;
	if (mergeTree == nullptr) {
		setEnabled(false);
	} else {
		m_sortedMergeTree = std::make_unique<MergeTree>(*mergeTree);
		m_sortedMergeTree->sort([this](MergeTree::Node& n1, MergeTree::Node& n2) {
			Point p1 = n1.m_p;
			Point p2 = n2.m_p;
			return p1.x < p2.x;
		});
		setEnabled(true);
		update();
	}
}

void MergeTreeDock::mouseMoveEvent(QMouseEvent* event) {
	if (!m_mergeTree) {
		return;
	}

	if (event->position().y() < TREE_MARGIN || event->position().y() > rect().height() - TREE_MARGIN) {
		leaveEvent(event);
		return;
	}

	m_hoveredHeight = inverseConvertHeight(event->position().y());
	emit hoveredHeightChanged(m_hoveredHeight);
	m_hoveredNodeId = std::nullopt;

	int column = std::round((event->position().x() - TREE_MARGIN) / COLUMN_WIDTH);

	// if hovering the color ramp: show the entire level set
	m_hoveringRamp = column < 0;
	if (m_hoveringRamp) {
		QImage mask(m_mapWidth, m_mapHeight, QImage::Format::Format_ARGB32);
		mask.fill(QColor{"white"});
		emit hoveredSubtreeMaskChanged(mask);

	// if hovering a column of the merge tree: show the corresponding contour
	} else if (m_columnToNodeMap.contains(column)) {
		int nodeId = m_columnToNodeMap[column];
		const MergeTree::Node& columnNode = m_sortedMergeTree->get(nodeId);
		m_hoveredNodeId = m_sortedMergeTree->parentAtHeight(columnNode.m_index, m_hoveredHeight);
		if (m_hoveredNodeId) {
			QImage mask = constructMaskImage(m_sortedMergeTree->get(*m_hoveredNodeId));
			emit hoveredSubtreeMaskChanged(mask);
		} else {
			QImage mask(m_mapWidth, m_mapHeight, QImage::Format::Format_ARGB32);
			mask.fill(QColor{"black"});
			emit hoveredSubtreeMaskChanged(mask);
		}

	} else {
		QImage mask(m_mapWidth, m_mapHeight, QImage::Format::Format_ARGB32);
		mask.fill(QColor{"black"});
		emit hoveredSubtreeMaskChanged(mask);
	}
	
	update();
}

void MergeTreeDock::leaveEvent(QEvent*) {
	m_hoveringRamp = false;
	m_hoveredNodeId = std::nullopt;

	QImage mask(m_mapWidth, m_mapHeight, QImage::Format::Format_ARGB32);
	mask.fill(QColor{"black"});
	emit hoveredSubtreeMaskChanged(mask);

	update();
}

void MergeTreeDock::paintEvent(QPaintEvent*) {
	if (!m_mergeTree) {
		return;
	}

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	m_columnToNodeMap.clear();
	const MergeTree::Node& root = m_sortedMergeTree->root();
	painter.setPen(QPen{QColor{"black"}, 2});
	drawSubtree(painter, root, 0, rect().height() - TREE_MARGIN);

	if (m_hoveredNodeId) {
		painter.setPen(QPen{QColor{"#238b45"}, 2});
		double x = columnToX(m_nodeToColumnMap[*m_hoveredNodeId]);
		double y = convertHeight(m_hoveredHeight);
		painter.drawLine(QPointF{x - COLUMN_WIDTH / 2.0f, y}, QPointF{x + COLUMN_WIDTH / 2.0f, y});
	}

	painter.save();
	painter.rotate(-90);
	painter.drawImage(QRect{QPoint{-rect().height() + TREE_MARGIN, TREE_MARGIN / 4},
	                        QSize{rect().height() - 2 * TREE_MARGIN, TREE_MARGIN / 2}},
	                  m_colorRamp);
	painter.restore();
	// if hovering the ramp, draw a horizontal line on the ramp at that height
	if (m_hoveringRamp) {
		painter.setPen(QPen{QColor{"#238b45"}, 2});
		double y = convertHeight(m_hoveredHeight);
		painter.drawLine(QPointF{TREE_MARGIN / 4.0f, y}, QPointF{3 * TREE_MARGIN / 4.0f, y});
	}
}

void MergeTreeDock::setElevationRange(double minElevation, double maxElevation) {
	m_minElevation = minElevation;
	m_maxElevation = maxElevation;
}

void MergeTreeDock::setMapSize(int width, int height) {
	m_mapWidth = width;
	m_mapHeight = height;
}

void MergeTreeDock::setColorRamp(ColorRamp colorRamp) {
	m_colorRamp = colorRamp.toImage();
	update();
}

void MergeTreeDock::setDelta(double delta) {
	m_delta = delta;
	update();
}

double MergeTreeDock::convertHeight(double height) {
	double fraction = (height - m_minElevation) / (m_maxElevation - m_minElevation);
	double treeHeight = rect().height() - 2 * TREE_MARGIN;
	return (1 - fraction) * treeHeight + TREE_MARGIN;
}

double MergeTreeDock::inverseConvertHeight(double height) {
	double treeHeight = rect().height() - 2 * TREE_MARGIN;
	double fraction = 1 - (height - TREE_MARGIN) / treeHeight;
	return fraction * (m_maxElevation - m_minElevation) + m_minElevation;
}

double MergeTreeDock::columnToX(int column) {
	return column * COLUMN_WIDTH + TREE_MARGIN;
}

int MergeTreeDock::drawSubtree(QPainter& painter, const MergeTree::Node& root, int column, double yParent) {
	m_columnToNodeMap[column] = root.m_index;
	m_nodeToColumnMap[root.m_index] = column;

	double height = root.m_p.h;
	double x = columnToX(column);
	double y = convertHeight(height);
	if (y != yParent) {
		painter.drawLine(QPointF{x, y}, QPointF{x, yParent});
	}

	int currentColumn = column;
	int subtreeColumnCount = 0;
	int childrenDrawn = 0;
	for (int childId : root.m_children) {
		const MergeTree::Node& child = m_sortedMergeTree->get(childId);
		if (child.m_volumeAbove < m_delta) {
			continue;
		}
		if (childrenDrawn > 0) {
			currentColumn++;
		}
		childrenDrawn++;
		subtreeColumnCount = drawSubtree(painter, child, currentColumn, y);
		currentColumn += subtreeColumnCount;
	}
	if (childrenDrawn > 1) {
		painter.drawLine(QPointF{x, y}, QPointF{columnToX(currentColumn - subtreeColumnCount), y});
	}

	return currentColumn - column;
};

QImage MergeTreeDock::constructMaskImage(const MergeTree::Node& root) {
	QImage mask(m_mapWidth, m_mapHeight, QImage::Format::Format_ARGB32);
	mask.fill(QColor{"black"});
	addSubtreeToMask(mask, root);
	return mask;
}

void MergeTreeDock::addSubtreeToMask(QImage& mask, const MergeTree::Node& root) {
	if (root.m_children.empty()) {
		assert(std::holds_alternative<MsComplex::Face>(root.m_criticalSimplex));
		MsComplex::Face maximum = std::get<MsComplex::Face>(root.m_criticalSimplex);
		for (InputDcel::Face f : maximum.data().faces) {
			Point p = f.data().p;
			int x = std::floor(p.x);
			int y = std::floor(p.y);
			mask.setPixelColor(x, y, QColor{"white"});
		}
		return;
	}

	for (int child : root.m_children) {
		addSubtreeToMask(mask, m_sortedMergeTree->get(child));
	}
}
