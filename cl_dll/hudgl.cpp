#define _USE_MATH_DEFINES
#include "util_vector.h"
#include "hudgl.h"

#include <cmath>
#include <vector>

#ifdef _WIN32
#include <winsani_in.h>
#include <Windows.h>
#include <winsani_out.h>
#pragma comment(lib, "opengl32.lib")
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

HudGL::HudGL() {
	// Same steps as FillRGBA does.
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

HudGL::~HudGL() {
	// Reset the line width in case we changed it.
	glLineWidth(1.0f);

	// Same steps as FillRGBA does.
	glColor3f(1.0f, 1.0f, 1.0f);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
}

void HudGL::color(float r, float g, float b, float a) const {
	glColor4f(r, g, b, a);
}

void HudGL::color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) const {
	glColor4ub(r, g, b, a);
}

void HudGL::line_width(float width) const {
	glLineWidth(width);
}

void HudGL::line(const Vector2D& start, const Vector2D& end) const {
	glBegin(GL_LINES);
	glVertex2f(start.x, start.y);
	glVertex2f(end.x, end.y);
	glEnd();
}

void HudGL::circle(const Vector2D& center, const std::vector<Vector2D>& points) const {
	glBegin(GL_LINE_STRIP);

	for (const auto& point : points)
		glVertex2f(center.x + point.x, center.y + point.y);

	glVertex2f(center.x + points[0].x, center.y + points[0].y);
	glEnd();
}

void HudGL::rectangle(const Vector2D& corner_a, const Vector2D& corner_b) const {
	glBegin(GL_QUADS);
	glVertex2f(corner_a.x, corner_a.y);
	glVertex2f(corner_a.x, corner_b.y);
	glVertex2f(corner_b.x, corner_b.y);
	glVertex2f(corner_b.x, corner_a.y);
	glEnd();
}

std::vector<Vector2D> HudGL::compute_circle(float radius) {
	// Maximum allowed distance between the circle and the rendered line segment.
	constexpr float MAX_ERROR = 0.1f;
	const unsigned segment_count =
		static_cast<unsigned>(std::ceil(M_PI / std::acos((radius - MAX_ERROR) / radius)));

	std::vector<Vector2D> points;
	points.reserve(segment_count);

	for (unsigned i = 0; i < segment_count; ++i) {
		float angle = static_cast<float>(M_PI * 2 * i / segment_count);
		points.emplace_back(radius * std::cos(angle), radius * std::sin(angle));
	}

	return points;
}
