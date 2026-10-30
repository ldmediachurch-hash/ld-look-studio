#include "cube-parser.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

static inline float clamp01(float v)
{
	return std::max(0.0f, std::min(1.0f, v));
}

static inline float lerpf(float a, float b, float t)
{
	return a + t * (b - a);
}

void CubeLut::sample(float r, float g, float b, float &outR, float &outG, float &outB) const
{
	if (!isValid()) {
		outR = r;
		outG = g;
		outB = b;
		return;
	}

	float spanR = domainMax[0] - domainMin[0];
	float spanG = domainMax[1] - domainMin[1];
	float spanB = domainMax[2] - domainMin[2];

	if (spanR <= 0.0001f)
		spanR = 1.0f;
	if (spanG <= 0.0001f)
		spanG = 1.0f;
	if (spanB <= 0.0001f)
		spanB = 1.0f;

	float u = ((clamp01(r) - domainMin[0]) / spanR) * static_cast<float>(size - 1);
	float v = ((clamp01(g) - domainMin[1]) / spanG) * static_cast<float>(size - 1);
	float w = ((clamp01(b) - domainMin[2]) / spanB) * static_cast<float>(size - 1);

	u = std::max(0.0f, std::min(static_cast<float>(size - 1), u));
	v = std::max(0.0f, std::min(static_cast<float>(size - 1), v));
	w = std::max(0.0f, std::min(static_cast<float>(size - 1), w));

	uint32_t x0 = static_cast<uint32_t>(u);
	uint32_t y0 = static_cast<uint32_t>(v);
	uint32_t z0 = static_cast<uint32_t>(w);

	uint32_t x1 = std::min(x0 + 1, size - 1);
	uint32_t y1 = std::min(y0 + 1, size - 1);
	uint32_t z1 = std::min(z0 + 1, size - 1);

	float fx = u - static_cast<float>(x0);
	float fy = v - static_cast<float>(y0);
	float fz = w - static_cast<float>(z0);

	auto getVoxel = [&](uint32_t x, uint32_t y, uint32_t z) -> const float * {
		size_t idx = static_cast<size_t>(((z * size + y) * size + x) * 4);
		return &data[idx];
	};

	const float *c000 = getVoxel(x0, y0, z0);
	const float *c100 = getVoxel(x1, y0, z0);
	const float *c010 = getVoxel(x0, y1, z0);
	const float *c110 = getVoxel(x1, y1, z0);

	const float *c001 = getVoxel(x0, y0, z1);
	const float *c101 = getVoxel(x1, y0, z1);
	const float *c011 = getVoxel(x0, y1, z1);
	const float *c111 = getVoxel(x1, y1, z1);

	for (int c = 0; c < 3; ++c) {
		float c00 = lerpf(c000[c], c100[c], fx);
		float c10 = lerpf(c010[c], c110[c], fx);
		float c0 = lerpf(c00, c10, fy);

		float c01 = lerpf(c001[c], c101[c], fx);
		float c11 = lerpf(c011[c], c111[c], fx);
		float c1 = lerpf(c01, c11, fy);

		float val = lerpf(c0, c1, fz);
		if (c == 0)
			outR = clamp01(val);
		else if (c == 1)
			outG = clamp01(val);
		else
			outB = clamp01(val);
	}
}

bool CubeParser::loadCubeFile(const QString &filePath, CubeLut &lut)
{
	std::ifstream file(filePath.toStdString(), std::ios::in);
	if (!file.is_open()) {
		return false;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return loadCubeFromString(buffer.str(), lut);
}

bool CubeParser::loadCubeFromString(const std::string &content, CubeLut &lut)
{
	lut.title.clear();
	lut.size = 0;
	lut.domainMin[0] = 0.0f;
	lut.domainMin[1] = 0.0f;
	lut.domainMin[2] = 0.0f;
	lut.domainMax[0] = 1.0f;
	lut.domainMax[1] = 1.0f;
	lut.domainMax[2] = 1.0f;
	lut.data.clear();

	std::istringstream stream(content);
	std::string line;

	std::vector<float> points;

	while (std::getline(stream, line)) {
		// Strip carriage returns or trailing spaces
		while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
			line.pop_back();
		}
		if (line.empty() || line[0] == '#') {
			continue;
		}

		if (line.rfind("TITLE", 0) == 0) {
			size_t firstQuote = line.find('"');
			size_t lastQuote = line.rfind('"');
			if (firstQuote != std::string::npos && lastQuote != std::string::npos &&
			    lastQuote > firstQuote) {
				lut.title = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
			} else {
				lut.title = line.substr(5);
			}
			continue;
		}

		if (line.rfind("LUT_3D_SIZE", 0) == 0) {
			std::istringstream iss(line.substr(11));
			iss >> lut.size;
			continue;
		}

		if (line.rfind("DOMAIN_MIN", 0) == 0) {
			std::istringstream iss(line.substr(10));
			iss >> lut.domainMin[0] >> lut.domainMin[1] >> lut.domainMin[2];
			continue;
		}

		if (line.rfind("DOMAIN_MAX", 0) == 0) {
			std::istringstream iss(line.substr(10));
			iss >> lut.domainMax[0] >> lut.domainMax[1] >> lut.domainMax[2];
			continue;
		}

		// Check if line contains 3 float numbers
		float r = 0.0f, g = 0.0f, b = 0.0f;
		std::istringstream iss(line);
		if (iss >> r >> g >> b) {
			points.push_back(r);
			points.push_back(g);
			points.push_back(b);
		}
	}

	if (lut.size == 0 || points.size() != static_cast<size_t>(lut.size * lut.size * lut.size * 3)) {
		return false;
	}

	size_t totalVoxels = static_cast<size_t>(lut.size * lut.size * lut.size);
	lut.data.resize(totalVoxels * 4);

	for (size_t i = 0; i < totalVoxels; ++i) {
		lut.data[i * 4 + 0] = points[i * 3 + 0];
		lut.data[i * 4 + 1] = points[i * 3 + 1];
		lut.data[i * 4 + 2] = points[i * 3 + 2];
		lut.data[i * 4 + 3] = 1.0f;
	}

	return true;
}
