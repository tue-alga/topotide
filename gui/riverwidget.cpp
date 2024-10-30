#include <QBrush>
#include <QColor>
#include <QCursor>
#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QImage>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QOpenGLShader>
#include <QOpenGLVersionFunctionsFactory>
#include <QOpenGLVersionProfile>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QReadLocker>
#include <QStack>
#include <QTextDocument>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "riverwidget.h"

RiverWidget::RiverWidget() {
	QSurfaceFormat format;
	setMouseTracking(true);
	format.setSamples(16);
	setFormat(format);
	m_topoTideBackgroundImage = QPixmap(":res/icons/topotide-background.svg");
}

RiverWidget::~RiverWidget() {
	if (texture) {
		delete texture;
	}
}

void RiverWidget::mouseMoveEvent(QMouseEvent* event) {
	if (!m_riverFrame) {
		return;
	}

	QPointF converted = inverseConvertPoint(event->pos());
	auto x = static_cast<int>(converted.x() + 0.5);
	auto y = static_cast<int>(converted.y() + 0.5);
	mousePos = Point(x, y, 0);
	mouseInBounds = m_riverFrame->m_heightMap.isInBounds(x, y);
	if (mouseInBounds) {
		hoveredCoordinateChanged(x, y,
		                         (float) m_riverFrame->m_heightMap.elevationAt(x, y));
	} else {
		emit mouseLeft();
	}

	if (m_dragging) {
		if (m_inBoundaryEditMode && m_draggedVertex != nullptr) {
			m_draggedVertex->m_x = mousePos.x;
			m_draggedVertex->m_y = mousePos.y;
			m_boundaryToEdit.ensureConnection();
		} else {
			QPointF delta = event->pos() - m_previousMousePos;
			QTransform translation;
			translation.translate(delta.x(), delta.y());
			m_transform = m_transform * translation;
		}
	}

	m_previousMousePos = event->pos();
	update();
}

void RiverWidget::mousePressEvent(QMouseEvent* event) {
	if (!m_riverFrame) {
		return;
	}

	m_dragging = true;

	if (m_inBoundaryEditMode) {
		m_draggedVertex = hoveredBoundaryVertex();
		if (m_draggedVertex == nullptr) {
			std::pair<Path*, int> newMidpoint = hoveredBoundaryMidpoint();
			Path* p = newMidpoint.first;
			if (p != nullptr) {
				int i = newMidpoint.second;
				HeightMap::Coordinate c1 = p->m_points[i];
				HeightMap::Coordinate c2 = p->m_points[i + 1];
				HeightMap::Coordinate midP((c1.m_x + c2.m_x) / 2,
				                           (c1.m_y + c2.m_y) / 2);
				p->m_points.insert(p->m_points.begin() + i + 1, midP);
				m_draggedVertex = &(p->m_points[i + 1]);
			}
		}
	}

	setCursor(Qt::ClosedHandCursor);
	m_previousMousePos = event->pos();

	update();
}

void RiverWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (!m_riverFrame) {
		return;
	}

	m_dragging = false;
	setCursor(Qt::ArrowCursor);
	m_draggedVertex = nullptr;
}

void RiverWidget::leaveEvent(QEvent* event) {
	if (!m_riverFrame) {
		return;
	}

	mouseInBounds = false;
	emit mouseLeft();
	update();
}

void RiverWidget::wheelEvent(QWheelEvent* event) {
	if (!m_riverFrame) {
		return;
	}
	if (event->angleDelta().isNull()) {
		return;
	}

	double delta = event->angleDelta().y();
	double factor = pow(2, delta / 240);

	// limit the zoom factor
	// note: we cannot use limitZoomFactor() here, because then while it would
	// indeed be impossible to zoom in further than the MAX_ZOOM_FACTOR, the
	// translation would still be carried out
	if (factor * m_transform.m11() > MAX_ZOOM_FACTOR) {
		factor = MAX_ZOOM_FACTOR / m_transform.m11();
	}

	QPointF mousePos = event->position();
	mousePos -= QPointF(width() / 2.0, height() / 2.0);
	QTransform transform;
	transform.translate(mousePos.x(), mousePos.y());
	transform.scale(factor, factor);
	transform.translate(-mousePos.x(), -mousePos.y());
	m_transform *= transform;

	update();
}

