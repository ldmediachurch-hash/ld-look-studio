#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include <QColor>
#include <QImage>
#include <QObject>
#include <QString>

#include "look-library.hpp"

class SnapshotManager : public QObject {
	Q_OBJECT

public:
	explicit SnapshotManager(LookLibrary *library, QObject *parent = nullptr);

	void setBaseStill(const QImage &image);
	QImage getBaseStill() const;

	QImage getThumbnailForLook(const LookItem &item, float intensity = 1.0f, float skinProtect = 0.5f);
	void invalidateThumbnails();

	// Fallback studio test still (color bars + melanin skin tone swatches)
	static QImage createStudioCalibrationCard();

signals:
	void stillUpdated(const QImage &still);
	void thumbnailsRegenerated();

private:
	QImage applyLutToImage(const QImage &src, const CubeLut &lut, float intensity, float skinProtect);

	LookLibrary *library_ = nullptr;
	mutable std::mutex mutex_;
	QImage baseStill_;
	std::unordered_map<std::string, QImage> thumbnailCache_;
};
