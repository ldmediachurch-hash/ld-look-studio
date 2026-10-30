#include "look-library.hpp"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

LookLibrary::LookLibrary(QObject *parent) : QObject(parent) {}

void LookLibrary::initialize(const QString &pluginDataPath)
{
	pluginDataPath_ = pluginDataPath;
	refresh();
}

QString LookLibrary::getUserLooksPath() const
{
	QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	if (base.isEmpty()) {
		base = QDir::homePath() + "/.config/obs-studio";
	}
	QString customDir = base + "/plugin_config/ld-look-studio/custom-looks";
	QDir().mkpath(customDir);
	return customDir;
}

void LookLibrary::refresh()
{
	looks_.clear();

	// 1. Scan built-in looks in plugin data path
	if (!pluginDataPath_.isEmpty()) {
		QString looksDir = pluginDataPath_ + "/looks";
		scanDirectory(looksDir, true);
	}

	// 2. Scan user custom looks directory
	QString userDir = getUserLooksPath();
	scanDirectory(userDir, false);

	emit libraryChanged();
}

void LookLibrary::scanDirectory(const QString &dirPath, bool isBuiltIn)
{
	QDir dir(dirPath);
	if (!dir.exists()) {
		return;
	}

	QStringList filters;
	filters << "*.cube" << "*.CUBE";
	QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

	for (const QFileInfo &fileInfo : files) {
		LookItem item;
		item.filePath = fileInfo.absoluteFilePath();
		item.id = fileInfo.baseName().toLower();
		item.isBuiltIn = isBuiltIn;

		if (isBuiltIn) {
			assignMetadata(item);
		} else {
			item.category = "Custom";
			item.title = fileInfo.baseName().replace('-', ' ').replace('_', ' ');
			item.description = "User imported 3D LUT";
		}

		looks_.push_back(item);
	}
}

void LookLibrary::assignMetadata(LookItem &item) const
{
	QString base = QFileInfo(item.filePath).baseName();

	if (base.compare("Afro-Glow", Qt::CaseInsensitive) == 0) {
		item.title = "Afro Glow";
		item.category = "African Cinema";
		item.description = "Warm amber & golden undertones preserving deep bronze melanin.";
	} else if (base.compare("Rich-Mahogany", Qt::CaseInsensitive) == 0) {
		item.title = "Rich Mahogany";
		item.category = "African Cinema";
		item.description = "Deep velvet contrast for darker skin tones, prevents ashen cast.";
	} else if (base.compare("Nollywood-Prestige", Qt::CaseInsensitive) == 0) {
		item.title = "Nollywood Prestige";
		item.category = "African Cinema";
		item.description = "Vibrant modern cinema grade, punchy primaries & royal fidelity.";
	} else if (base.compare("Golden-Hour-Lagos", Qt::CaseInsensitive) == 0) {
		item.title = "Golden Hour Lagos";
		item.category = "African Cinema";
		item.description = "Sun-drenched equatorial warmth, rich golden highlights.";
	} else if (base.compare("Wakanda-Royal-Gold", Qt::CaseInsensitive) == 0) {
		item.title = "Wakanda Royal Gold";
		item.category = "Iconic Movies";
		item.description = "Black Panther aesthetic: opulent regal gold & indigo shadows.";
	} else if (base.compare("Moonlight-Velvet", Qt::CaseInsensitive) == 0) {
		item.title = "Moonlight Velvet";
		item.category = "Iconic Movies";
		item.description = "Moonlight aesthetic: deep sapphire night & luminous velvet skin.";
	} else if (base.compare("Insecure-Glow", Qt::CaseInsensitive) == 0) {
		item.title = "Insecure Glow";
		item.category = "Iconic Movies";
		item.description = "Ava Berkofsky style: radiant moisturized skin & soft lifted shadows.";
	} else if (base.compare("Dune-Amber", Qt::CaseInsensitive) == 0) {
		item.title = "Dune Amber";
		item.category = "Iconic Movies";
		item.description = "Greig Fraser aesthetic: muted cool tones & warm desert amber.";
	} else if (base.compare("Kodak-2383-Print", Qt::CaseInsensitive) == 0) {
		item.title = "Kodak 2383 Print";
		item.category = "Iconic Movies";
		item.description = "Hollywood 35mm print stock: subtractive color & cyan shadow rolloff.";
	} else if (base.compare("Matrix-Cyber-Noir", Qt::CaseInsensitive) == 0) {
		item.title = "Matrix Cyber Noir";
		item.category = "Iconic Movies";
		item.description = "Cyberpunk noir: olive green shadow bias & protected warm skin.";
	} else if (base.compare("Miami-Vice-Teal-Orange", Qt::CaseInsensitive) == 0) {
		item.title = "Miami Teal & Orange";
		item.category = "Iconic Movies";
		item.description = "Complementary teal backgrounds with punchy amber skin.";
	} else if (base.compare("Sunday-Clean-Rec709", Qt::CaseInsensitive) == 0) {
		item.title = "Sunday Clean Rec709";
		item.category = "Church & Stage";
		item.description = "True-to-life broadcast color with soft knee highlight compression.";
	} else if (base.compare("Warm-Pulpit", Qt::CaseInsensitive) == 0) {
		item.title = "Warm Pulpit";
		item.category = "Church & Stage";
		item.description = "Neutralizes cool stage LED spotlights & protects specular highlights.";
	} else if (base.compare("Stage-Punch", Qt::CaseInsensitive) == 0) {
		item.title = "Stage Punch";
		item.category = "Church & Stage";
		item.description = "High micro-contrast & punchy saturation for flat camera sensors.";
	} else if (base.compare("Noir-Silver-Print", Qt::CaseInsensitive) == 0) {
		item.title = "Noir Silver Print";
		item.category = "Black & White";
		item.description = "Panchromatic silver gelatin B&W with rich dark skin gradation.";
	} else if (base.compare("Soft-Sepia-Vintage", Qt::CaseInsensitive) == 0) {
		item.title = "Soft Sepia Vintage";
		item.category = "Black & White";
		item.description = "Warm antique sepia split for acoustic worship & reflective moments.";
	} else {
		item.title = base.replace('-', ' ').replace('_', ' ');
		item.category = "Iconic Movies";
		item.description = "Cinematic 3D LUT preset.";
	}
}

