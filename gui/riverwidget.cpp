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
#include <optional>
#include <qopengltexture.h>
#include <vector>

#include "riverwidget.h"

#include "boundary.h"
#include "boundarycreator.h"

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
		hoveredCoordinateChanged(HeightMap::Coordinate{x, y},
		                         (float) m_riverFrame->m_heightMap.elevationAt(x, y));
	} else {
		emit mouseLeft();
	}

	if (m_dragging) {
		if (m_mode == Mode::EDIT_BOUNDARY && m_draggedVertex.has_value()) {
			HeightMap::Coordinate newPosition{static_cast<int>(mousePos.x),
			                                  static_cast<int>(mousePos.y)};
			newPosition = m_riverFrame->m_heightMap.clampToBounds(newPosition);
			m_boundaryToEdit.movePoint(*m_draggedVertex, newPosition);
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

	if (m_mode == Mode::GENERATE_BOUNDARY_SEED) {
		if (mouseInBounds) {
			HeightMap::Coordinate seed(static_cast<int>(mousePos.x), static_cast<int>(mousePos.y));
			m_boundaryCreator->setSeed(seed);
			if (m_boundaryCreator->getPath()) {
				m_boundaryToEdit = Boundary(*m_boundaryCreator->getPath());
				m_mode = Mode::SET_SOURCE_START;
				m_boundaryToEdit.removePermeableRegions();
				m_boundaryToEdit.addPermeableRegion(Boundary::Region{0, 0});
				emit statusMessage("Click to indicate where on the boundary the source starts and "
				                   "ends (in clockwise order)");
			} else {
				m_mode = Mode::NORMAL;
				emit boundaryEdited(std::nullopt);
				emit statusMessage("");
			}
		}

	} else if (m_mode == Mode::SET_SOURCE_START || m_mode == Mode::SET_SOURCE_END ||
	           m_mode == Mode::SET_SINK_START || m_mode == Mode::SET_SINK_END) {
		switch (m_mode) {
			case Mode::SET_SOURCE_START:
				m_mode = Mode::SET_SOURCE_END;
				break;
			case Mode::SET_SOURCE_END:
				m_mode = Mode::SET_SINK_START;
				m_boundaryToEdit.addPermeableRegion(Boundary::Region{0, 0});
				emit statusMessage("Click to indicate where on the boundary the sink starts and ends (in clockwise order)");
				break;
			case Mode::SET_SINK_START:
				m_mode = Mode::SET_SINK_END;
				break;
			case Mode::SET_SINK_END:
				m_mode = Mode::NORMAL;
				emit statusMessage("");
				emit boundaryEdited(m_boundaryToEdit);
				break;
		}

	} else if (m_mode == Mode::EDIT_BOUNDARY) {
		const Path& path = m_boundaryToEdit.path();
		m_draggedVertex = hoveredPathVertex(path);
		if (!m_draggedVertex.has_value()) {
			std::optional<int> newMidpoint = hoveredBoundaryMidpoint();
			if (newMidpoint.has_value()) {
				HeightMap::Coordinate c1 = path.m_points[*newMidpoint];
				HeightMap::Coordinate c2 = path.m_points[*newMidpoint + 1];
				HeightMap::Coordinate midP((c1.m_x + c2.m_x) / 2,
				                           (c1.m_y + c2.m_y) / 2);
				m_boundaryToEdit.insertPoint(*newMidpoint + 1, midP);
				m_draggedVertex = *newMidpoint + 1;
			}
		}
	}

	setCursor(Qt::ClosedHandCursor);
	m_previousMousePos = event->pos();

	update();
}

void RiverWidget::mouseReleaseEvent(QMouseEvent*) {
	if (!m_riverFrame) {
		return;
	}

	m_dragging = false;
	setCursor(Qt::ArrowCursor);
	m_draggedVertex = std::nullopt;
}

void RiverWidget::leaveEvent(QEvent*) {
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
	updateElevationRampTexture();
	initializeShaders();
}

void RiverWidget::initializeShaders() {

	// compile shaders and program
	QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex);
	vshader->compileSourceFile(":/res/shader/river.vert");

	QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment);
	fshader->compileSourceFile(":/res/shader/river.frag");

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
	double min = m_riverData->minimumElevation();
	double max = m_riverData->maximumElevation();
	QImage image(heightMap.width(), heightMap.height(), QImage::Format::Format_ARGB32);
	for (int y = 0; y < heightMap.height(); ++y) {
		for (int x = 0; x < heightMap.width(); ++x) {
			double elevation = heightMap.elevationAt(x, y);
			if (std::isnan(elevation)) {
				image.setPixel(x, y, 0x00000000);
			} else {
				unsigned int value = 0xff000000 + 0xffffff * (elevation - min) / (max - min);
				image.setPixel(x, y, value);
			}
		}
	}
	texture = new QOpenGLTexture(image);
	texture->setWrapMode(QOpenGLTexture::ClampToEdge);

	QImage maskImage(heightMap.width(), heightMap.height(), QImage::Format::Format_ARGB32);
	maskImage.fill(QColor{"black"});
	contourMaskTexture = new QOpenGLTexture(maskImage);
	contourMaskTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
	contourMaskTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
}

