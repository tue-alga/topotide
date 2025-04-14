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
#include <optional>

#include "boundarycreator.h"
#include "colorramp.h"
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
		void updateElevationRampTexture();
		void paintGL() override;

		bool drawBackground() const;
		ColorRamp colorRamp() const;
		double waterLevel() const;
		double contourLevel() const;
		bool showElevation() const;
		bool showWaterPlane() const;
		bool showOutlines() const;
		int contourCount() const;
		bool showShading() const;
		bool showInputDcel() const;
		bool drawGradientPairsAsTrees() const;
		bool showCriticalPoints() const;
		bool showMsComplex() const;
		bool msEdgesStraight() const;
		bool showNetwork() const;
		double networkDelta() const;
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
		bool showSpurs() const;
		bool showFingers() const;
#endif

		bool boundaryEditMode() const;

	public slots:
		void setRiverData(const std::shared_ptr<RiverData>& data);
		void setRiverFrame(const std::shared_ptr<RiverFrame>& frame);
		void setDrawBackground(bool drawBackground);
		void setColorRamp(ColorRamp colorRamp);
		void setWaterLevel(double waterLevel);
		void setContourLevel(double contourLevel);
		void setShowElevation(bool showElevation);
		void setShowWaterPlane(bool showWaterPlane);
		void setShowOutlines(bool showOutlines);
		void setContourCount(int contourCount);
		void setShowShading(bool showShading);
		void setShowInputDcel(bool showInputDcel);
		void setDrawGradientPairsAsTrees(bool drawGradientPairsAsTrees);
		void setShowCriticalPoints(bool showCriticalPoints);
		void setShowMsComplex(bool showMsComplex);
		void setMsEdgesStraight(bool msEdgesStraight);
		void setShowNetwork(bool showNetwork);
		void setNetworkDelta(double networkDelta);
		void setPointToHighlight(std::optional<Point> point);
		void setContourMask(QImage mask);
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
		void setShowSpurs(bool showSpurs);
		void setShowFingers(bool showFingers);
		void setShowSimplified(bool showSimplified);
#endif

		/**
		 * Resets the transformation to show the entire river.
		 */
		void resetTransform();
		void zoomIn();
		void zoomOut();
		void limitMaxZoom();

		void setUnits(Units units);

		void startBoundaryGenerateMode();
		void startSetSourceSinkMode();
		void startBoundaryEditMode();
		void endBoundaryEditMode();

		void setPointsToDraw(std::vector<Point> pointsToDraw);
		void setEdgesToDraw(std::vector<InputDcel::HalfEdge> edgesToDraw);

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
		 * \param coord The new x- and y-coordinates (in river coordinates).
		 * \param height The new height value.
		 */
		void hoveredCoordinateChanged(HeightMap::Coordinate coord, double height);

		/**
		 * Emitted whenever the cursor leaves the river widget.
		 */
		void mouseLeft();

		void boundaryEdited(std::optional<Boundary> boundary);

		void statusMessage(QString message);

	private:

		/**
		 * The transformation used for mapping river coordinates to screen
		 * coordinates.
		 */
		QTransform m_transform;

		const double MAX_ZOOM_FACTOR = 120.0;

		void drawUninitializedMessage(QPainter& p) const;
		void drawGrid(QPainter& p) const;
		void drawBoundary(QPainter& p, const Boundary& boundary) const;
		void drawBoundaryEditable(QPainter& p, Boundary& boundary);
		void drawBoundaryVertices(QPainter& p, const Path& path) const;
		void drawBoundaryVertex(QPainter& p, HeightMap::Coordinate c,
		                        bool outlined = false) const;
		void drawBoundaryCreatorState(QPainter& p);
		void drawPath(QPainter& p, const Path& path) const;
	    void drawPermeableRegion(QPainter& p, const Path& path,
	                             const Boundary::Region& region) const;
	    void drawCriticalPoints(QPainter& p, InputDcel& dcel) const;
		void drawGradientPairs(QPainter& p, InputDcel& dcel) const;
	    void drawArrow(QPainter& p, const QPointF& p1, const QPointF& p2) const;
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
		void drawSpurs(QPainter& p, InputDcel& dcel) const;