void RiverWidget::initializeGL() {
	gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_0>(
		QOpenGLContext::currentContext());
	gl->initializeOpenGLFunctions();
	updateTexture();
	initializeShaders();
}

void RiverWidget::initializeShaders() {

	// compile shaders and program
	QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex);
	vshader->compileSourceFile(":/res/shader/river.vert");

	QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment);
	fshader->compileSourceFile(":/res/shader/river-" + m_theme + ".frag");

	program = new QOpenGLShaderProgram();
	program->addShader(vshader);
	program->addShader(fshader);
	program->link();
}

void RiverWidget::updateTexture() {
	if (!m_riverFrame) {
		return;
	}
	const HeightMap& heightMap = m_riverFrame->m_heightMap;
	double min = heightMap.minimumElevation();
	double max = heightMap.maximumElevation();
	QImage image(heightMap.width(), heightMap.height(), QImage::Format::Format_ARGB32);
	for (int y = 0; y < heightMap.height(); ++y) {
		for (int x = 0; x < heightMap.width(); ++x) {
			double elevation = heightMap.elevationAt(x, y);
			if (elevation == HeightMap::nodata) {
				image.setPixel(x, y, 0x00000000);
			} else {
				unsigned int value =
				    0xff000000 +
				    0xffffff * (elevation - m_riverData->minimumElevation()) /
				        (m_riverData->maximumElevation() - m_riverData->minimumElevation());
				image.setPixel(x, y, value);
			}
		}
	}
	texture = new QOpenGLTexture(image);
	texture->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void RiverWidget::paintGL() {

	QPainter p(this);

	if (!m_riverFrame) {
		drawUninitializedMessage(p);
		return;
	}

	// initialize shader variables
	program->bind();
	program->setUniformValue(
	    "waterLevel", (float) ((m_waterLevel - m_riverData->minimumElevation()) /
	                           (m_riverData->maximumElevation() - m_riverData->minimumElevation())));
	program->setUniformValue("showMap", m_showElevation);
	program->setUniformValue("showOutlines", m_showOutlines);
	program->setUniformValue("contourCount", m_contourCount);
	program->setUniformValue("showShading", m_showShading);
	program->setUniformValue("showWaterPlane", m_showWaterPlane);
	program->setUniformValue("lineWidth", 1.0f / width());
	program->setUniformValue("lineHeight", 1.0f / height());

	// bind elevation texture
	if (texture != nullptr) {
		texture->bind(0);
		program->setUniformValue("tex", 0);
		program->setUniformValue("texWidth", (float) texture->width());
		program->setUniformValue("texHeight", (float) texture->height());
	}

	// draw background map using OpenGL
	QTransform transform;
	transform.translate(0.5, 0.5);
	transform.scale(1.0 / (m_riverFrame->m_heightMap.width()),
	                1.0 / (m_riverFrame->m_heightMap.height()));
	transform = m_transform.inverted() * transform;
	transform.scale(0.5 * width(), -0.5 * height());
	transform.scale(1, m_units.m_xResolution / m_units.m_yResolution);
	program->setUniformValue("matrix", transform);

	if (m_drawBackground) {
		gl->glBegin(GL_QUADS);
		gl->glVertex2d(-1, -1);
		gl->glVertex2d(1, -1);
		gl->glVertex2d(1, 1);
		gl->glVertex2d(-1, 1);
		gl->glEnd();
	} else {
		p.fillRect(rect(), QColor(255, 255, 255));
	}

	// draw overlays using QPainter
	if (m_transform.m11() > 10) {
		drawGrid(p);
	}

	if (m_inBoundaryEditMode) {
		drawBoundaryEditable(p, m_boundaryToEdit);

	} else {
		if (m_riverData) {
			drawBoundary(p, m_riverData->boundaryRasterized());
		}

		if (m_showInputDcel &&
		        m_riverFrame->m_inputDcel != nullptr) {
			QReadLocker lock(&(m_riverFrame->m_inputDcelLock));
			drawGradientPairs(p, *m_riverFrame->m_inputDcel);
		}

		if (m_showMsComplex &&
		        m_riverFrame->m_msComplex != nullptr) {
			QReadLocker locker(&(m_riverFrame->m_msComplexLock));
			drawMsComplex(p, *m_riverFrame->m_msComplex);
		}

		if (m_showMsComplex &&
		        m_riverFrame->m_inputDcel != nullptr) {
			QReadLocker lock(&(m_riverFrame->m_inputDcelLock));
			drawCriticalPoints(p, *m_riverFrame->m_inputDcel);
		}

		if (m_showNetwork &&
		        m_riverFrame->m_msComplex != nullptr &&
		        m_riverFrame->m_networkGraph != nullptr) {
			drawNetwork(p, *m_riverFrame->m_networkGraph);
		}

		if (mouseInBounds) {
			drawVertex(p, mousePos, VertexType::disconnected);
		}
	}
}

void RiverWidget::drawUninitializedMessage(QPainter& p) const {
	p.setBrush(palette().window());
	p.setPen(Qt::NoPen);
	p.drawRect(rect());

	p.setOpacity(0.2);
	p.drawPixmap(QRect(rect().bottomRight() - QPoint(370, 345), rect().bottomRight() + QPoint(-10, 15)),
	            m_topoTideBackgroundImage);
	p.setOpacity(1);

	p.setBrush(Qt::NoBrush);
	p.setPen(QPen(palette().text().color()));
	QTextDocument text;
	text.setHtml("<div style='text-align: center;'>"
	             "<h2>No river loaded</h2>"
	             "<p>To open a river, use <i>File &gt; Open DEM...</i> or "
	             "drag-and-drop an image or text file containing elevation "
	             "data onto this window.</p>");
	text.setTextWidth(rect().width());
	text.drawContents(&p, rect());
}

void RiverWidget::drawGrid(QPainter &p) const {
	double opacity = (m_transform.m11() - 10.0) / 50.0;
	if (opacity > 1) {
		opacity = 1;
	}
	int a = static_cast<int>(opacity * 128);
	p.setPen(QPen(QBrush(QColor(0, 0, 0, a)), 1));

	// vertical lines
	float yTop = convertPoint(0, 0).y();
	float yBottom = convertPoint(0, texture->height() - 1).y();
	for (int x = 0; x < texture->width(); x++) {
		float xF = convertPoint(x, 0).x();
		p.drawLine(QPointF(xF, yTop), QPointF(xF, yBottom));
	}

	// horizontal lines
	float xLeft = convertPoint(0, 0).x();
	float xRight = convertPoint(texture->width() - 1, 0).x();
	for (int y = 0; y < texture->height(); y++) {
		float yF = convertPoint(0, y).y();
		p.drawLine(QPointF(xLeft, yF), QPointF(xRight, yF));
	}
}

void RiverWidget::drawBoundary(
        QPainter& p, const Boundary& boundary) const {

	p.setPen(QPen(QBrush(QColor("black")), 2));
	drawPath(p, boundary.m_top);
	drawPath(p, boundary.m_bottom);

	p.setPen(QPen(QBrush(QColor(13, 110, 190)), 3));
	drawPath(p, boundary.m_source);
	p.setPen(QPen(QBrush(QColor(163, 99, 4)), 3));
	drawPath(p, boundary.m_sink);
}

void RiverWidget::drawBoundaryEditable(QPainter& p, Boundary& boundary) {

	const HeightMap::Coordinate* hovered = hoveredBoundaryVertex();
	if (hovered != nullptr) {
		drawBoundaryVertex(p, *hovered, true);
	}

	drawBoundary(p, boundary);

	if (hovered == nullptr) {
		std::pair<const Path*, int> mid = hoveredBoundaryMidpoint();
		if (mid.first != nullptr) {
			HeightMap::Coordinate c1 = mid.first->m_points[mid.second];
			HeightMap::Coordinate c2 = mid.first->m_points[mid.second + 1];
			Point midP((c1.m_x + c2.m_x) / 2.0,
			           (c1.m_y + c2.m_y) / 2.0,
			           0);
			p.setPen(QPen(QBrush(QColor(60, 60, 60, 128)), 0));
			p.setBrush(QBrush(QColor(255, 255, 255, 128)));
			QPointF p2 = convertPoint(midP);
			p.drawEllipse(p2.x() - 4, p2.y() - 4, 8, 8);
		}
	}

	drawBoundaryVertices(p, boundary.m_source);
	drawBoundaryVertices(p, boundary.m_top);
	drawBoundaryVertices(p, boundary.m_sink);
	drawBoundaryVertices(p, boundary.m_bottom);
}

void RiverWidget::drawBoundaryVertices(QPainter& p, const Path& path) const {
	for (HeightMap::Coordinate c : path.m_points) {
		if (c != path.end()) {
			drawBoundaryVertex(p, c);
		}
	};
}

void RiverWidget::drawBoundaryVertex(
        QPainter& p, HeightMap::Coordinate c,
        bool outlined) const {

	if (outlined) {
		p.setPen(QPen(QBrush(QColor(80, 130, 255)), 8));
	} else {
		p.setPen(QPen(QBrush(QColor(60, 60, 60)), 0));
	}
	p.setBrush(QBrush(QColor(255, 255, 255)));

	QPointF p2 = convertPoint(Point(c.m_x, c.m_y, 0));
	p.drawEllipse(p2.x() - 4, p2.y() - 4, 8, 8);
}

void RiverWidget::drawPath(
        QPainter& p, const Path& path) const {

	std::vector<HeightMap::Coordinate> points = path.m_points;
	for (int i = 0; i < (int) (points.size()) - 1; i++) {
		HeightMap::Coordinate p1 = points[i];
		HeightMap::Coordinate p2 = points[i + 1];
		p.drawLine(convertPoint(p1.m_x, p1.m_y),
		           convertPoint(p2.m_x, p2.m_y));
	}
}

void RiverWidget::drawCriticalPoints(QPainter& p, InputDcel& dcel) const {
	QBrush blue(QColor(60, 90, 220));
	QBrush green(QColor(110, 180, 90));
	QBrush black(QColor(0, 0, 0, 127));
	QBrush red(QColor(220, 90, 60, 127));

	for (size_t i = 0; i < dcel.halfEdgeCount(); i++) {
		InputDcel::HalfEdge e = dcel.halfEdge(i);
		if (e.origin().id() < e.destination().id()) {
			continue;  // avoid drawing the same edge twice
		}
		if (!e.data().pairedWithVertex && !e.data().pairedWithFace &&
		    !e.twin().data().pairedWithVertex && !e.twin().data().pairedWithFace &&
		    std::isfinite(e.data().p.h)) {
			p.setPen(QPen(black, 6));
			p.drawLine(0.5 * (convertPoint(e.origin().data().p) + convertPoint(e.data().p)),
			           0.5 * (convertPoint(e.destination().data().p) + convertPoint(e.data().p)));
			p.setPen(QPen(green, 4));
			p.drawLine(0.5 * (convertPoint(e.origin().data().p) + convertPoint(e.data().p)),
			           0.5 * (convertPoint(e.destination().data().p) + convertPoint(e.data().p)));
		}
	}

	for (size_t i = 0; i < dcel.faceCount(); i++) {
		InputDcel::Face f = dcel.face(i);
		if (f.data().pairedWithEdge == -1 && std::isfinite(f.data().p.h)) {
			p.setPen(QPen(black, 1));
			p.setBrush(red);
			QPolygonF face;
			f.forAllBoundaryVertices([this, &face, &f](InputDcel::Vertex v) {
				face << 0.25 * (3 * convertPoint(v.data().p) + convertPoint(f.data().p));
			});
			p.drawPolygon(face);
		}
	}

	p.setPen(QPen(black, 1));
	p.setBrush(blue);
	for (size_t i = 0; i < dcel.vertexCount(); i++) {
		InputDcel::Vertex v = dcel.vertex(i);
		if (v.data().pairedWithEdge == -1 && std::isfinite(v.data().p.h)) {
			p.drawEllipse(convertPoint(v.data().p), 4, 4);
		}
	}
}

void RiverWidget::drawGradientPairs(QPainter& p, InputDcel& dcel) const {
	QPen blue(QBrush(QColor(60, 90, 220)), 2);
	QPen red(QBrush(QColor(220, 90, 60)), 2);

	for (size_t i = 0; i < dcel.halfEdgeCount(); i++) {
		InputDcel::HalfEdge e = dcel.halfEdge(i);
		if (!std::isfinite(e.data().p.h)) {
			continue;
		}
		if (e.data().pairedWithVertex) {
			p.setPen(blue);
			drawArrow(p, convertPoint(e.origin().data().p),
			          (convertPoint(e.origin().data().p) + convertPoint(e.data().p)) * 0.5);
		}
		if (e.data().pairedWithFace && std::isfinite(e.incidentFace().data().p.h)) {
			p.setPen(red);
			drawArrow(p, convertPoint(e.data().p),
			          (convertPoint(e.data().p) + convertPoint(e.incidentFace().data().p)) * 0.5);
		}
	}
}

void RiverWidget::drawArrow(QPainter& p, const QPointF& p1, const QPointF& p2) const {
	p.drawLine(p1, p2);
	QPointF difference = p2 - p1;
	QPointF perpendicular(difference.y(), -difference.x());
	p.drawLine(p2, 0.5 * (p1 + p2) + 0.25 * perpendicular);
	p.drawLine(p2, 0.5 * (p1 + p2) - 0.25 * perpendicular);
}

void RiverWidget::drawMsComplex(QPainter& p, MsComplex& msComplex) const {
	QPen blue(QBrush(QColor(60, 90, 220)), 3);
	p.setBrush(Qt::NoBrush);
	p.setPen(blue);

	for (int i = 0; i < msComplex.halfEdgeCount(); i++) {

		MsComplex::HalfEdge e = msComplex.halfEdge(i);
		if (e.isRemoved()) {
			continue;
		}

		// only draw saddle -> minimum edges
		if (e.origin().data().type == VertexType::saddle && std::isfinite(e.origin().data().p.h)) {
			if (m_msEdgesStraight) {
				p.drawLine(convertPoint(e.origin().data().p.x,
									e.origin().data().p.y),
							convertPoint(e.destination().data().p.x,
									e.destination().data().p.y));
			} else {
				QPolygonF path;
				InputDcel::HalfEdge saddle = e.data().m_dcelPath.edges()[0];
				path << convertPoint(saddle.data().p.x, saddle.data().p.y);
				e.data().m_dcelPath.forAllVertices([this, &path, &e](InputDcel::Vertex v) {
					if (v != e.data().m_dcelPath.origin()) {
						path << convertPoint(v.data().p.x, v.data().p.y);
					}
				});
				p.drawPath(makePathRounded(path));
			}
		}
	}

	// vertex ID labels
	/*for (int i = 0; i < msComplex->vertexCount(); i++) {
		MsComplex::Vertex v = msComplex->vertex(i);
		if (v.isRemoved()) {
			continue;
		}
		QPointF drawPos = convertPoint(v.data().p);
		if (msComplex->vertexCount() < 100) {
			p.drawText(drawPos, QString("%1")
			           .arg(v.id()));
		}
	}*/
}

QPainterPath RiverWidget::makePathRounded(const QPolygonF& path) const {
	QPainterPath result;
	result.moveTo(path[0]);
	// for all internal vertices
	for (int i = 1; i < path.size() - 1; ++i) {
		QPointF p1 = path[i - 1];
		QPointF p2 = path[i];
		QPointF p3 = path[i + 1];
		double l21 = QLineF(p1, p2).length();
		double l23 = QLineF(p3, p2).length();
		double pixelLength = m_transform.m11() * 0.3;
		QPointF p21 = p2 + (p1 - p2) * (pixelLength / l21);
		QPointF p23 = p2 + (p3 - p2) * (pixelLength / l23);
		result.lineTo(p21);
		result.quadTo(p2, p23);
	}
	result.lineTo(path[path.size() - 1]);
	return result;
}

void RiverWidget::drawVertex(QPainter& p, Point p1, VertexType type) const {

	if (type == VertexType::regular) {
		return;
	}

	p.setPen(QPen(QBrush(QColor(60, 60, 60)), 0));

	switch (type) {
		case (VertexType::minimum):
			p.setBrush(QBrush(QColor(70, 50, 190)));
			break;
		case (VertexType::saddle):
			p.setBrush(QBrush(QColor(70, 190, 50)));
			break;
		case (VertexType::maximum):
			p.setBrush(QBrush(QColor(190, 70, 50)));
			break;
		case (VertexType::regular):
			p.setBrush(QBrush(QColor(150, 150, 150)));
			break;
		case (VertexType::disconnected):
			p.setBrush(QBrush(QColor(180, 180, 40)));
	}

	QPointF p2 = convertPoint(p1);
	p.drawEllipse(p2, 4, 4);
}

void RiverWidget::drawMsEdge(QPainter& p, MsComplex::HalfEdge e) const {
	if (m_msEdgesStraight) {
		p.drawLine(convertPoint(e.origin().data().p.x,
		                        e.origin().data().p.y),
		           convertPoint(e.destination().data().p.x,
		                        e.destination().data().p.y));
	} else {
		if (e.origin().data().type == VertexType::minimum) {
			e = e.twin();
		}
		for (auto l : e.data().m_dcelPath.edges()) {
			p.drawLine(convertPoint(l.origin().data().p.x,
			                        l.origin().data().p.y),
			           convertPoint(l.destination().data().p.x,
			                        l.destination().data().p.y));
		}
	}
}

void RiverWidget::drawNetwork(QPainter& p,
                              const NetworkGraph& graph) const {
	// sort edges on delta
	double deltaMax = 0;
	std::vector<NetworkGraph::Edge> edges;
	for (int i = 0; i < graph.edgeCount(); i++) {
		const NetworkGraph::Edge& e = graph.edge(i);
		if (e.delta < std::numeric_limits<double>::infinity()) {
			deltaMax = std::max(deltaMax, e.delta);
		}
		if (e.delta > m_networkDelta) {
			edges.push_back(e);
		}
	}
	std::sort(edges.begin(), edges.end(),
	          [](NetworkGraph::Edge e1, NetworkGraph::Edge e2) {
		return e1.delta < e2.delta;
	});

	for (NetworkGraph::Edge& e : edges) {
		double delta = e.delta;
		QColor color;
		/*
		// orange brown
		if (delta == std::numeric_limits<double>::infinity()) {
			color = QColor("#fec44f");
		} else if (delta > deltaMax / 1e1) {
			color = QColor("#fe9929");
		} else if (delta > deltaMax / 1e2) {
			color = QColor("#ec7014");
		} else if (delta > deltaMax / 1e3) {
			color = QColor("#cc4c02");
		} else if (delta > deltaMax / 1e4) {
			color = QColor("#993404");
		} else if (delta > deltaMax / 1e5) {
			color = QColor("#662506");
		} else {
			color = QColor("#331609");
		}*/
		// green blue
		if (delta == std::numeric_limits<double>::infinity()) {
			color = QColor("#c7e9b4");
		} else if (delta > deltaMax / 1e1) {
			color = QColor("#7fcdbb");
		} else if (delta > deltaMax / 1e2) {
			color = QColor("#41b6c4");
		} else if (delta > deltaMax / 1e3) {
			color = QColor("#1d91c0");
		} else if (delta > deltaMax / 1e4) {
			color = QColor("#225ea8");
		} else if (delta > deltaMax / 1e5) {
			color = QColor("#253494");
		} else {
			color = QColor("#081d58");
		}
		double width = 4;
		if (delta < std::numeric_limits<double>::infinity()) {
			width = std::max(1.5, 3 - 0.5 * log10(deltaMax / delta));
		}
		p.setPen(QPen(color, width));
		p.setBrush(Qt::NoBrush);
		drawGraphEdge(p, e);
	}

	/*for (int i = 0; i < graph->vertexCount(); i++) {
		drawVertex(p, (*graph)[i].p, VertexType::maximum);
	}*/
}

void RiverWidget::drawGraphEdge(QPainter& p, const NetworkGraph::Edge& e) const {
	QPolygonF path;
	for (int i = 0; i < e.path.size(); i++) {
		if (std::isfinite(e.path[i].h)) {
			path << convertPoint(e.path[i].x, e.path[i].y);
		}
	}
	if (path.size() <= 1) {
		return;
	}
	p.drawPath(makePathRounded(path));
}

QPolygonF RiverWidget::polygonForMsFace(MsComplex::Face f) const {
	QPolygonF p;

	if (m_msEdgesStraight) {
		f.forAllBoundaryEdges([&p, this](MsComplex::HalfEdge e) {
			p.append(convertPoint(e.origin().data().p));
		});
	} else {
		f.forAllBoundaryEdges([&p, this](MsComplex::HalfEdge e) {
			if (e.origin().data().type == VertexType::minimum) {
				for (int i = e.twin().data().m_dcelPath.length() - 1; i >= 0; i--) {
					InputDcel::HalfEdge l = e.twin().data().m_dcelPath.edges()[i];
					p.append(convertPoint(l.destination().data().p));
				}
			} else {
				for (auto l : e.data().m_dcelPath.edges()) {
					p.append(convertPoint(l.origin().data().p));
				}
			}
		});
	}

	return p;
}

void RiverWidget::resetTransform() {
	if (!m_riverFrame) {
		qDebug() << "Tried to reset RiverWidget transform with no river frame loaded";
		return;
	}

	double verticalStretch = m_units.m_yResolution / m_units.m_xResolution;

	int riverWidth = m_riverFrame->m_heightMap.width() - 1;
	int riverHeight = (m_riverFrame->m_heightMap.height() - 1) * verticalStretch;

	m_transform = QTransform();
	double scaleFactor = std::min(static_cast<double>(width()) / riverWidth,
	                              static_cast<double>(height()) / riverHeight);
	m_transform *= scaleFactor;

	limitMaxZoom();

	update();
}

void RiverWidget::zoomIn() {
	m_transform *= 1.5;
	limitMaxZoom();

	update();
}

void RiverWidget::zoomOut() {
	m_transform /= 1.5;

	update();
}

void RiverWidget::limitMaxZoom() {
	if (m_transform.m11() > MAX_ZOOM_FACTOR) {
		m_transform *= MAX_ZOOM_FACTOR / m_transform.m11();
	}
}

QPointF RiverWidget::convertPoint(Point p) const {
	return convertPoint(p.x, p.y);
}

QPointF RiverWidget::convertPoint(double x, double y) const {
	QPointF mapped = m_transform.map(
	            QPointF(x + 0.5, y + 0.5) -
	            QPointF(m_riverFrame->m_heightMap.width(), m_riverFrame->m_heightMap.height()) / 2);
	double stretchFactor = m_units.m_yResolution / m_units.m_xResolution;
	mapped.setY(mapped.y() * stretchFactor);
	return mapped + QPointF(width(), height()) / 2;
}

QPointF RiverWidget::inverseConvertPoint(QPointF p) const {
	double stretchFactor = m_units.m_yResolution / m_units.m_xResolution;
	QPointF toMap = p - QPointF(width(), height()) / 2;
	toMap.setY(toMap.y() / stretchFactor);
	return m_transform.inverted().map(toMap) +
	        QPointF(m_riverFrame->m_heightMap.width(), m_riverFrame->m_heightMap.height()) / 2 -
	        QPointF(0.5, 0.5);
}

int RiverWidget::hoveredMsVertex(MsComplex& msComplex) const {

	for (int i = 0; i < msComplex.vertexCount(); i++) {
		MsComplex::Vertex v = msComplex.vertex(i);
		if (v.data().p.x == mousePos.x &&
		    v.data().p.y == mousePos.y) {
			return v.id();
		}
	}

	return -1;
}

HeightMap::Coordinate* RiverWidget::hoveredBoundaryVertex() {
	HeightMap::Coordinate* result = nullptr;
	int minDistance = std::numeric_limits<int>::max();

	auto checkPath = [&](Path& p) {
		for (HeightMap::Coordinate& c : p.m_points) {
			int distance = abs(c.m_x - mousePos.x) +
			               abs(c.m_y - mousePos.y);
			if (c != p.end() && distance < minDistance) {
				result = &c;
				minDistance = distance;
			}
		};
	};

	checkPath(m_boundaryToEdit.m_source);
	checkPath(m_boundaryToEdit.m_top);
	checkPath(m_boundaryToEdit.m_sink);
	checkPath(m_boundaryToEdit.m_bottom);

	if (minDistance > 25.0 / m_transform.m11()) {
		return nullptr;
	}

	return result;
}

std::pair<Path*, int> RiverWidget::hoveredBoundaryMidpoint() {
	std::pair<Path*, int> result;
	double minDistance = std::numeric_limits<int>::max();

	auto checkPath = [&](Path& p) {
		for (int i = 0; i < p.length(); i++) {
			const HeightMap::Coordinate& c1 = p.m_points[i];
			const HeightMap::Coordinate& c2 = p.m_points[i + 1];
			double distance = abs((c1.m_x + c2.m_x) / 2.0 - mousePos.x) +
			                  abs((c1.m_y + c2.m_y) / 2.0 - mousePos.y);
			if (distance < minDistance) {
				result.first = &p;
				result.second = i;
				minDistance = distance;
			}
		};
	};

	checkPath(m_boundaryToEdit.m_source);
	checkPath(m_boundaryToEdit.m_top);
	checkPath(m_boundaryToEdit.m_sink);
	checkPath(m_boundaryToEdit.m_bottom);

	if (minDistance > 25.0 / m_transform.m11()) {
		return std::make_pair(nullptr, -1);
	}

	return result;
}

void RiverWidget::setRiverData(const std::shared_ptr<RiverData>& data) {
	m_riverData = data;
	update();
}

void RiverWidget::setRiverFrame(const std::shared_ptr<RiverFrame>& frame) {
	m_riverFrame = frame;
	updateTexture();
	update();
}

bool RiverWidget::drawBackground() const {
	return m_drawBackground;
}

void RiverWidget::setDrawBackground(bool drawBackground) {
	m_drawBackground = drawBackground;
	update();
}

QString RiverWidget::theme() const {
	return m_theme;
}

void RiverWidget::setTheme(QString theme) {
	m_theme = theme;
	initializeShaders();
	update();
}

double RiverWidget::waterLevel() const {
	return m_waterLevel;
}

void RiverWidget::setWaterLevel(double waterLevel) {
	m_waterLevel = waterLevel;
	update();
}

bool RiverWidget::showWaterPlane() const {
	return m_showWaterPlane;
}

void RiverWidget::setShowWaterPlane(bool showWaterPlane) {
	m_showWaterPlane = showWaterPlane;
	update();
}

bool RiverWidget::showShading() const {
	return m_showShading;
}

void RiverWidget::setShowShading(bool showShading) {
	m_showShading = showShading;
	update();
}

bool RiverWidget::showOutlines() const {
	return m_showOutlines;
}

void RiverWidget::setShowOutlines(bool showOutlines) {
	m_showOutlines = showOutlines;
	update();
}

int RiverWidget::contourCount() const {
	return m_contourCount;
}

void RiverWidget::setContourCount(int contourCount) {
	m_contourCount = contourCount;
	update();
}

bool RiverWidget::showElevation() const {
	return m_showElevation;
}

void RiverWidget::setShowElevation(bool showElevation) {
	m_showElevation = showElevation;
	update();
}

bool RiverWidget::showInputDcel() const {
	return m_showInputDcel;
}

void RiverWidget::setShowInputDcel(bool showInputDcel) {
	m_showInputDcel = showInputDcel;
	update();
}

bool RiverWidget::showMsComplex() const {
	return m_showMsComplex;
}

void RiverWidget::setShowMsComplex(bool showMsComplex) {
	m_showMsComplex = showMsComplex;
	update();
}

bool RiverWidget::msEdgesStraight() const {
	return m_msEdgesStraight;
}

void RiverWidget::setMsEdgesStraight(bool msEdgesStraight) {
	m_msEdgesStraight = msEdgesStraight;
	update();
}

bool RiverWidget::showStriation() const {
	return m_showStriation;
}

void RiverWidget::setShowStriation(bool showStriation) {
	m_showStriation = showStriation;
	update();
}

bool RiverWidget::showNetwork() const {
	return m_showNetwork;
}

void RiverWidget::setShowNetwork(bool showNetwork) {
	m_showNetwork = showNetwork;
	update();
}

void RiverWidget::setStriationItem(int item) {
	m_striationItem = item;
	update();
}

int RiverWidget::networkPath() const {
	return m_networkPath;
}

void RiverWidget::setNetworkPath(int path) {
	m_networkPath = path;
	update();
}

double RiverWidget::networkDelta() const {
	return m_networkDelta;
}

void RiverWidget::setNetworkDelta(double networkDelta) {
	m_networkDelta = networkDelta;
	update();
}

void RiverWidget::setUnits(Units units) {
	m_units = units;
	update();
}

void RiverWidget::startBoundaryEditMode(const Boundary& boundary) {
	m_inBoundaryEditMode = true;
	m_boundaryToEdit = boundary;
	update();
}

Boundary RiverWidget::endBoundaryEditMode() {
	m_inBoundaryEditMode = false;
	update();
	return m_boundaryToEdit;
}

bool RiverWidget::boundaryEditMode() const {
	return m_inBoundaryEditMode;
}
