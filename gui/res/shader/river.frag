#version 130

/**
 * Fragment shader for the river widget.
 */

uniform mat3 matrix;

/**
 * The height map texture.
 */
uniform sampler2D tex;

/**
 * A texture that defines the color ramp used for drawing elevations.
 */
uniform sampler2D elevationRampTex;

/**
 * A texture that defines in which part of the terrain the contour has to be
 * drawn.
 */
uniform sampler2D contourMaskTex;

/**
 * The water level.
 */
uniform float waterLevel;

/**
 * The level of the contour.
 */
uniform float contourLevel;

/**
 * Whether to show the background map.
 */
uniform bool showMap;

/**
 * Whether to show contour lines.
 */
uniform bool showOutlines;

/**
 * How many contours to render.
 */
uniform int contourCount;

/**
 * Whether to show hill-shading.
 */
uniform bool showShading;

/**
 * Whether to show the water in blue and the land in brown, as opposed to a
 * black-and-white image.
 */
uniform bool showWaterPlane;

/**
 * If showOutlines is true, the width of the outlines.
 */
uniform float lineWidth;

/**
 * If showOutlines is true, the height of the outlines.
 */
uniform float lineHeight;

/**
 * Width of the heightmap texture.
 */
uniform float texWidth;

/**
 * Height of the heightmap texture.
 */
uniform float texHeight;

/**
 * The position of the fragment being drawn.
 */
in vec4 pos;

/**
 * The computed color.
 */
out vec3 color;

vec2 texCoord(vec2 p) {
	return (matrix * vec3(p.x, p.y, 1)).xy;
}

float elevation(vec2 p) {
	vec2 coordinate = texCoord(p);
	return dot(texture(tex, coordinate).rgb,
	           vec3(255.0f / 256.0f,
	                255.0f / (256.0f * 256.0f),
	                255.0f / (256.0f * 256.0f * 256.0f)));
}

vec3 waterColor(float depth) {
	return vec3(116.0 * (1.5 - 1.25 * depth),
	            163.0 * (1.5 - 1.25 * depth),
	            238.0 * (2.5 - 1.5 * depth)) / 256.0;
}

bool isOnPeriodicContour(vec2 p) {
	bool onOutline = false;
	int value = int(contourCount * elevation(p.xy));
	for (int dx = -1; dx <= 1; dx++) {
		for (int dy = -1; dy <= 1; dy++) {
			int valueAround = int(contourCount * elevation(
			            p.xy + vec2(dx * lineWidth, dy * lineHeight)));
			onOutline = onOutline || (value != valueAround);
		}
	}
	return onOutline;
}

bool isOnContour(vec2 p, float contourHeight) {
	bool onOutline = false;
	for (int dx = -1; dx <= 1; dx++) {
		for (int dy = -1; dy <= 1; dy++) {
			float heightAround = elevation(
						pos.xy + vec2(dx * lineWidth, dy * lineHeight)) - contourHeight;
			onOutline = onOutline || ((elevation(pos.xy) - contourHeight <= 0) != (heightAround <= 0));
		}
	}
	return onOutline;
}

/**
 * The main method.
 */
void main() {
	vec2 tPos = texCoord(pos.xy);
	bool inBounds = tPos.x >= .5f / texWidth && tPos.x <= 1 - .5f / texWidth &&
			tPos.y >= .5f / texHeight && tPos.y <= 1 - .5f / texHeight;
	bool isNoData = texture(tex, tPos).a < 1.0f;

	if (!inBounds || isNoData) {
		color = vec3(1);
		return;
	}

	float height = elevation(pos.xy);
	if (showWaterPlane && waterLevel > height) {
		color = waterColor(waterLevel - height);
	} else if (showMap) {
		color = texture(elevationRampTex, vec2(height, 0)).rgb;
	} else {
		color = vec3(1);
	}

	if (showShading) {
		/* source: http://frozenfractal.com/blog/2013/5/24/height-map-shader */
		vec2 offset = vec2(1.0 / texWidth, 1.0 / texHeight) * 1;
		float brightness =
			(
			texture2D(tex, texCoord(pos.xy) + offset).r -
			texture2D(tex, texCoord(pos.xy) - offset).r
			);
		brightness = min(2, brightness);
		color *= brightness + 1;
	}

	if (showOutlines) {
		if (isOnPeriodicContour(pos.xy)) {
			color = color * 0.8;
		}
	}

	// draw black lines between water and land
	if (showWaterPlane) {
		if (isOnContour(pos.xy, waterLevel)) {
			color = color * 0.5;
		}
	}

	// draw contour
	bool onContourMask = texture(contourMaskTex, tPos - vec2(.5 / texWidth, .5 / texHeight)).r > 0.5;
	// debug: draw mask
	//if (onContourMask && int(gl_FragCoord.x) % 2 == int(gl_FragCoord.y) % 2) {
	//	color = vec3(1, 0, 0);
	//}
	if (isOnContour(pos.xy, contourLevel) && onContourMask) {
		color = vec3(0.137, 0.545, 0.271);
	}
}
