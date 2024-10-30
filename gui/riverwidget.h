#ifndef RIVERWIDGET_H
#define RIVERWIDGET_H

#include <QDragMoveEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QPoint>
#include <QString>
#include <QTabletEvent>
#include <QWheelEvent>

#include <memory>

#include "mscomplex.h"
#include "networkgraph.h"
#include "point.h"
#include "units.h"

#include "riverdata.h"

/**
 * The widget that visualizes the river.
 */
class RiverWidget : public QOpenGLWidget {

	Q_OBJECT

	public:
		RiverWidget();
		~RiverWidget() override;

		void initializeGL() override;
		void initializeShaders();
		void updateTexture();
		void paintGL() override;

		bool drawBackground() const;
		QString theme() const;
		double waterLevel() const;
		bool showElevation() const;
		bool showWaterPlane() const;
		bool showOutlines() const;
		int contourCount() const;
		bool showShading() const;
		bool showInputDcel() const;
		bool showMsComplex() const;
		bool msEdgesStraight() const;
		bool showStriation() const;
		bool showNetwork() const;
		int networkPath() const;
		double networkDelta() const;

		bool boundaryEditMode() const;

	public slots:
		void setRiverData(const std::shared_ptr<RiverData>& data);
		void setRiverFrame(const std::shared_ptr<RiverFrame>& frame);
		void setDrawBackground(bool drawBackground);
		void setTheme(QString theme);
		void setWaterLevel(double waterLevel);
		void setShowElevation(bool showElevation);
		void setShowWaterPlane(bool showWaterPlane);
		void setShowOutlines(bool showOutlines);
		void setContourCount(int contourCount);
		void setShowShading(bool showShading);
		void setShowInputDcel(bool showInputDcel);
		void setShowMsComplex(bool showMsComplex);
		void setMsEdgesStraight(bool msEdgesStraight);
		void setShowStriation(bool showStriation);
		void setShowNetwork(bool showNetwork);
		void setStriationItem(int item);
		void setNetworkPath(int path);
		void setNetworkDelta(double networkDelta);

		/**
		 * Resets the transformation to show the entire river.
		 */
		void resetTransform();
		void zoomIn();
		void zoomOut();
		void limitMaxZoom();

		void setUnits(Units units);

		void startBoundaryEditMode(const Boundary& boundary);
		Boundary endBoundaryEditMode();

	protected:
		void mouseMoveEvent(QMouseEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void wheelEvent(QWheelEvent* event) override;
		void leaveEvent(QEvent* event) override;

	signals:

		/**
		 * Emitted whenever the user moves the cursor.
		 *
		 * \param x The new x-coordinate (in river coordinates).
		 * \param y The new y-coordinate (in river coordinates).
		 * \param height The new height value.
		 */
		void hoveredCoordinateChanged(int x, int y, double height);

		/**
		 * Emitted whenever the cursor leaves the river widget.
		 */
		void mouseLeft();

	private:

		/**
		 * The transformation used for mapping river coordinates to screen
		 * coordinates.
		 */
		QTransform m_transform;

		const double MAX_ZOOM_FACTOR = 60.0;

		void drawUninitializedMessage(QPainter& p) const;
		void drawGrid(QPainter& p) const;
		void drawBoundary(QPainter& p, const Boundary& boundary) const;
		void drawBoundaryEditable(QPainter& p, Boundary& boundary);
		void drawBoundaryVertices(QPainter& p, const Path& path) const;
		void drawBoundaryVertex(QPainter& p, HeightMap::Coordinate c,
		                        bool outlined = false) const;
		void drawPath(QPainter& p, const Path& path) const;
		void drawCriticalPoints(QPainter& p, InputDcel& dcel) const;
		void drawGradientPairs(QPainter& p, InputDcel& dcel) const;
	    void drawArrow(QPainter& p, const QPointF& p1, const QPointF& p2) const;
	    void drawInputDcel(QPainter& p, InputDcel& dcel) const;
		void drawMsComplex(QPainter& p, MsComplex& msComplex) const;
		QPainterPath makePathRounded(const QPolygonF& path) const;
		void drawVertex(QPainter& p, Point p1, VertexType type) const;
		void drawMsEdge(QPainter& p, MsComplex::HalfEdge e) const;
		void drawNetwork(QPainter& p, const NetworkGraph& graph) const;
		void drawGraphEdge(QPainter& p, const NetworkGraph::Edge& e) const;
		QPolygonF polygonForMsFace(MsComplex::Face f) const;
		QPointF convertPoint(Point p) const;
		QPointF convertPoint(double x, double y) const;
		QPointF inverseConvertPoint(QPointF p) const;
		int hoveredMsVertex(MsComplex& msComplex) const;

		HeightMap::Coordinate* hoveredBoundaryVertex();

		// returns the first vertex of the edge of which the midpoint is
		// hovered
		std::pair<Path*, int> hoveredBoundaryMidpoint();

		HeightMap::Coordinate* m_draggedVertex = nullptr;

		std::shared_ptr<RiverData> m_riverData;
		std::shared_ptr<RiverFrame> m_riverFrame;

		QOpenGLShaderProgram* program;
		QOpenGLTexture* texture = nullptr;
		QOpenGLFunctions_3_0* gl;

		QString m_theme = "blue-yellow";
		double m_waterLevel = 0;
		bool m_drawBackground = true;
		bool m_showElevation = true;
		bool m_showWaterPlane = true;
		bool m_showOutlines = true;
		int m_contourCount = 20;
		bool m_showShading = false;
		bool m_showInputDcel = false;
		bool m_showMsComplex = false;
		bool m_msEdgesStraight = false;
		bool m_showStriation = false;
		bool m_showNetwork = true;
		double m_networkDelta = -1;

		QPixmap m_topoTideBackgroundImage;

		int m_striationItem = 0;
		int m_networkPath = 0;

		bool mouseInBounds = false;
		Point mousePos;
		QPointF m_previousMousePos;
		bool m_dragging = false;

		Units m_units;

		bool m_inBoundaryEditMode = false;
		Boundary m_boundaryToEdit;
};

#endif // RIVERWIDGET_H
