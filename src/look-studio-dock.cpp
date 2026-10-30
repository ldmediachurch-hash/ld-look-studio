#include "look-studio-dock.hpp"
#include "look-filter.hpp"

#include <obs-frontend-api.h>
#include <util/platform.h>

#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>

// Helper to set snapshot manager pointer for filter
extern void look_filter_set_snapshot_manager(SnapshotManager *mgr);

// ============================================================================
// LookCardWidget Implementation
// ============================================================================

LookCardWidget::LookCardWidget(const LookItem &item, QWidget *parent) : QWidget(parent), item_(item)
{
	setCursor(Qt::PointingHandCursor);
	setMinimumSize(140, 105);
	setMaximumHeight(130);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void LookCardWidget::setThumbnail(const QImage &image)
{
	thumbnail_ = image;
	update();
}

void LookCardWidget::setIsLive(bool isLive)
{
	if (isLive_ != isLive) {
		isLive_ = isLive;
		update();
	}
}

void LookCardWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		emit clicked(item_);
	}
	QWidget::mousePressEvent(event);
}

void LookCardWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	QRect r = rect().adjusted(1, 1, -1, -1);
	int cornerRadius = 6;

	QPainterPath clipPath;
	clipPath.addRoundedRect(r, cornerRadius, cornerRadius);
	p.setClipPath(clipPath);

	// 1. Draw thumbnail or fallback dark background
	if (!thumbnail_.isNull()) {
		p.drawImage(r, thumbnail_);
	} else {
		p.fillRect(r, QColor(22, 25, 32));
	}

	// 2. Bottom gradient overlay for title legibility
	QRect bottomGradRect(r.left(), r.bottom() - 40, r.width(), 40);
	QLinearGradient textBg(0, bottomGradRect.top(), 0, bottomGradRect.bottom());
	textBg.setColorAt(0.0, QColor(0, 0, 0, 0));
	textBg.setColorAt(0.4, QColor(10, 12, 16, 180));
	textBg.setColorAt(1.0, QColor(8, 10, 14, 235));
	p.fillRect(bottomGradRect, textBg);

	// 3. Draw Title & Category
	p.setPen(QColor(255, 255, 255));
	QFont titleFont = font();
	titleFont.setPixelSize(11);
	titleFont.setBold(true);
	p.setFont(titleFont);
	QRect titleRect(r.left() + 8, r.bottom() - 32, r.width() - 16, 16);
	p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, item_.title);

	p.setPen(QColor(140, 147, 160));
	QFont catFont = font();
	catFont.setPixelSize(9);
	catFont.setBold(false);
	p.setFont(catFont);
	QRect catRect(r.left() + 8, r.bottom() - 16, r.width() - 16, 14);
	p.drawText(catRect, Qt::AlignLeft | Qt::AlignVCenter, item_.category.toUpper());

	// 4. Live Badge
	if (isLive_) {
		QRect badgeRect(r.left() + 6, r.top() + 6, 52, 18);
		p.setPen(Qt::NoPen);
		p.setBrush(QColor(15, 17, 21, 210));
		p.drawRoundedRect(badgeRect, 4, 4);

		// Red Dot
		p.setBrush(QColor(255, 45, 45));
		p.drawEllipse(badgeRect.left() + 6, badgeRect.top() + 5, 7, 7);

		// "LIVE" Text
		p.setPen(QColor(255, 255, 255));
		QFont liveFont = font();
		liveFont.setPixelSize(9);
		liveFont.setBold(true);
		p.setFont(liveFont);
		p.drawText(QRect(badgeRect.left() + 16, badgeRect.top(), badgeRect.width() - 16, badgeRect.height()),
			   Qt::AlignVCenter | Qt::AlignLeft, "LIVE");
	}

	// 5. Border: 2px White if Live, 1px subtle dark otherwise
	p.setClipping(false);
	if (isLive_) {
		p.setPen(QPen(QColor(255, 255, 255), 2.0));
		p.setBrush(Qt::NoBrush);
		p.drawRoundedRect(r, cornerRadius, cornerRadius);
	} else {
		p.setPen(QPen(QColor(34, 38, 48), 1.0));
		p.setBrush(Qt::NoBrush);
		p.drawRoundedRect(r, cornerRadius, cornerRadius);
	}
}