void RiverWidget::updateElevationRampTexture() {
	elevationRampTexture = new QOpenGLTexture(m_colorRamp.toImage());
	elevationRampTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
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
	program->setUniformValue(
	    "contourLevel",
	    (float) ((m_contourLevel - m_riverData->minimumElevation()) /
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
	if (elevationRampTexture != nullptr) {
		elevationRampTexture->bind(1);
		program->setUniformValue("elevationRampTex", 1);
	}
	if (contourMaskTexture != nullptr) {
		contourMaskTexture->bind(2);
		program->setUniformValue("contourMaskTex", 2);
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

	if (m_mode == Mode::GENERATE_BOUNDARY_SEED) {
		if (mouseInBounds) {
			HeightMap::Coordinate seed(static_cast<int>(mousePos.x), static_cast<int>(mousePos.y));
			m_boundaryCreator->setSeed(seed);
			std::optional<Path> path = m_boundaryCreator->getPath();
			if (path) {
				p.setPen(QPen(QBrush(EDITABLE_BOUNDARY_COLOR), 2));
				drawPath(p, *path);
			}
		}

	} else if (m_mode == Mode::SET_SOURCE_START || m_mode == Mode::SET_SOURCE_END ||
	           m_mode == Mode::SET_SINK_START || m_mode == Mode::SET_SINK_END) {
		std::optional<int> hovered = hoveredNonPermeableBoundaryVertex(m_boundaryToEdit);
		if (hovered) {
			if (m_mode == Mode::SET_SOURCE_START || m_mode == Mode::SET_SINK_START) {
				m_boundaryToEdit.setLastPermeableRegion(Boundary::Region{*hovered, *hovered});
			} else {
				m_boundaryToEdit.setLastPermeableRegion(Boundary::Region{
				    m_boundaryToEdit.lastPermeableRegion().m_start, *hovered});
			}
		}
		drawBoundaryCreatorState(p);

	} else if (m_mode == Mode::EDIT_BOUNDARY) {
		drawBoundaryEditable(p, m_boundaryToEdit);

	} else {
		if (m_riverData) {
			p.setPen(QPen(QBrush(BOUNDARY_COLOR), 2));
			drawBoundary(p, m_riverData->boundaryRasterized());
		}

		if (m_showInputDcel && m_riverFrame->m_inputDcel != nullptr) {
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
			if (m_showSimplified && m_riverFrame->m_simplifiedInputDcel != nullptr) {
				QReadLocker lock(&(m_riverFrame->m_simplifiedInputDcelLock));
				drawGradientPairs(p, *m_riverFrame->m_simplifiedInputDcel);
			} else {
#endif
				QReadLocker lock(&(m_riverFrame->m_inputDcelLock));
				drawGradientPairs(p, *m_riverFrame->m_inputDcel);
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
			}
#endif
		}

		if (m_showMsComplex && m_riverFrame->m_msComplex != nullptr) {
			QReadLocker locker(&(m_riverFrame->m_msComplexLock));
			drawMsComplex(p, *m_riverFrame->m_msComplex);
		}

		if (m_showCriticalPoints && m_riverFrame->m_inputDcel != nullptr) {
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
			if (m_showSimplified && m_riverFrame->m_simplifiedInputDcel != nullptr) {
				QReadLocker lock(&(m_riverFrame->m_simplifiedInputDcelLock));
				drawCriticalPoints(p, *m_riverFrame->m_simplifiedInputDcel);
			} else {
#endif
				QReadLocker lock(&(m_riverFrame->m_inputDcelLock));
				drawCriticalPoints(p, *m_riverFrame->m_inputDcel);
#ifdef EXPERIMENTAL_FINGERS_SUPPORT
			}
#endif
		}

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
		if (m_showSpurs && m_riverFrame->m_inputDcel != nullptr) {
			if (m_showSimplified && m_riverFrame->m_simplifiedInputDcel != nullptr) {
				QReadLocker lock(&(m_riverFrame->m_simplifiedInputDcelLock));
				drawSpurs(p, *m_riverFrame->m_simplifiedInputDcel);
			}
		}

		if (m_showFingers &&
		        m_riverFrame->m_fingers != nullptr) {
			drawFingers(p, *m_riverFrame->m_fingers);
		}
#endif

		if (m_showNetwork && m_riverFrame->m_msComplex != nullptr &&
		    m_riverFrame->m_networkGraph != nullptr) {
			drawNetwork(p, *m_riverFrame->m_networkGraph);
		}

		if (m_pointToHighlight) {
			QPointF p2 = convertPoint(*m_pointToHighlight);
			p.setPen(Qt::NoPen);
			p.setBrush(QColor{"#dd4422"});
			p.drawEllipse(p2, 4, 4);
		}

		if (mouseInBounds) {
			QPointF p2 = convertPoint(mousePos);
			p.setPen(Qt::NoPen);
			p.setBrush(QColor{"#238b45"});
			p.drawEllipse(p2, 4, 4);
		}

		for (const InputDcel::HalfEdge& edge : m_edgesToDraw) {
			p.setPen(QPen(QColor{"#123456"}, 2));
			p.drawLine(convertPoint(edge.origin().data().p),
			           convertPoint(edge.destination().data().p));
		}
		for (const Point& point : m_pointsToDraw) {
			drawVertex(p, point, VertexType::saddle);
		}
	}
}

void RiverWidget::drawUninitializedMessage(QPainter& p) const {
	p.setBrush(palette().window());
	p.setPen(Qt::NoPen);
	p.drawRect(rect());

	p.setOpacity(0.2);
	p.drawPixmap(
	    QRect(rect().bottomRight() - QPoint(370, 345), rect().bottomRight() + QPoint(-10, 15)),
	    m_topoTideBackgroundImage);
	p.setOpacity(1);

	p.setBrush(Qt::NoBrush);
	p.setPen(QPen(palette().text().color()));
	QTextDocument text;
	text.setHtml("<div style='text-align: center;'>"
	             "<h2>No elevation data loaded</h2>"
	             "<p>To open a DEM, use <i>File &gt; Open DEM</i> or "
	             "drag-and-drop a file containing elevation data onto this window.</p>");
	text.setTextWidth(rect().width());
	text.drawContents(&p, rect());
}

void RiverWidget::drawGrid(QPainter& p) const {
	double opacity = (m_transform.m11() - 10.0) / 50.0;
	if (opacity > 1) {
		opacity = 1;
	}
	int a = static_cast<int>(opacity * 128);
	p.setPen(QPen(QBrush(QColor(0, 0, 0, a)), 1));

	// vertical lines
	double yTop = std::max(0.0, convertPoint(0, 0).y());
	double yBottom =
	    std::min(static_cast<double>(height()), convertPoint(0, m_riverData->height() - 1).y());
	int firstLineX = std::max(0, static_cast<int>(inverseConvertPoint(rect().topLeft()).x()));
	int lastLineX = std::min(m_riverData->width() - 1,
	                         static_cast<int>(inverseConvertPoint(rect().bottomRight()).x()));
	for (int x = firstLineX; x <= lastLineX; x++) {
		double xF = convertPoint(x, 0).x();
		p.drawLine(QPointF(xF, yTop), QPointF(xF, yBottom));
	}

	// horizontal lines
	double xLeft = std::max(0.0, convertPoint(0, 0).x());
	double xRight =
	    std::min(static_cast<double>(width()), convertPoint(m_riverData->width() - 1, 0).x());
	int firstLineY = std::max(0, static_cast<int>(inverseConvertPoint(rect().topLeft()).y()));
	int lastLineY = std::min(m_riverData->height() - 1,
	                         static_cast<int>(inverseConvertPoint(rect().bottomRight()).y()));
	for (int y = firstLineY; y <= lastLineY; y++) {
		double yF = convertPoint(0, y).y();
		p.drawLine(QPointF(xLeft, yF), QPointF(xRight, yF));
	}
}

void RiverWidget::drawBoundary(QPainter& p, const Boundary& boundary) const {
	drawPath(p, boundary.path());
	for (const Boundary::Region& region : boundary.permeableRegions()) {
		p.setPen(QPen(QBrush(PERMEABLE_BOUNDARY_COLOR), 3));
		drawPermeableRegion(p, boundary.path(), region);
	}
}

void RiverWidget::drawBoundaryEditable(QPainter& p, Boundary& boundary) {
	p.setPen(QPen(QBrush(EDITABLE_BOUNDARY_COLOR), 2));
	drawBoundary(p, boundary);

	if (m_draggedVertex.has_value()) {
		drawBoundaryVertex(p, boundary.path().m_points[*m_draggedVertex], true);
	} else {
		const std::optional<int> hovered = hoveredPathVertex(boundary.path());
		if (hovered.has_value()) {
			drawBoundaryVertex(p, boundary.path().m_points[*hovered], true);
		} else {
			std::optional<int> mid = hoveredBoundaryMidpoint();
			if (mid.has_value()) {
				HeightMap::Coordinate c1 = boundary.path().m_points[*mid];
				HeightMap::Coordinate c2 = boundary.path().m_points[*mid + 1];
				Point midP((c1.m_x + c2.m_x) / 2.0, (c1.m_y + c2.m_y) / 2.0, 0);
				p.setPen(QPen(QBrush(QColor(60, 60, 60, 128)), 0));
				p.setBrush(QBrush(QColor(255, 255, 255, 128)));
				QPointF p2 = convertPoint(midP);
				p.drawEllipse(p2.x() - 4, p2.y() - 4, 8, 8);
			}
		}
	}

	drawBoundaryVertices(p, boundary.path());
}

void RiverWidget::drawBoundaryVertices(QPainter& p, const Path& path) const {
	for (HeightMap::Coordinate c : path.m_points) {
		drawBoundaryVertex(p, c);
	}
}

void RiverWidget::drawBoundaryVertex(QPainter& p, HeightMap::Coordinate c, bool outlined) const {

	if (outlined) {
		p.setPen(QPen(QBrush(QColor(80, 130, 255)), 8));
	} else {
		p.setPen(QPen(QBrush(QColor(60, 60, 60)), 0));
	}
	p.setBrush(QBrush(QColor(255, 255, 255)));

	QPointF p2 = convertPoint(Point(c.m_x, c.m_y, 0));
	p.drawEllipse(p2.x() - 4, p2.y() - 4, 8, 8);
}

void RiverWidget::drawBoundaryCreatorState(QPainter& p) {
	// the entire boundary path
	Path path = m_boundaryToEdit.path();
	p.setPen(QPen(QBrush(EDITABLE_BOUNDARY_COLOR), 2));
	drawPath(p, path);

	// permeable regions
	for (const Boundary::Region& region : m_boundaryToEdit.permeableRegions()) {
		p.setPen(QPen(QBrush(PERMEABLE_BOUNDARY_COLOR), 3));
		drawPermeableRegion(p, path, region);

		p.setPen(QPen(QBrush(QColor(60, 60, 60)), 0));
		p.setBrush(PERMEABLE_BOUNDARY_COLOR);
		QPointF p2 = convertPoint(path.m_points[region.m_start].m_x,
		                          path.m_points[region.m_start].m_y);
		p.drawEllipse(p2.x() - 4, p2.y() - 4, 8, 8);
		p2 = convertPoint(path.m_points[region.m_end].m_x, path.m_points[region.m_end].m_y);
		p.drawEllipse(p2.x() - 4, p2.y() - 4, 8, 8);
	}
}

void RiverWidget::drawPath(QPainter& p, const Path& path) const {
	std::vector<HeightMap::Coordinate> points = path.m_points;
	for (int i = 0; i < (int) (points.size()) - 1; i++) {
		HeightMap::Coordinate p1 = points[i];
		HeightMap::Coordinate p2 = points[i + 1];
		p.drawLine(convertPoint(p1.m_x, p1.m_y), convertPoint(p2.m_x, p2.m_y));
	}
}

void RiverWidget::drawPermeableRegion(QPainter& p, const Path& path,
                                      const Boundary::Region& region) const {
	std::vector<HeightMap::Coordinate> points = path.m_points;
	for (int i = region.m_start; i != region.m_end; i = (i + 1) % path.length()) {
		HeightMap::Coordinate p1 = points[i];
		HeightMap::Coordinate p2 = points[i + 1];
		p.drawLine(convertPoint(p1.m_x, p1.m_y), convertPoint(p2.m_x, p2.m_y));
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
			continue; // avoid drawing the same edge twice
		}
		if (dcel.isCritical(e) && inBounds(e.data().p)) {
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
		if (dcel.isCritical(f) && inBounds(f.data().p)) {
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
		if (dcel.isCritical(v) && inBounds(v.data().p)) {
			p.drawEllipse(convertPoint(v.data().p), 4, 4);
		}
	}
}

void RiverWidget::drawGradientPairs(QPainter& p, InputDcel& dcel) const {
	QPen blue(QBrush(QColor(60, 90, 220)), 2);
	QPen red(QBrush(QColor(220, 90, 60)), 2);

	QFont font(p.font());
	font.setPixelSize(10);
	p.setFont(font);

	for (size_t i = 0; i < dcel.halfEdgeCount(); i++) {
		InputDcel::HalfEdge e = dcel.halfEdge(i);
		if (!inBounds(e.data().p)) {
			continue;
		}
		if (e.data().pairedWithVertex) {
			p.setPen(blue);
			if (m_drawGradientPairsAsTrees) {
				p.drawLine(convertPoint(e.origin().data().p), convertPoint(e.destination().data().p));
			} else {
				drawArrow(p, convertPoint(e.origin().data().p),
						(convertPoint(e.origin().data().p) + convertPoint(e.data().p)) * 0.5);
			}
		}
		if (e.data().pairedWithFace && inBounds(e.incidentFace().data().p)) {
			p.setPen(red);
			if (m_drawGradientPairsAsTrees) {
				if (std::isfinite(e.incidentFace().data().p.h) && std::isfinite(e.oppositeFace().data().p.h)) {
					p.drawLine(convertPoint(e.incidentFace().data().p),
					           convertPoint(e.twin().incidentFace().data().p));
					if (m_transform.m11() > 50) {
						QPointF center = convertPoint(e.data().p);
						QPointF origin = convertPoint(e.origin().data().p);
						QPointF destination = convertPoint(e.destination().data().p);
						QPointF vector = destination - origin;
						QPointF perpendicular(-vector.y(), vector.x());
						double length = std::hypot(perpendicular.x(), perpendicular.y());
						QPointF offset = perpendicular * (10.0 / length);
						if (rect().contains(center.toPoint())) {
							PiecewiseLinearFunction& volume = e.data().volumeAbove;
							p.save();
							p.translate(center + offset);
							p.rotate(180 * std::atan2(-vector.y(), -vector.x()) / M_PI);
							p.drawText(QRectF(QPointF(-100, -100), QSizeF(200, 200)),
							           Qt::AlignCenter,
							           QString("%1 m").arg(volume.heightForVolume(m_networkDelta)));
							p.restore();
							p.save();
							p.translate(center - offset);
							p.rotate(180 * std::atan2(vector.y(), vector.x()) / M_PI);
							PiecewiseLinearFunction twinVolume = e.twin().data().volumeAbove;
							p.drawText(QRectF(QPointF(-100, -100), QSizeF(200, 200)),
							           Qt::AlignCenter,
							           QString("%1 m").arg(twinVolume.heightForVolume(m_networkDelta)));
							p.restore();
						}
					}
				}
			} else {
				drawArrow(p, convertPoint(e.data().p),
						(convertPoint(e.data().p) + convertPoint(e.incidentFace().data().p)) * 0.5);
			}
		}
	}

	if (m_drawGradientPairsAsTrees) {
		p.setPen(blue);
		for (size_t i = 0; i < dcel.vertexCount(); i++) {
			InputDcel::Vertex v = dcel.vertex(i);
			if (std::isfinite(v.data().p.h) && dcel.isBlueLeaf(v)) {
				p.drawEllipse(convertPoint(v.data().p), 2, 2);
			}
		}
		p.setPen(red);
		for (size_t i = 0; i < dcel.faceCount(); i++) {
			InputDcel::Face f = dcel.face(i);
			if (std::isfinite(f.data().p.h) && dcel.isRedLeaf(f)) {
				p.drawEllipse(convertPoint(f.data().p), 2, 2);
			}
		}
	}
}

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
void RiverWidget::drawSpurs(QPainter& p, InputDcel& dcel) const {
	std::vector<QColor> colors = {
	    // ColorBrewer colors, qualitative Set1
	    QColor{228, 26, 28}, QColor{55, 126, 184},  QColor{77, 175, 74}, QColor{152, 78, 163},
	    QColor{255, 127, 0}, QColor{153, 153, 153}, QColor{166, 86, 40}, QColor{247, 129, 191},
	};
	
	// background shading
	p.setPen(Qt::NoPen);
	p.setOpacity(0.2);

	int color = 0;
	for (size_t i = 0; i < dcel.faceCount(); i++) {
		InputDcel::Face f = dcel.face(i);
		if (std::isfinite(f.data().p.h) && dcel.isRedLeaf(f)) {
			if (f.data().isSignificant) {
				p.setBrush(QBrush(colors[color++ % colors.size()]));
				QPolygonF polygon;
				for (int vId : f.data().spurBoundary) {
					polygon << convertPoint(dcel.vertex(vId).data().p);
				}
				p.drawPath(makePathRounded(polygon));
			}
		}
	}

	// outline
	p.setOpacity(1);
	p.setBrush(Qt::NoBrush);

	color = 0;
	for (size_t i = 0; i < dcel.faceCount(); i++) {
		InputDcel::Face f = dcel.face(i);
		if (std::isfinite(f.data().p.h) && dcel.isRedLeaf(f)) {
			if (f.data().isSignificant) {
				p.setPen(QPen(colors[color++ % colors.size()], 1));
				QPolygonF polygon;
				for (int vId : f.data().spurBoundary) {
					polygon << convertPoint(dcel.vertex(vId).data().p);
				}
				p.drawPath(makePathRounded(polygon));
			}
		}
	}

	// significant leaf and path to top edge
	QFont font(p.font());
	font.setPixelSize(10);
	font.setBold(true);
	p.setFont(font);

	color = 0;
	for (size_t i = 0; i < dcel.faceCount(); i++) {
		InputDcel::Face f = dcel.face(i);
		if (std::isfinite(f.data().p.h) && dcel.isRedLeaf(f)) {
			if (f.data().isSignificant) {
				p.setPen(QPen(colors[color++ % colors.size()], 2));
				p.drawEllipse(convertPoint(f.data().p), 4, 4);
				double flankingHeight = f.data().flankingHeight;
				p.drawText(
					QRectF(convertPoint(f.data().p) + QPointF(-100, -87), QSizeF(200, 200)),
					Qt::AlignCenter, QString("%1 m").arg(flankingHeight));

				// draw path upwards until top edge
				QPolygonF path;
				for (int faceId : f.data().pathToTopEdge) {
					InputDcel::Face face = dcel.face(faceId);
					path << convertPoint(face.data().p);
				}
				InputDcel::HalfEdge topEdge = dcel.halfEdge(f.data().topEdge);
				path << convertPoint(topEdge.data().p);
				p.drawPath(makePathRounded(path));
				if (std::isfinite(topEdge.oppositeFace().data().p.h)) {
					drawArrow(p, convertPoint(topEdge.data().p), convertPoint(topEdge.oppositeFace().data().p));
				}
			}
		}
	}
}
#endif

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
		if (e.origin().data().type == VertexType::saddle && inBounds(e.origin().data().p)) {
			if (m_msEdgesStraight) {
				p.drawLine(convertPoint(e.origin().data().p.x, e.origin().data().p.y),
				           convertPoint(e.destination().data().p.x, e.destination().data().p.y));
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
		p.drawLine(convertPoint(e.origin().data().p.x, e.origin().data().p.y),
		           convertPoint(e.destination().data().p.x, e.destination().data().p.y));
	} else {
		if (e.origin().data().type == VertexType::minimum) {
			e = e.twin();
		}
		for (auto l : e.data().m_dcelPath.edges()) {
			p.drawLine(convertPoint(l.origin().data().p.x, l.origin().data().p.y),
			           convertPoint(l.destination().data().p.x, l.destination().data().p.y));
		}
	}
}

void RiverWidget::drawNetwork(QPainter& p, const NetworkGraph& graph) const {
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
	          [](NetworkGraph::Edge e1, NetworkGraph::Edge e2) { return e1.delta < e2.delta; });

	// draw white casing
	for (NetworkGraph::Edge& e : edges) {
		double delta = e.delta;
		QColor color;
		double width = 4;
		if (delta < std::numeric_limits<double>::infinity()) {
			width = std::max(1.5, 3 - 0.5 * log10(deltaMax / delta));
		}
		p.setPen(QPen(QColor{"white"}, width + 2));
		p.setBrush(Qt::NoBrush);
		drawGraphEdge(p, e);
	}

	// draw colored edges
	for (NetworkGraph::Edge& e : edges) {
		double delta = e.delta;
		QColor color;
		// bubble gum
		if (delta == std::numeric_limits<double>::infinity()) {
			color = QColor("#49006a");
		} else if (delta > deltaMax / 1e1) {
			color = QColor("#7a0177");
		} else if (delta > deltaMax / 1e2) {
			color = QColor("#ae017e");
		} else if (delta > deltaMax / 1e3) {
			color = QColor("#dd3497");
		} else if (delta > deltaMax / 1e4) {
			color = QColor("#f768a1");
		} else if (delta > deltaMax / 1e5) {
			color = QColor("#fa9fb5");
		} else {
			color = QColor("#fcc5c0");
		}
		// orange brown
		/*if (delta == std::numeric_limits<double>::infinity()) {
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
		/*if (delta == std::numeric_limits<double>::infinity()) {
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
		}*/
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
	if (e.path.size() < 2) {
		return;
	}

	QPolygonF path;
	if (inBounds(e.path[0]) && inBounds(e.path[1])) {
		path << convertPoint((e.path[0].x + e.path[1].x) / 2, (e.path[0].y + e.path[1].y) / 2);
	}
	for (int i = 1; i < e.path.size(); i++) {
		if (inBounds(e.path[i])) {
			path << convertPoint(e.path[i].x, e.path[i].y);
		}
	}
	if (path.size() <= 1) {
		return;
	}
	p.drawPath(makePathRounded(path));
}

void RiverWidget::drawFingers(QPainter& p, const std::vector<InputDcel::Path>& fingers) const {
	p.setPen(QPen(QColor{"#ffffff"}, 3 + 2));
	p.setBrush(Qt::NoBrush);
	for (auto& finger : fingers) {
		QPolygonF polygon;
		finger.forAllVertices([this, &polygon](InputDcel::Vertex v) {
			if (std::isfinite(v.data().p.h)) {
				polygon << convertPoint(v.data().p);
			}
		});
		p.drawPath(makePathRounded(polygon));
	}

	//QColor color{"#238b45"};
	QColor color{"#cc4c02"};
	p.setPen(QPen{color, 3});
	for (auto& finger : fingers) {
		QPolygonF polygon;
		finger.forAllVertices([this, &polygon](InputDcel::Vertex v) {
			if (std::isfinite(v.data().p.h)) {
				polygon << convertPoint(v.data().p);
			}
		});
		p.drawPath(makePathRounded(polygon));
	}
}

QPolygonF RiverWidget::polygonForMsFace(MsComplex::Face f) const {
	QPolygonF p;

	if (m_msEdgesStraight) {
		f.forAllBoundaryEdges(
		    [&p, this](MsComplex::HalfEdge e) { p.append(convertPoint(e.origin().data().p)); });
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
		if (v.data().p.x == mousePos.x && v.data().p.y == mousePos.y) {
			return v.id();
		}
	}

	return -1;
}

std::optional<int> RiverWidget::hoveredPathVertex(const Path& p) const {
	double distanceLimit = 25.0 / m_transform.m11();
	return p.closestTo(mousePos, distanceLimit);
}

std::optional<int> RiverWidget::hoveredNonPermeableBoundaryVertex(const Boundary& b) const {
	double distanceLimit = 25.0 / m_transform.m11();
	return b.path().closestTo(mousePos, distanceLimit, [&b](int index) -> bool {
		const std::vector<Boundary::Region>& permeableRegions = b.permeableRegions();
		// Prohibit taking points that lie in existing permeable regions. Skip
		// the last one, as that's the one we're currently editing.
		for (int i = 0; i < permeableRegions.size() - 1; i++) {
			const Boundary::Region& region = permeableRegions[i];
			int start = region.m_start;
			int end = region.m_end;
			if (start <= end && index >= start && index <= end) {
				return false;
			} else if (start > end && (index >= start || index <= end)) {
				return false;
			}
		}
		return true;
	});
}

std::optional<int> RiverWidget::hoveredBoundaryMidpoint() {
	std::optional<int> result;
	double minDistance = std::numeric_limits<int>::max();

	for (int i = 0; i < m_boundaryToEdit.path().length(); i++) {
		const HeightMap::Coordinate& c1 = m_boundaryToEdit.path().m_points[i];
		const HeightMap::Coordinate& c2 = m_boundaryToEdit.path().m_points[i + 1];
		double distance = abs((c1.m_x + c2.m_x) / 2.0 - mousePos.x) +
							abs((c1.m_y + c2.m_y) / 2.0 - mousePos.y);
		if (distance < minDistance) {
			result = i;
			minDistance = distance;
		}
	}

	if (minDistance > 25.0 / m_transform.m11()) {
		return std::nullopt;
	}

	return result;
}

bool RiverWidget::inBounds(Point p) const {
	return m_drawOutOfBounds || std::isfinite(p.h);
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

ColorRamp RiverWidget::colorRamp() const {
	return m_colorRamp;
}

void RiverWidget::setColorRamp(ColorRamp colorRamp) {
	m_colorRamp = colorRamp;
	updateElevationRampTexture();
	update();
}

double RiverWidget::waterLevel() const {
	return m_waterLevel;
}

void RiverWidget::setWaterLevel(double waterLevel) {
	m_waterLevel = waterLevel;
	update();
}

double RiverWidget::contourLevel() const {
	return m_contourLevel;
}

void RiverWidget::setContourLevel(double contourLevel) {
	m_contourLevel = contourLevel;
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

bool RiverWidget::drawGradientPairsAsTrees() const {
	return m_drawGradientPairsAsTrees;
}

void RiverWidget::setDrawGradientPairsAsTrees(bool drawGradientPairsAsTrees) {
	m_drawGradientPairsAsTrees = drawGradientPairsAsTrees;
	update();
}

bool RiverWidget::showCriticalPoints() const {
	return m_showCriticalPoints;
}

void RiverWidget::setShowCriticalPoints(bool showCriticalPoints) {
	m_showCriticalPoints = showCriticalPoints;
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

bool RiverWidget::showNetwork() const {
	return m_showNetwork;
}

void RiverWidget::setShowNetwork(bool showNetwork) {
	m_showNetwork = showNetwork;
	update();
}

#ifdef EXPERIMENTAL_FINGERS_SUPPORT
bool RiverWidget::showSpurs() const {
	return m_showSpurs;
}

void RiverWidget::setShowSpurs(bool showSpurs) {
	m_showSpurs = showSpurs;
	update();
}

bool RiverWidget::showFingers() const {
	return m_showFingers;
}

void RiverWidget::setShowFingers(bool showFingers) {
	m_showFingers = showFingers;
	update();
}

void RiverWidget::setShowSimplified(bool showSimplified) {
	m_showSimplified = showSimplified;
	update();
}
#endif

double RiverWidget::networkDelta() const {
	return m_networkDelta;
}

void RiverWidget::setNetworkDelta(double networkDelta) {
	m_networkDelta = networkDelta;
	update();
}

void RiverWidget::setPointToHighlight(std::optional<Point> point) {
	m_pointToHighlight = point;
	update();
}

void RiverWidget::setContourMask(QImage mask) {
	contourMaskTexture = new QOpenGLTexture(mask);
	contourMaskTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
	contourMaskTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
	update();
}

void RiverWidget::setUnits(Units units) {
	m_units = units;
	update();
}

void RiverWidget::startBoundaryGenerateMode() {
	m_mode = Mode::GENERATE_BOUNDARY_SEED;
	m_boundaryCreator = BoundaryCreator{m_riverFrame->m_heightMap};
	emit statusMessage("Select an area surrounded by nodata to draw a boundary around");
	update();
}

void RiverWidget::startSetSourceSinkMode() {
	m_boundaryToEdit = m_riverData->boundary();
	m_mode = Mode::SET_SOURCE_START;
	m_boundaryToEdit.removePermeableRegions();
	m_boundaryToEdit.addPermeableRegion(Boundary::Region{0, 0});
	emit statusMessage("Click to indicate where on the boundary the source starts and ends (in clockwise order)");
	update();
}

void RiverWidget::startBoundaryEditMode() {
	m_mode = Mode::EDIT_BOUNDARY;
	m_boundaryToEdit = Boundary{m_riverData->boundary()};
	emit statusMessage("Drag points to edit the boundary; drag from the middle of an edge to add a point");
	update();
}

void RiverWidget::endBoundaryEditMode() {
	m_mode = Mode::NORMAL;
	emit boundaryEdited(m_boundaryToEdit);
	update();
}

bool RiverWidget::boundaryEditMode() const {
	return m_mode != Mode::NORMAL;
}

void RiverWidget::setPointsToDraw(std::vector<Point> pointsToDraw) {
	m_pointsToDraw = pointsToDraw;
	update();
}

void RiverWidget::setEdgesToDraw(std::vector<InputDcel::HalfEdge> edgesToDraw) {
	m_edgesToDraw = edgesToDraw;
	update();
}
