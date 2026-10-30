#pragma once

#include <obs.h>

#include <memory>
#include <vector>

#include <QComboBox>
#include <QDockWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

#include "look-library.hpp"
#include "snapshot-manager.hpp"

class LookCardWidget : public QWidget {
	Q_OBJECT

public:
	explicit LookCardWidget(const LookItem &item, QWidget *parent = nullptr);

	void setThumbnail(const QImage &image);
	void setIsLive(bool isLive);
	const LookItem &getLookItem() const { return item_; }

signals:
	void clicked(const LookItem &item);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;

private:
	LookItem item_;
	QImage thumbnail_;
	bool isLive_ = false;
	bool isHovered_ = false;
};

class LookStudioDock : public QDockWidget {
	Q_OBJECT

public:
	explicit LookStudioDock(QWidget *parent = nullptr);
	~LookStudioDock() override;

	void initialize(const QString &pluginDataPath);

public slots:
	void refreshSources();
	void onSourceChanged(int index);
	void onCategorySelected(const QString &category);
	void onLookCardClicked(const LookItem &item);
	void onIntensityChanged(int value);
	void onSkinProtectChanged(int value);
	void onSplitModeChanged(int index);
	void onWipePosChanged(int value);
	void onCaptureStillClicked();
	void onImportLutClicked();
	void onResetClicked();
	void onThumbnailsRegenerated();

private:
	void setupUi();
	void updateGalleryCards();
	void applyStyling();
	obs_source_t *getSelectedSource() const;

	LookLibrary *library_ = nullptr;
	SnapshotManager *snapshotMgr_ = nullptr;

	QString activeCategoryId_ = "All";
	QString activeLookId_ = "wakanda-royal-gold";

	// UI Controls
	QComboBox *sourceCombo_ = nullptr;
	QPushButton *captureStillBtn_ = nullptr;

	QHBoxLayout *categoryLayout_ = nullptr;
	std::vector<QPushButton *> categoryButtons_;

	QWidget *galleryContainer_ = nullptr;
	QGridLayout *galleryLayout_ = nullptr;
	std::vector<LookCardWidget *> cardWidgets_;

	QSlider *intensitySlider_ = nullptr;
	QLabel *intensityValueLabel_ = nullptr;

	QSlider *skinProtectSlider_ = nullptr;
	QLabel *skinProtectValueLabel_ = nullptr;

	QComboBox *splitModeCombo_ = nullptr;
	QSlider *wipeSlider_ = nullptr;
	QLabel *wipeValueLabel_ = nullptr;

	QPushButton *applyLiveBtn_ = nullptr;
	QPushButton *importLutBtn_ = nullptr;
	QPushButton *resetBtn_ = nullptr;
	QLabel *statusHintLabel_ = nullptr;
};
