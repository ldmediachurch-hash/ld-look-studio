#include "snapshot-manager.hpp"

#include <algorithm>
#include <cmath>

#include <QPainter>
#include <QPainterPath>

static inline float clamp01(float v)
{
	return std::max(0.0f, std::min(1.0f, v));
}

static inline float lerpf(float a, float b, float t)
{
	return a + t * (b - a);
}

static float computeMelaninMaskCpu(float r, float g, float b)
{
	float y = 0.299f * r + 0.587f * g + 0.114f * b;
	float cb = -0.168736f * r - 0.331264f * g + 0.500f * b;
	float cr = 0.500f * r - 0.418688f * g - 0.081312f * b;

	float angle = std::atan2(cr, -cb);
	float angle_diff = std::abs(angle - 1.95f);
	float hue_score = clamp01(1.0f - angle_diff / 0.55f);

	float lum_score = 0.0f;
	if (y > 0.08f && y < 0.85f) {
		float low = (y - 0.08f) / 0.10f;
		float high = (0.85f - y) / 0.25f;
		lum_score = clamp01(std::min(low, high));
	}

	float chroma_len = std::sqrt(cb * cb + cr * cr);
	float chroma_score = clamp01((chroma_len - 0.02f) / 0.04f);
	float highlight_protect = clamp01((0.95f - y) / 0.25f);

	return clamp01(hue_score * lum_score * chroma_score * highlight_protect);
}

SnapshotManager::SnapshotManager(LookLibrary *library, QObject *parent) : QObject(parent), library_(library)
{
	baseStill_ = createStudioCalibrationCard();
}

void SnapshotManager::setBaseStill(const QImage &image)
{
	if (image.isNull()) {
		return;
	}

	{
		std::lock_guard<std::mutex> lock(mutex_);
		// Scale down to 320x180 for crisp fast thumbnail previews
		baseStill_ = image.scaled(320, 180, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
		thumbnailCache_.clear();
	}

	emit stillUpdated(baseStill_);
	emit thumbnailsRegenerated();
}

QImage SnapshotManager::getBaseStill() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return baseStill_;
}

void SnapshotManager::invalidateThumbnails()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		thumbnailCache_.clear();
	}
	emit thumbnailsRegenerated();
}

QImage SnapshotManager::getThumbnailForLook(const LookItem &item, float intensity, float skinProtect)
{
	std::string cacheKey = item.id.toStdString() + "_" + std::to_string(static_cast<int>(intensity * 100)) + "_" +
			       std::to_string(static_cast<int>(skinProtect * 100));

	{
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = thumbnailCache_.find(cacheKey);
		if (it != thumbnailCache_.end()) {
			return it->second;
		}
	}

	if (!library_) {
		return baseStill_;
	}

	auto lut = library_->getLutData(item);
	if (!lut || !lut->isValid()) {
		return baseStill_;
	}

	QImage graded = applyLutToImage(baseStill_, *lut, intensity, skinProtect);

	{
		std::lock_guard<std::mutex> lock(mutex_);
		thumbnailCache_[cacheKey] = graded;
	}

	return graded;
}

QImage SnapshotManager::applyLutToImage(const QImage &src, const CubeLut &lut, float intensity, float skinProtect)
{
	QImage dst = src.convertToFormat(QImage::Format_ARGB32);
	int width = dst.width();
	int height = dst.height();

	for (int y = 0; y < height; ++y) {
		QRgb *scanLine = reinterpret_cast<QRgb *>(dst.scanLine(y));
		for (int x = 0; x < width; ++x) {
			QRgb pixel = scanLine[x];
			float r = qRed(pixel) / 255.0f;
			float g = qGreen(pixel) / 255.0f;
			float b = qBlue(pixel) / 255.0f;
			int a = qAlpha(pixel);

			float gr, gg, gb;
			lut.sample(r, g, b, gr, gg, gb);

			if (skinProtect > 0.001f) {
				float skinMask = computeMelaninMaskCpu(r, g, b);
				float protectWeight = skinMask * skinProtect;
				float pr = lerpf(gr, r, protectWeight);
				float pg = lerpf(gg, g, protectWeight);
				float pb = lerpf(gb, b, protectWeight);

				float melaninGlow = skinMask * 0.04f * skinProtect;
				pr += melaninGlow * 1.05f;
				pg += melaninGlow * 0.75f;
				pb += melaninGlow * 0.35f;

				gr = clamp01(pr);
				gg = clamp01(pg);
				gb = clamp01(pb);
			}

			float finalR = lerpf(r, gr, intensity);
			float finalG = lerpf(g, gg, intensity);
			float finalB = lerpf(b, gb, intensity);

			scanLine[x] = qRgba(static_cast<int>(clamp01(finalR) * 255.0f),
					    static_cast<int>(clamp01(finalG) * 255.0f),
					    static_cast<int>(clamp01(finalB) * 255.0f), a);
		}
	}

	return dst;
}

QImage SnapshotManager::createStudioCalibrationCard()
{
	QImage card(320, 180, QImage::Format_ARGB32);
	card.fill(QColor(18, 20, 26));

	QPainter p(&card);
	p.setRenderHint(QPainter::Antialiasing);

	// Studio gradient background
	QLinearGradient bgGrad(0, 0, 320, 180);
	bgGrad.setColorAt(0.0, QColor(25, 28, 36));
	bgGrad.setColorAt(1.0, QColor(12, 14, 18));
	p.fillRect(0, 0, 320, 180, bgGrad);

	// Tonal ramp across mid screen
	int barY = 35;
	int barH = 50;
	int stepW = 320 / 8;
	QColor tones[8] = {
		QColor(15, 12, 10),    // Deep shadow
		QColor(42, 28, 20),    // Deep mahogany / ebony
		QColor(85, 52, 35),    // Rich dark bronze
		QColor(135, 85, 55),   // Warm terracotta / amber
		QColor(185, 125, 80),  // Golden honey melanin
		QColor(220, 165, 120), // Radiant bronze highlight
		QColor(240, 210, 180), // Warm ivory specular
		QColor(255, 245, 235)  // Peak highlight
	};

	for (int i = 0; i < 8; ++i) {
		p.fillRect(i * stepW, barY, stepW, barH, tones[i]);
	}

	// Smooth continuous gradient bar
	int gradY = 95;
	int gradH = 30;
	QLinearGradient skinGrad(15, 0, 305, 0);
	skinGrad.setColorAt(0.00, QColor(20, 14, 10));
	skinGrad.setColorAt(0.20, QColor(65, 40, 26));
	skinGrad.setColorAt(0.45, QColor(140, 85, 50));
	skinGrad.setColorAt(0.70, QColor(200, 140, 90));
	skinGrad.setColorAt(0.90, QColor(235, 185, 140));
	skinGrad.setColorAt(1.00, QColor(255, 235, 215));
	p.fillRect(15, gradY, 290, gradH, skinGrad);

	// Card border
	p.setPen(QColor(45, 50, 62));
	p.drawRect(0, 0, 319, 179);

	// Studio label
	p.setPen(QColor(140, 147, 160));
	QFont f = p.font();
	f.setPixelSize(10);
	f.setBold(true);
	p.setFont(f);
	p.drawText(QRect(15, 135, 290, 35), Qt::AlignLeft | Qt::AlignVCenter, "LD LOOK STUDIO · COLOR CALIBRATION");

	p.end();
	return card;
}
