#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <QString>

struct CubeLut {
	std::string title;
	uint32_t size = 0;
	float domainMin[3] = {0.0f, 0.0f, 0.0f};
	float domainMax[3] = {1.0f, 1.0f, 1.0f};
	// Flat RGBA data in order: z (blue outer), y (green mid), x (red inner)
	// Stored as RGBA floats (4 floats per entry), length = size * size * size * 4
	std::vector<float> data;

	bool isValid() const { return size >= 2 && data.size() == static_cast<size_t>(size * size * size * 4); }

	// CPU-side trilinear interpolation
	void sample(float r, float g, float b, float &outR, float &outG, float &outB) const;
};

class CubeParser {
public:
	static bool loadCubeFile(const QString &filePath, CubeLut &lut);
	static bool loadCubeFromString(const std::string &content, CubeLut &lut);
};