// ============================================================================
// LookStudioDock Implementation
// ============================================================================

LookStudioDock::LookStudioDock(QWidget *parent) : QDockWidget("LD Look Studio", parent)
{
	setObjectName("LDLookStudioDock");
	setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

	library_ = new LookLibrary(this);
	snapshotMgr_ = new SnapshotManager(library_, this);
	look_filter_set_snapshot_manager(snapshotMgr_);

	setupUi();
	applyStyling();

	connect(snapshotMgr_, &SnapshotManager::thumbnailsRegenerated, this, &LookStudioDock::onThumbnailsRegenerated);
	connect(library_, &LookLibrary::libraryChanged, this, &LookStudioDock::updateGalleryCards);
}

LookStudioDock::~LookStudioDock()
{
	look_filter_set_snapshot_manager(nullptr);
}

void LookStudioDock::initialize(const QString &pluginDataPath)
{
	library_->initialize(pluginDataPath);
	refreshSources();
	updateGalleryCards();
}

void LookStudioDock::setupUi()
{
	QWidget *mainContainer = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout(mainContainer);
	mainLayout->setContentsMargins(12, 12, 12, 12);
	mainLayout->setSpacing(10);

	// ------------------------------------------------------------------------
	// Header Section: Source Selector & Freeze Still
	// ------------------------------------------------------------------------
	QHBoxLayout *sourceHeader = new QHBoxLayout();
	sourceHeader->setSpacing(8);

	QLabel *sourceLabel = new QLabel("TARGET SOURCE", mainContainer);
	sourceLabel->setObjectName("sectionHeader");

	sourceCombo_ = new QComboBox(mainContainer);
	sourceCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	connect(sourceCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&LookStudioDock::onSourceChanged);

	captureStillBtn_ = new QPushButton("Freeze Frame", mainContainer);
	captureStillBtn_->setToolTip("Capture current frame from upstream filter stack to update Look Gallery");
	connect(captureStillBtn_, &QPushButton::clicked, this, &LookStudioDock::onCaptureStillClicked);

	sourceHeader->addWidget(sourceLabel);
	sourceHeader->addWidget(sourceCombo_, 1);
	sourceHeader->addWidget(captureStillBtn_);
	mainLayout->addLayout(sourceHeader);

	// ------------------------------------------------------------------------
	// Category Filter Chips
	// ------------------------------------------------------------------------
	QHBoxLayout *catHeader = new QHBoxLayout();
	QLabel *galleryLabel = new QLabel("LOOK GALLERY", mainContainer);
	galleryLabel->setObjectName("sectionHeader");
	catHeader->addWidget(galleryLabel);
	mainLayout->addLayout(catHeader);

	QScrollArea *chipScroll = new QScrollArea(mainContainer);
	chipScroll->setWidgetResizable(true);
	chipScroll->setFixedHeight(36);
	chipScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	chipScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	chipScroll->setFrameShape(QFrame::NoFrame);

	QWidget *chipContainer = new QWidget(chipScroll);
	categoryLayout_ = new QHBoxLayout(chipContainer);
	categoryLayout_->setContentsMargins(0, 0, 0, 0);
	categoryLayout_->setSpacing(6);

	QStringList categories = {"All",           "African Cinema", "Iconic Movies", "Church & Stage",
				  "Black & White", "Custom"};
	for (const QString &cat : categories) {
		QPushButton *btn = new QPushButton(cat, chipContainer);
		btn->setCheckable(true);
		btn->setChecked(cat == activeCategoryId_);
		btn->setProperty("categoryChip", true);
		connect(btn, &QPushButton::clicked, this, [this, cat]() { onCategorySelected(cat); });
		categoryButtons_.push_back(btn);
		categoryLayout_->addWidget(btn);
	}
	categoryLayout_->addStretch();
	chipScroll->setWidget(chipContainer);
	mainLayout->addWidget(chipScroll);

	// ------------------------------------------------------------------------
	// Gallery Grid (Scrollable)
	// ------------------------------------------------------------------------
	QScrollArea *galleryScroll = new QScrollArea(mainContainer);
	galleryScroll->setWidgetResizable(true);
	galleryScroll->setFrameShape(QFrame::NoFrame);
	galleryScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	galleryContainer_ = new QWidget(galleryScroll);
	galleryLayout_ = new QGridLayout(galleryContainer_);
	galleryLayout_->setContentsMargins(0, 0, 0, 0);
	galleryLayout_->setSpacing(8);

	galleryScroll->setWidget(galleryContainer_);
	mainLayout->addWidget(galleryScroll, 1);

	// ------------------------------------------------------------------------
	// Creative Adjustments Section (Intensity & Melanin Protection)
	// ------------------------------------------------------------------------
	QVBoxLayout *adjustLayout = new QVBoxLayout();
	adjustLayout->setSpacing(6);

	QLabel *adjHeader = new QLabel("CREATIVE CONTROLS", mainContainer);
	adjHeader->setObjectName("sectionHeader");
	adjustLayout->addWidget(adjHeader);

	// Intensity Row
	QHBoxLayout *intensityRow = new QHBoxLayout();
	QLabel *intLabel = new QLabel("Look Mix / Intensity:", mainContainer);
	intLabel->setObjectName("paramLabel");
	intensitySlider_ = new QSlider(Qt::Horizontal, mainContainer);
	intensitySlider_->setRange(0, 100);
	intensitySlider_->setValue(100);
	intensityValueLabel_ = new QLabel("100%", mainContainer);
	intensityValueLabel_->setFixedWidth(40);
	intensityValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	connect(intensitySlider_, &QSlider::valueChanged, this, &LookStudioDock::onIntensityChanged);
	intensityRow->addWidget(intLabel);
	intensityRow->addWidget(intensitySlider_, 1);
	intensityRow->addWidget(intensityValueLabel_);
	adjustLayout->addLayout(intensityRow);

	// Skin Tone Protection Row
	QHBoxLayout *skinRow = new QHBoxLayout();
	QLabel *skinLabel = new QLabel("Melanin Skin Protection:", mainContainer);
	skinLabel->setObjectName("paramLabel");
	skinProtectSlider_ = new QSlider(Qt::Horizontal, mainContainer);
	skinProtectSlider_->setRange(0, 100);
	skinProtectSlider_->setValue(50);
	skinProtectValueLabel_ = new QLabel("50%", mainContainer);
	skinProtectValueLabel_->setFixedWidth(40);
	skinProtectValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	connect(skinProtectSlider_, &QSlider::valueChanged, this, &LookStudioDock::onSkinProtectChanged);
	skinRow->addWidget(skinLabel);
	skinRow->addWidget(skinProtectSlider_, 1);
	skinRow->addWidget(skinProtectValueLabel_);
	adjustLayout->addLayout(skinRow);

	mainLayout->addLayout(adjustLayout);

	// ------------------------------------------------------------------------
	// Comparison & Wipe Controls
	// ------------------------------------------------------------------------
	QHBoxLayout *splitRow = new QHBoxLayout();
	QLabel *splitHeader = new QLabel("A/B Wipe:", mainContainer);
	splitHeader->setObjectName("paramLabel");

	splitModeCombo_ = new QComboBox(mainContainer);
	splitModeCombo_->addItem("Off", 0);
	splitModeCombo_->addItem("Horizontal Split", 1);
	splitModeCombo_->addItem("Vertical Split", 2);
	connect(splitModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&LookStudioDock::onSplitModeChanged);

	wipeSlider_ = new QSlider(Qt::Horizontal, mainContainer);
	wipeSlider_->setRange(0, 100);
	wipeSlider_->setValue(50);
	wipeSlider_->setEnabled(false);
	wipeValueLabel_ = new QLabel("50%", mainContainer);
	wipeValueLabel_->setFixedWidth(40);
	wipeValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	connect(wipeSlider_, &QSlider::valueChanged, this, &LookStudioDock::onWipePosChanged);

	splitRow->addWidget(splitHeader);
	splitRow->addWidget(splitModeCombo_);
	splitRow->addWidget(wipeSlider_, 1);
	splitRow->addWidget(wipeValueLabel_);
	mainLayout->addLayout(splitRow);

	// ------------------------------------------------------------------------
	// Sticky Bottom Action Bar
	// ------------------------------------------------------------------------
	QHBoxLayout *bottomBar = new QHBoxLayout();
	bottomBar->setSpacing(8);

	applyLiveBtn_ = new QPushButton("Apply Live", mainContainer);
	applyLiveBtn_->setObjectName("primaryButton");
	applyLiveBtn_->setFixedHeight(36);
	applyLiveBtn_->setCursor(Qt::PointingHandCursor);
	connect(applyLiveBtn_, &QPushButton::clicked, this, [this]() {
		const LookItem *look = library_->findLookById(activeLookId_);
		if (look) {
			onLookCardClicked(*look);
		}
	});

	importLutBtn_ = new QPushButton("Import .cube…", mainContainer);
	importLutBtn_->setFixedHeight(36);
	connect(importLutBtn_, &QPushButton::clicked, this, &LookStudioDock::onImportLutClicked);

	resetBtn_ = new QPushButton("Reset", mainContainer);
	resetBtn_->setFixedHeight(36);
	connect(resetBtn_, &QPushButton::clicked, this, &LookStudioDock::onResetClicked);

	bottomBar->addWidget(applyLiveBtn_, 2);
	bottomBar->addWidget(importLutBtn_, 1);
	bottomBar->addWidget(resetBtn_, 1);
	mainLayout->addLayout(bottomBar);

	// Hint bar at very bottom
	statusHintLabel_ = new QLabel("Click any look to apply live · Freeze frame captures stacked upstream still",
				      mainContainer);
	statusHintLabel_->setObjectName("hintLabel");
	statusHintLabel_->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(statusHintLabel_);

	setWidget(mainContainer);
}

