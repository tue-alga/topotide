#ifndef MERGETREEWIDGET
#define MERGETREEWIDGET

#include <QDockWidget>
#include <QPen>
#include <QPainter>

#include <memory>
#include <optional>

#include "colorramp.h"
#include "mergetree.h"

/**
* Widget that visualises a merge tree.
*/

class MergeTreeDock : public QDockWidget {

	Q_OBJECT

	public:
		MergeTreeDock(QWidget *parent = nullptr);

		/*
		* Holds the recommended size of the widget
		*/
		QSize sizeHint() const override;

		/*
		* Holds the minimum size of the widget
		*/
		QSize minimumSizeHint() const override;

	signals:
		void pointToHighlightChanged(std::optional<Point> pos);
		void hoveredHeightChanged(double height);
		void hoveredSubtreeMaskChanged(QImage mask);

	public slots:
		/**
		 * Changes the merge tree that is being visualized.
		 * \param mergeTree The merge tree, or `nullptr` to disable the widget.
		 */
		void setMergeTree(std::shared_ptr<MergeTree> mergeTree);
		void setElevationRange(double minElevation, double maxElevation);
		void setMapSize(int width, int height);
		void setColorRamp(ColorRamp colorRamp);
		void setDelta(double delta);

	protected:
		void mouseMoveEvent(QMouseEvent* event) override;
		void paintEvent(QPaintEvent *event) override;
		void leaveEvent(QEvent* event) override;

	private:
		/// Converts from heights to pixels.
		double convertHeight(double height);
		/// Converts from pixels to heights.
		double inverseConvertHeight(double height);
		double columnToX(int column);
		/// Returns the number of columns spanned by the subtree drawn.
		int drawSubtree(QPainter& painter, const MergeTree::Node& root, int column, double yParent);

		QImage constructMaskImage(const MergeTree::Node& root);
		void addSubtreeToMask(QImage& mask, const MergeTree::Node& root);

		std::shared_ptr<MergeTree> m_mergeTree = nullptr;
		std::unique_ptr<MergeTree> m_sortedMergeTree = nullptr;

		std::map<int, int> m_columnToNodeMap;
		std::map<int, int> m_nodeToColumnMap;
		std::optional<int> m_hoveredNodeId;

		const int TREE_MARGIN = 20;
		const int COLUMN_WIDTH = 8;

		double m_delta;

		double m_minElevation;
		double m_maxElevation;
		double m_hoveredHeight;
		bool m_hoveringRamp = false;

		int m_mapWidth;
		int m_mapHeight;

		QImage m_colorRamp;
};

#endif
