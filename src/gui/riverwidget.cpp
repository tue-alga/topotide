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
#include <QOpenGLVersionProfile>
#include <QPainter>
#include <QPen>
#include <QReadLocker>
#include <QStack>
#include <QTextDocument>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <queue>
#include <vector>

#include "riverwidget.h"

RiverWidget::RiverWidget(RiverData* riverData) :
				m_riverData(riverData) {
	QSurfaceFormat format;
	setMouseTracking(true);
	format.setSamples(16);
	setFormat(format);
	resetTransform();
}

RiverWidget::~RiverWidget() {
	if (texture) {
		delete texture;
	}
}

void RiverWidget::mouseMoveEvent(QMouseEvent* event) {
	if (!m_riverData->isInitialized()) {
		return;
	}

	QPointF converted = inverseConvertPoint(event->pos());
	auto x = static_cast<int>(converted.x() + 0.5);
	auto y = static_cast<int>(converted.y() + 0.5);
	mousePos = Point(x, y, 0);
	mouseInBounds = m_riverData->heightMap.isInBounds(x, y);
	if (mouseInBounds) {
		hoveredCoordinateChanged(x, y,
		                         (float) m_riverData->heightMap.elevationAt(x, y));
	} else {
		emit mouseLeft();
	}

	if (m_dragging) {
		if (m_inBoundaryEditMode && m_draggedVertex != nullptr) {
			m_draggedVertex->m_x = mousePos.x;
			m_draggedVertex->m_y = mousePos.y;
			m_riverData->boundary.ensureConnection();
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
	if (!m_riverData->isInitialized()) {
		return;
	}

	m_dragging = true;

	if (m_inBoundaryEditMode) {
		m_draggedVertex = hoveredBoundaryVertex(m_riverData->boundary);
		if (m_draggedVertex == nullptr) {
			std::pair<Path*, int> newMidpoint =
			        hoveredBoundaryMidpoint(m_riverData->boundary);
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
	if (!m_riverData->isInitialized()) {
		return;
	}

	m_dragging = false;
	setCursor(Qt::ArrowCursor);
	m_draggedVertex = nullptr;
}

void RiverWidget::leaveEvent(QEvent* event) {
	if (!m_riverData->isInitialized()) {
		return;
	}

	mouseInBounds = false;
	emit mouseLeft();
	update();
}

void RiverWidget::wheelEvent(QWheelEvent* event) {
	if (!m_riverData->isInitialized()) {
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

	QPointF mousePos = event->pos();
	mousePos -= QPointF(width() / 2, height() / 2);
	QTransform transform;
	transform.translate(mousePos.x(), mousePos.y());
	transform.scale(factor, factor);
	transform.translate(-mousePos.x(), -mousePos.y());
	m_transform *= transform;

	update();
}

void RiverWidget::initializeGL() {
	gl = QOpenGLContext::currentContext()->
	     versionFunctions<QOpenGLFunctions_3_0>();
	//gl = context()->versionFunctions(vp);
	gl->initializeOpenGLFunctions();

	// initialize texture
	if (m_riverData->isInitialized()) {
		updateTexture();
	}

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
	texture = new QOpenGLTexture(m_riverData->image.mirrored());
	texture->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void RiverWidget::paintGL() {

	QPainter p(this);

	if (!m_riverData->isInitialized()) {
		drawUninitializedMessage(p);
		return;
	}

	// initialize shader variables
	program->bind();
	program->setUniformValue("waterLevel", (float) m_waterLevel / (256 * 256 * 256));
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
	transform.scale(1.0 / (m_riverData->image.width()),
	                -1.0 / (m_riverData->image.height()));
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
		drawBoundaryEditable(p, m_riverData->boundary);

	} else {
		drawBoundary(p, m_riverData->boundaryRasterized);

		if (m_showInputDcel &&
		        m_riverData->inputDcel != nullptr) {
			QReadLocker lock(&(m_riverData->inputDcelLock));
			drawInputDcel(p, m_riverData->inputDcel);
		}

		if (m_showStriation &&
		        m_riverData->msComplex != nullptr &&
		        m_riverData->striation != nullptr) {
			drawStriation(p, m_riverData->msComplex, m_riverData->striation);
		}

		if (m_showMsComplex &&
		        m_riverData->msComplex != nullptr) {
			QReadLocker locker(&(m_riverData->msComplexLock));
			drawMsComplex(p, m_riverData->msComplex);
		}

		if (m_showNetwork &&
		        m_riverData->msComplex != nullptr &&
		        m_riverData->networkGraph != nullptr) {
			drawNetwork(p, m_riverData->network, m_riverData->networkGraph);
		}

		if (mouseInBounds) {
			drawVertex(p, mousePos, VertexType::disconnected);
		}
	}
}

void RiverWidget::drawUninitializedMessage(QPainter& p) const {
	p.setBrush(palette().background());
	p.setPen(Qt::NoPen);
	p.drawRect(rect());

	p.setBrush(Qt::NoBrush);
	p.setPen(QPen(palette().text().color()));
	QTextDocument text;
	text.setHtml("<div style='text-align: center;'>"
	             "<h2>No river loaded</h2>"
	             "<p>To open a river, use <i>File &gt; Open...</i> or "
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
        QPainter& p, Boundary& boundary) const {

	p.setPen(QPen(QBrush(QColor("black")), 2));
	drawPath(p, boundary.m_top);
	drawPath(p, boundary.m_bottom);

	p.setPen(QPen(QBrush(QColor(13, 110, 190)), 3));
	drawPath(p, boundary.m_source);
	p.setPen(QPen(QBrush(QColor(163, 99, 4)), 3));
	drawPath(p, boundary.m_sink);
}

void RiverWidget::drawBoundaryEditable(
        QPainter& p, Boundary& boundary) const {

	HeightMap::Coordinate* hovered =
	        hoveredBoundaryVertex(m_riverData->boundary);
	if (hovered != nullptr) {
		drawBoundaryVertex(p, *hovered, true);
	}

	drawBoundary(p, boundary);

	if (hovered == nullptr) {
		std::pair<Path*, int> mid =
		        hoveredBoundaryMidpoint(m_riverData->boundary);
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

void RiverWidget::drawBoundaryVertices(QPainter& p, Path& path) const {
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
        QPainter& p, Path& path) const {

	std::vector<HeightMap::Coordinate> points = path.m_points;
	for (int i = 0; i < (int) (points.size()) - 1; i++) {
		HeightMap::Coordinate p1 = points[i];
		HeightMap::Coordinate p2 = points[i + 1];
		p.drawLine(convertPoint(p1.m_x, p1.m_y),
		           convertPoint(p2.m_x, p2.m_y));
	}
}

void RiverWidget::drawInputDcel(QPainter& p, InputDcel* dcel) const {

	QPen opaque(QBrush(QColor(60, 60, 60)), 0);
	QPen transparent(QBrush(QColor(60, 60, 60, 80)), 0);

	for (int i = 0; i < dcel->halfEdgeCount(); i++) {

		InputDcel::HalfEdge e = dcel->halfEdge(i);

		// only draw edges in one direction
		if (e.origin().id() > e.destination().id()) {
			continue;
		}

		if (e.origin().id() < 3) {
			p.setPen(transparent);
		} else {
			p.setPen(opaque);
		}

		p.drawLine(convertPoint(e.origin().data().p.x,
							e.origin().data().p.y),
					convertPoint(e.destination().data().p.x,
							e.destination().data().p.y));
	}
	p.setPen(Qt::NoPen);

	for (int i = 0; i < dcel->vertexCount(); i++) {
		InputDcel::Vertex v = dcel->vertex(i);
		drawVertex(p, v.data().p, v.data().type);
	}

	p.setBrush(Qt::NoBrush);
}

void RiverWidget::drawMsComplex(QPainter& p, MsComplex* msComplex) const {

	p.setBrush(Qt::NoBrush);
	p.setPen(QPen(QColor("black"), 1));

	for (int i = 0; i < msComplex->halfEdgeCount(); i++) {

		MsComplex::HalfEdge e = msComplex->halfEdge(i);
		if (e.isRemoved()) {
			continue;
		}

		// only draw saddle -> minimum edges
		if (e.origin().data().type == VertexType::saddle) {
			if (m_msEdgesStraight) {
				p.drawLine(convertPoint(e.origin().data().p.x,
									e.origin().data().p.y),
							convertPoint(e.destination().data().p.x,
									e.destination().data().p.y));
			} else {
				for (auto l : e.data().m_dcelPath.edges()) {
					p.drawLine(convertPoint(l.origin().data().p.x,
										l.origin().data().p.y),
								convertPoint(l.destination().data().p.x,
										l.destination().data().p.y));
				}
			}
		}
	}

	for (int i = 0; i < msComplex->vertexCount(); i++) {
		MsComplex::Vertex v = msComplex->vertex(i);
		if (v.isRemoved()) {
			continue;
		}
		drawVertex(p, v.data().p, v.data().type);
	}

	for (int i = 0; i < msComplex->vertexCount(); i++) {
		MsComplex::Vertex v = msComplex->vertex(i);
		if (v.isRemoved()) {
			continue;
		}
		QPointF drawPos = convertPoint(v.data().p);
		drawPos += QPointF(3, -3);
		if (msComplex->vertexCount() < 100) {
			p.drawText(drawPos, QString("%1")
			           .arg(v.id()));
		}
	}

	for (int i = 0; i < msComplex->faceCount(); i++) {
		MsComplex::Face f = msComplex->face(i);
		if (f.isRemoved()) {
			continue;
		}
		Point maxPos = f.data().maximum.data().p;
		if (maxPos.h == std::numeric_limits<double>::infinity()) {
			continue;
		}

		if (msComplex->vertexCount() < 100) {
			p.drawText(convertPoint(maxPos) + QPointF(3, -3),
			           QString("%1 (%2)")
			               .arg(f.id())
			               .arg(f.data().volumeAbove(m_waterLevel)
			                    / (256 * 256 * 256) * 100, 0, 'f', 1, '0'));
		}
		drawVertex(p, maxPos, VertexType::maximum);
	}
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
	p.drawEllipse(p2.x() - 4, p2.y() - 4, 8, 8);
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

void RiverWidget::drawStriation(QPainter& p, MsComplex* msc,
                                Striation* striation) const {

	Striation::Item& item = striation->item(m_striationItem);

	p.setPen(Qt::NoPen);
	p.setBrush(QBrush(QColor("#eecc66")));
	int topItem = item.m_topItem;
	if (topItem != -1) {
		striation->forItemsInOrder(topItem,
		                           [&](Striation::Item& item, int i) {
			QPolygonF polygon = polygonForMsFace(msc->face(item.m_face));
			p.drawPolygon(polygon);
		});
	}
	p.setBrush(QBrush(QColor("#88dd77")));
	int bottomItem = item.m_bottomItem;
	if (bottomItem != -1) {
		striation->forItemsInOrder(bottomItem,
		                           [&](Striation::Item& item, int i) {
			QPolygonF polygon = polygonForMsFace(msc->face(item.m_face));
			p.drawPolygon(polygon);
		});
	}

	p.setBrush(QBrush(QColor("#5577cc")));
	QPolygonF polygon = polygonForMsFace(msc->face(item.m_face));
	p.drawPolygon(polygon);

	p.setPen(QPen(QBrush(QColor(180, 80, 30)), 3));
	p.setBrush(Qt::NoBrush);
	std::vector<int> topPath = item.m_topCarvePath;
	for (auto e : topPath) {
		drawMsEdge(p, msc->halfEdge(e));
	}
	p.setPen(QPen(QBrush(QColor(80, 30, 180)), 3));
	std::vector<int> bottomPath = item.m_bottomCarvePath;
	for (auto e : bottomPath) {
		drawMsEdge(p, msc->halfEdge(e));
	}

	for (auto v : item.m_topVertices) {
		drawVertex(p, msc->vertex(v).data().p, VertexType::maximum);
	}
	for (auto v : item.m_bottomVertices) {
		drawVertex(p, msc->vertex(v).data().p, VertexType::minimum);
	}
}

void RiverWidget::drawNetwork(QPainter& p,
                              Network* network,
                              NetworkGraph* graph) const {

	/*p.setPen(QPen(QColor("white"), 3));
	for (int i = 0; i < graph->edgeCount(); i++) {
		drawGraphEdge(p, graph->edge(i));
	}*/

	// sort edges on delta
	double deltaMax = 0;
	std::vector<NetworkGraph::Edge> edges;
	for (int i = 0; i < graph->edgeCount(); i++) {
		NetworkGraph::Edge& e = graph->edge(i);
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
		drawGraphEdge(p, e);
	}

	if (network != nullptr) {
		if (m_networkPath < network->paths().size()) {
			p.setPen(QPen(QColor(240, 130, 60), 2));
			for (auto edge : network->paths()[m_networkPath].edges()) {
				drawMsEdge(p, edge.edge);
			}
		}
	}

	/*for (int i = 0; i < graph->vertexCount(); i++) {
		drawVertex(p, (*graph)[i].p, VertexType::maximum);
	}*/
}

void RiverWidget::drawGraphEdge(QPainter& p, NetworkGraph::Edge& e) const {
	for (int i = 0; i < e.path.size() - 1; i++) {
		if (e.path[i].h != -std::numeric_limits<double>::infinity() &&
		        e.path[i + 1].h != -std::numeric_limits<double>::infinity()) {
			p.drawLine(convertPoint(e.path[i].x,
			                        e.path[i].y),
			           convertPoint(e.path[i + 1].x,
			                        e.path[i + 1].y));
		}
	}
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

	double verticalStretch = m_units.m_yResolution / m_units.m_xResolution;

	int riverWidth = m_riverData->image.width() - 1;
	int riverHeight = (m_riverData->image.height() - 1) * verticalStretch;

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
	            QPointF(m_riverData->image.width(), m_riverData->image.height()) / 2);
	double stretchFactor = m_units.m_yResolution / m_units.m_xResolution;
	mapped.setY(mapped.y() * stretchFactor);
	return mapped + QPointF(width(), height()) / 2;
}

QPointF RiverWidget::inverseConvertPoint(QPointF p) const {
	double stretchFactor = m_units.m_yResolution / m_units.m_xResolution;
	QPointF toMap = p - QPointF(width(), height()) / 2;
	toMap.setY(toMap.y() / stretchFactor);
	return m_transform.inverted().map(toMap) +
	        QPointF(m_riverData->image.width(), m_riverData->image.height()) / 2 -
	        QPointF(0.5, 0.5);
}

int RiverWidget::hoveredMsVertex(MsComplex* msComplex) const {

	for (int i = 0; i < msComplex->vertexCount(); i++) {
		MsComplex::Vertex v = msComplex->vertex(i);
		if (v.data().p.x == mousePos.x &&
		    v.data().p.y == mousePos.y) {
			return v.id();
		}
	}

	return -1;
}

HeightMap::Coordinate* RiverWidget::hoveredBoundaryVertex(
        Boundary& boundary) const {
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

	checkPath(boundary.m_source);
	checkPath(boundary.m_top);
	checkPath(boundary.m_sink);
	checkPath(boundary.m_bottom);

	if (minDistance > 25.0 / m_transform.m11()) {
		return nullptr;
	}

	return result;
}

std::pair<Path*, int> RiverWidget::hoveredBoundaryMidpoint(
        Boundary& boundary) const {
	std::pair<Path*, int> result;
	double minDistance = std::numeric_limits<int>::max();

	auto checkPath = [&](Path& p) {
		for (int i = 0; i < p.length(); i++) {
			HeightMap::Coordinate& c1 = p.m_points[i];
			HeightMap::Coordinate& c2 = p.m_points[i + 1];
			double distance = abs((c1.m_x + c2.m_x) / 2.0 - mousePos.x) +
			                  abs((c1.m_y + c2.m_y) / 2.0 - mousePos.y);
			if (distance < minDistance) {
				result.first = &p;
				result.second = i;
				minDistance = distance;
			}
		};
	};

	checkPath(boundary.m_source);
	checkPath(boundary.m_top);
	checkPath(boundary.m_sink);
	checkPath(boundary.m_bottom);

	if (minDistance > 25.0 / m_transform.m11()) {
		return std::make_pair(nullptr, -1);
	}

	return result;
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

int RiverWidget::waterLevel() const {
	return m_waterLevel;
}

void RiverWidget::setWaterLevel(int waterLevel) {
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

void RiverWidget::setBoundaryEditMode(bool inBoundaryEditMode) {
	m_inBoundaryEditMode = inBoundaryEditMode;
	update();
}

void RiverWidget::startBoundaryEditMode() {
	setBoundaryEditMode(true);
}

void RiverWidget::endBoundaryEditMode() {
	setBoundaryEditMode(false);
}

bool RiverWidget::boundaryEditMode() const {
	return m_inBoundaryEditMode;
}