void LookStudioDock::applyStyling()
{
	QString qss = R"(
		QWidget {
			background-color: #0F1115;
			color: #FFFFFF;
			font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
			font-size: 12px;
		}

		#sectionHeader {
			color: #8C93A0;
			font-size: 10px;
			font-weight: 700;
			letter-spacing: 0.8px;
		}

		#paramLabel {
			color: #C0C5CE;
			font-size: 11px;
			font-weight: 500;
		}

		#hintLabel {
			color: #6C7280;
			font-size: 10px;
		}

		QComboBox {
			background-color: #161920;
			border: 1px solid #222630;
			border-radius: 6px;
			padding: 4px 8px;
			color: #FFFFFF;
			font-size: 11px;
		}
		QComboBox::drop-down {
			border: none;
		}
		QComboBox QAbstractItemView {
			background-color: #161920;
			border: 1px solid #222630;
			color: #FFFFFF;
			selection-background-color: #2A303C;
		}

		QPushButton {
			background-color: #161920;
			border: 1px solid #222630;
			border-radius: 6px;
			padding: 5px 12px;
			color: #FFFFFF;
			font-weight: 600;
		}
		QPushButton:hover {
			background-color: #1E222B;
			border-color: #353B49;
		}
		QPushButton:pressed {
			background-color: #12141A;
		}

		QPushButton[categoryChip="true"] {
			background-color: #161920;
			border: 1px solid #222630;
			border-radius: 12px;
			padding: 3px 10px;
			font-size: 11px;
			color: #8C93A0;
		}
		QPushButton[categoryChip="true"]:hover {
			color: #FFFFFF;
			background-color: #1E222B;
		}
		QPushButton[categoryChip="true"]:checked {
			background-color: #FFFFFF;
			color: #0F1115;
			border: 1px solid #FFFFFF;
			font-weight: 700;
		}

		#primaryButton {
			background-color: #FFFFFF;
			color: #0F1115;
			border: none;
			border-radius: 6px;
			font-weight: 700;
			font-size: 12px;
		}
		#primaryButton:hover {
			background-color: #E2E4E8;
		}
		#primaryButton:pressed {
			background-color: #C8CBD0;
		}

		QSlider::groove:horizontal {
			height: 4px;
			background: #222630;
			border-radius: 2px;
		}
		QSlider::sub-page:horizontal {
			background: #FFFFFF;
			border-radius: 2px;
		}
		QSlider::handle:horizontal {
			background: #FFFFFF;
			border: 1px solid #161920;
			width: 14px;
			margin-top: -5px;
			margin-bottom: -5px;
			border-radius: 7px;
		}

		QScrollBar:vertical {
			background: #0F1115;
			width: 6px;
			margin: 0px;
		}
		QScrollBar::handle:vertical {
			background: #222630;
			min-height: 20px;
			border-radius: 3px;
		}
		QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
			height: 0px;
		}
	)";
	setStyleSheet(qss);
}

