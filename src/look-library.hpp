#pragma once

#include <memory>
#include <vector>

#include <QObject>
#include <QString>
#include <QStringList>

#include "cube-parser.hpp"

struct LookItem {
	QString id;
	QString title;
	QString category;
	QString filePath;
	QString description;
	bool isBuiltIn = false;
	mutable std::shared_ptr<CubeLut> lutData;
};

class LookLibrary : public QObject {
	Q_OBJECT

public:
	explicit LookLibrary(QObject *parent = nullptr);

	void initialize(const QString &pluginDataPath);
	void refresh();

	const std::vector<LookItem> &getAllLooks() const { return looks_; }
	std::vector<LookItem> getLooksByCategory(const QString &category) const;
	QStringList getCategories() const;

	const LookItem *findLookById(const QString &id) const;
	const LookItem *findLookByPath(const QString &filePath) const;

	bool importCustomLook(const QString &sourcePath, QString &outNewPath);
	QString getUserLooksPath() const;

	std::shared_ptr<CubeLut> getLutData(const LookItem &item) const;

signals:
	void libraryChanged();

private:
	void scanDirectory(const QString &dirPath, bool isBuiltIn);
	void assignMetadata(LookItem &item) const;

	QString pluginDataPath_;
	std::vector<LookItem> looks_;
};