#endif
	    void drawInputDcel(QPainter& p, InputDcel& dcel) const;
		void drawMsComplex(QPainter& p, MsComplex& msComplex) const;
		QPainterPath makePathRounded(const QPolygonF& path) const;
		void drawVertex(QPainter& p, Point p1, VertexType type) const;
		void drawMsEdge(QPainter& p, MsComplex::HalfEdge e) const;
		void drawNetwork(QPainter& p, const NetworkGraph& graph) const;
		void drawGraphEdge(QPainter& p, const NetworkGraph::Edge& e) const;
		void drawFingers(QPainter& p, const std::vector<InputDcel::Path>& fingers) const;
		QPolygonF polygonForMsFace(MsComplex::Face f) const;
		QPointF convertPoint(Point p) const;
		QPointF convertPoint(double x, double y) const;
		QPointF inverseConvertPoint(QPointF p) const;
		int hoveredMsVertex(MsComplex& msComplex) const;
		std::optional<int> hoveredPathVertex(const Path& p) const;
		std::optional<int> hoveredNonPermeableBoundaryVertex(const Boundary& b) const;

		// returns the first vertex of the edge of which the midpoint is
		// hovered
		std::optional<int> hoveredBoundaryMidpoint();

		/// Checks if the given point is finite, or always returns `true` if
		/// the RiverWidget is set to draw out-of-bounds elements. This is
		/// intended to be used for checking if we should skip drawing something
		/// as it's out of bounds.
		bool inBounds(Point p) const;
		bool m_drawOutOfBounds = false;

		std::optional<int> m_draggedVertex;

		std::shared_ptr<RiverData> m_riverData;
		std::shared_ptr<RiverFrame> m_riverFrame;

		QOpenGLShaderProgram* program;
		QOpenGLTexture* texture = nullptr;
		QOpenGLTexture* elevationRampTexture = nullptr;
		/// texture that masks where the contour is shown (black = not shown, white = shown)
		QOpenGLTexture* contourMaskTexture = nullptr;
		QOpenGLFunctions_3_0* gl;

		ColorRamp m_colorRamp;
		double m_waterLevel = 0;
		double m_contourLevel = 0;
		bool m_drawBackground = true;
		bool m_showElevation = true;
		bool m_showWaterPlane = true;
		bool m_showOutlines = true;
		int m_contourCount = 20;
		bool m_showShading = false;
		bool m_showInputDcel = false;
		bool m_drawGradientPairsAsTrees = false;
		bool m_showCriticalPoints = false;
		bool m_showMsComplex = false;
		bool m_msEdgesStraight = false;
		bool m_showNetwork = true;
		double m_networkDelta = -1;
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
		bool m_showSpurs = false;
		bool m_showFingers = false;
		bool m_showSimplified = false;
#endif
		std::optional<Point> m_pointToHighlight;

		QPixmap m_topoTideBackgroundImage;

		bool mouseInBounds = false;
		Point mousePos;
		QPointF m_previousMousePos;
		bool m_dragging = false;

		Units m_units;

		enum class Mode {
			NORMAL,
			EDIT_BOUNDARY,
			GENERATE_BOUNDARY_SEED,
			SET_SOURCE_START,
			SET_SOURCE_END,
			SET_SINK_START,
			SET_SINK_END
		};
		Mode m_mode = Mode::NORMAL;

		Boundary m_boundaryToEdit;
		std::vector<Point> m_pointsToDraw;
		std::vector<InputDcel::HalfEdge> m_edgesToDraw;
		std::optional<BoundaryCreator> m_boundaryCreator;

		const QColor BOUNDARY_COLOR{"#999999"};
		const QColor EDITABLE_BOUNDARY_COLOR{"black"};
		const QColor PERMEABLE_BOUNDARY_COLOR{13, 110, 190};
};

#endif // RIVERWIDGET_H