void LookStudioDock::updateGalleryCards()
{
	// Clear previous cards
	for (auto *card : cardWidgets_) {
		galleryLayout_->removeWidget(card);
		delete card;
	}
	cardWidgets_.clear();

	std::vector<LookItem> looks = library_->getLooksByCategory(activeCategoryId_);

	int cols = 2; // Clean 2-column grid in vertical dock
	int row = 0;
	int col = 0;

	for (const auto &look : looks) {
		LookCardWidget *card = new LookCardWidget(look, galleryContainer_);
		card->setIsLive(look.id.compare(activeLookId_, Qt::CaseInsensitive) == 0);

		float intensity = intensitySlider_ ? (intensitySlider_->value() / 100.0f) : 1.0f;
		float skinProtect = skinProtectSlider_ ? (skinProtectSlider_->value() / 100.0f) : 0.5f;
		QImage thumb = snapshotMgr_->getThumbnailForLook(look, intensity, skinProtect);
		card->setThumbnail(thumb);

		connect(card, &LookCardWidget::clicked, this, &LookStudioDock::onLookCardClicked);

		galleryLayout_->addWidget(card, row, col);
		cardWidgets_.push_back(card);

		col++;
		if (col >= cols) {
			col = 0;
			row++;
		}
	}
}

void LookStudioDock::onThumbnailsRegenerated()
{
	float intensity = intensitySlider_ ? (intensitySlider_->value() / 100.0f) : 1.0f;
	float skinProtect = skinProtectSlider_ ? (skinProtectSlider_->value() / 100.0f) : 0.5f;

	for (auto *card : cardWidgets_) {
		QImage thumb = snapshotMgr_->getThumbnailForLook(card->getLookItem(), intensity, skinProtect);
		card->setThumbnail(thumb);
	}
}

