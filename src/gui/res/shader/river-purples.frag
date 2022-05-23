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
 * The water level.
 */
uniform float waterLevel;
uniform float waterSlope;

/**
 * Whether to show the background map.
 */
uniform bool showMap;

/**
 * Whether to show contour lines.
 */
uniform bool showOutlines;

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

float elevation(vec2 p) {
	vec2 coordinate = (matrix * vec3(p.x, p.y, 1)).xy;
	float x = coordinate.x * texWidth + 0.5;
	float y = coordinate.y * texHeight + 0.5;
	float fx = floor(x);
	float fy = floor(y);
	float h1 = dot(texture(tex, vec2((fx - .5) / texWidth, (fy + .5) / texHeight)).rgb, vec3(1., 1. / 256, 1. / 65536));
	float h2 = dot(texture(tex, vec2((fx + .5) / texWidth, (fy + .5) / texHeight)).rgb, vec3(1., 1. / 256, 1. / 65536));
	float h3 = dot(texture(tex, vec2((fx - .5) / texWidth, (fy - .5) / texHeight)).rgb, vec3(1., 1. / 256, 1. / 65536));
	float h4 = dot(texture(tex, vec2((fx + .5) / texWidth, (fy - .5) / texHeight)).rgb, vec3(1., 1. / 256, 1. / 65536));

	float dx = x - fx;
	float dy = y - fy;

	if (1 - dx < dy) {
		// upper triangle (h1, h2, h4)
		return h1 * (1 - dx) + h4 * (1 - dy) + h2 * (dx + dy - 1);
	} else {
		// lower triangle (h1, h3, h4)
		return h1 * dy + h4 * dx + h3 * (1 - dx - dy);
	}
}

float elevationDetrended(vec2 p) {
	return elevation(p) - waterLevel +
	        (-0.5 + (matrix * vec3(p.x, p.y, 1)).x) * waterSlope;
}

vec3 waterColor(float value) {
	return vec3(116.0 * (1.25 * value + 1.5),
	            163.0 * (1.25 * value + 1.5),
	            238.0 * (.5 * value + 2.5)) / 256.0;
}

vec2 texCoord(vec2 pos) {
	return (matrix * vec3(pos.x, pos.y, 1)).xy;
}

bool isOnContour(vec2 pos) {
	bool onOutline = false;
	int value = int(20 * elevation(pos.xy));
	for (int dx = -1; dx <= 1; dx++) {
		for (int dy = -1; dy <= 1; dy++) {
			int valueAround = int(20 * elevation(
			            pos.xy + vec2(dx * lineWidth, dy * lineHeight)));
			onOutline = onOutline || (value != valueAround);
		}
	}
	return onOutline;
}

vec3 interpolate(float p, float p1, float p2, vec3 v1, vec3 v2) {
	float fraction = (p - p1) / (p2 - p1);
	return (1 - fraction) * v1 + fraction * v2;
}

vec3 c(int r, int g, int b) {
	return vec3(r / 255f, g / 255f, b / 255f);
}

/**
 * The main method.
 */
void main() {
	vec2 tPos = texCoord(pos.xy);

	if (tPos.x >= .5f / texWidth && tPos.x <= 1 - .5f / texWidth &&
			tPos.y >= .5f / texHeight && tPos.y <= 1 - .5f / texHeight) {

		float value = elevationDetrended(pos.xy);

		if (showWaterPlane && value < 0) {
			color = waterColor(value);
		} else if (showMap) {
			float p = elevation(pos.xy);
			if (p >= 1f) {
				color = vec3(1f);
			} else if (p >= 2/3f) {
				color = interpolate(p, 2/3f, 1f, c(192, 212, 230), c(241, 240, 246));
			} else {
				color = interpolate(p, 0f, 2/3f, c(63, 1, 125), c(192, 212, 230));
			}
		} else {
			color = vec3(1);
		}

		if (showShading) {
			/* source: http://frozenfractal.com/blog/2013/5/24/height-map-shader */
			vec2 offset = vec2(1.0 / texWidth, -1.0 / texHeight) * 1;
			float brightness =
			    (
			    texture2D(tex, texCoord(pos.xy) + offset).r -
			    texture2D(tex, texCoord(pos.xy) - offset).r
			    );
			brightness = min(2, brightness);
			color *= brightness + 1;
		}

		if (showOutlines) {
			if (isOnContour(pos.xy)) {
				color = color * 0.8;
			}
		}

		// draw black lines between water and land
		if (showWaterPlane) {
			bool onOutline = false;
			for (int dx = -1; dx <= 1; dx++) {
				for (int dy = -1; dy <= 1; dy++) {
					float valueAround = elevationDetrended(
								pos.xy + vec2(dx * lineWidth, dy * lineHeight));
					if ((value <= 0) != (valueAround <= 0)) {
						onOutline = true;
					}
				}
			}
			if (onOutline) {
				color = color * 0.5;
			}
		}

	} else {
		color = vec3(1);
	}
}