std::vector<LookItem> LookLibrary::getLooksByCategory(const QString &category) const
{
	if (category.isEmpty() || category.compare("All", Qt::CaseInsensitive) == 0) {
		return looks_;
	}

	std::vector<LookItem> filtered;
	for (const auto &look : looks_) {
		if (look.category.compare(category, Qt::CaseInsensitive) == 0) {
			filtered.push_back(look);
		}
	}
	return filtered;
}

QStringList LookLibrary::getCategories() const
{
	return {"All", "African Cinema", "Iconic Movies", "Church & Stage", "Black & White", "Custom"};
}

const LookItem *LookLibrary::findLookById(const QString &id) const
{
	for (const auto &look : looks_) {
		if (look.id.compare(id, Qt::CaseInsensitive) == 0) {
			return &look;
		}
	}
	return nullptr;
}

const LookItem *LookLibrary::findLookByPath(const QString &filePath) const
{
	for (const auto &look : looks_) {
		if (QFileInfo(look.filePath).absoluteFilePath() == QFileInfo(filePath).absoluteFilePath()) {
			return &look;
		}
	}
	return nullptr;
}

bool LookLibrary::importCustomLook(const QString &sourcePath, QString &outNewPath)
{
	QFileInfo srcInfo(sourcePath);
	if (!srcInfo.exists() || !srcInfo.isFile()) {
		return false;
	}

	QString userDir = getUserLooksPath();
	QString targetPath = userDir + "/" + srcInfo.fileName();

	if (QFile::exists(targetPath)) {
		QFile::remove(targetPath);
	}

	if (QFile::copy(sourcePath, targetPath)) {
		outNewPath = targetPath;
		refresh();
		return true;
	}

	return false;
}

std::shared_ptr<CubeLut> LookLibrary::getLutData(const LookItem &item) const
{
	if (item.lutData && item.lutData->isValid()) {
		return item.lutData;
	}

	auto lut = std::make_shared<CubeLut>();
	if (CubeParser::loadCubeFile(item.filePath, *lut)) {
		item.lutData = lut;
		return lut;
	}

	return nullptr;
}