void LookStudioDock::onCategorySelected(const QString &category)
{
	activeCategoryId_ = category;
	for (auto *btn : categoryButtons_) {
		btn->setChecked(btn->text() == category);
	}
	updateGalleryCards();
}

void LookStudioDock::onLookCardClicked(const LookItem &item)
{
	activeLookId_ = item.id;
	for (auto *card : cardWidgets_) {
		card->setIsLive(card->getLookItem().id == activeLookId_);
	}

	obs_source_t *source = getSelectedSource();
	look_filter_apply_look(source, item.filePath.toUtf8().constData());
	statusHintLabel_->setText(QString("Applied Look: %1 (%2)").arg(item.title, item.category));
}

void LookStudioDock::onIntensityChanged(int value)
{
	float intensity = value / 100.0f;
	intensityValueLabel_->setText(QString("%1%").arg(value));

	obs_source_t *source = getSelectedSource();
	look_filter_set_intensity(source, intensity);
}

void LookStudioDock::onSkinProtectChanged(int value)
{
	float skinProtect = value / 100.0f;
	skinProtectValueLabel_->setText(QString("%1%").arg(value));

	obs_source_t *source = getSelectedSource();
	look_filter_set_skin_protect(source, skinProtect);
}

void LookStudioDock::onSplitModeChanged(int index)
{
	int mode = splitModeCombo_->itemData(index).toInt();
	wipeSlider_->setEnabled(mode != 0);

	float wipePos = wipeSlider_->value() / 100.0f;
	obs_source_t *source = getSelectedSource();
	look_filter_set_split_mode(source, mode, wipePos);
}

void LookStudioDock::onWipePosChanged(int value)
{
	wipeValueLabel_->setText(QString("%1%").arg(value));
	int mode = splitModeCombo_->currentData().toInt();
	float wipePos = value / 100.0f;

	obs_source_t *source = getSelectedSource();
	look_filter_set_split_mode(source, mode, wipePos);
}

void LookStudioDock::onCaptureStillClicked()
{
	obs_source_t *source = getSelectedSource();
	look_filter_request_snapshot(source);
	statusHintLabel_->setText("Capturing still from upstream filter stack…");
}

void LookStudioDock::onImportLutClicked()
{
	QString path =
		QFileDialog::getOpenFileName(this, "Import 3D LUT", "", "Cube LUT (*.cube *.CUBE);;All Files (*.*)");
	if (path.isEmpty()) {
		return;
	}

	QString newPath;
	if (library_->importCustomLook(path, newPath)) {
		statusHintLabel_->setText("Custom LUT imported successfully!");
		onCategorySelected("Custom");
	} else {
		statusHintLabel_->setText("Failed to import LUT file.");
	}
}

void LookStudioDock::onResetClicked()
{
	activeLookId_.clear();
	for (auto *card : cardWidgets_) {
		card->setIsLive(false);
	}

	intensitySlider_->setValue(100);
	skinProtectSlider_->setValue(50);
	splitModeCombo_->setCurrentIndex(0);

	obs_source_t *source = getSelectedSource();
	look_filter_apply_look(source, "");
	statusHintLabel_->setText("Look reset to neutral passthrough.");
}

void LookStudioDock::refreshSources()
{
	sourceCombo_->clear();
	sourceCombo_->addItem("All Active Camera & Scene Filters", QVariant::fromValue(static_cast<void *>(nullptr)));

	// Enumerate sources that have ld_look_studio_filter attached
	auto enumCallback = [](void *param, obs_source_t *source) -> bool {
		QComboBox *combo = static_cast<QComboBox *>(param);
		if (!source)
			return true;

		obs_source_t *filter = obs_source_get_filter_by_name(source, "LD Look Studio");
		if (filter) {
			combo->addItem(QString::fromUtf8(obs_source_get_name(source)),
				       QVariant::fromValue(static_cast<void *>(source)));
			obs_source_release(filter);
		}
		return true;
	};

	obs_enum_sources(enumCallback, sourceCombo_);
}

void LookStudioDock::onSourceChanged(int index)
{
	Q_UNUSED(index);
}

obs_source_t *LookStudioDock::getSelectedSource() const
{
	if (!sourceCombo_ || sourceCombo_->currentIndex() <= 0) {
		return nullptr; // null applies to all active filters
	}
	return static_cast<obs_source_t *>(sourceCombo_->currentData().value<void *>());
}
