#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "cube-parser.hpp"
#include "half-float.hpp"
#include "look-library.hpp"

#define TEST_ASSERT(cond)                                                          \
	do {                                                                       \
		if (!(cond)) {                                                     \
			std::cerr << "Assertion failed: " #cond << " at "          \
				  << __FILE__ << ":" << __LINE__ << std::endl;     \
			std::exit(1);                                              \
		}                                                                  \
	} while (0)

static void testSyntheticCubeParsing()
{
	std::string synthCube = "# Synthetic Test 2x2x2 Cube LUT\n"
				"TITLE \"Synthetic Identity\"\n"
				"LUT_3D_SIZE 2\n"
				"DOMAIN_MIN 0.0 0.0 0.0\n"
				"DOMAIN_MAX 1.0 1.0 1.0\n"
				"0.0 0.0 0.0\n"
				"1.0 0.0 0.0\n"
				"0.0 1.0 0.0\n"
				"1.0 1.0 0.0\n"
				"0.0 0.0 1.0\n"
				"1.0 0.0 1.0\n"
				"0.0 1.0 1.0\n"
				"1.0 1.0 1.0\n";

	CubeLut lut;
	bool success = CubeParser::loadCubeFromString(synthCube, lut);
	TEST_ASSERT(success);
	TEST_ASSERT(lut.title == "Synthetic Identity");
	TEST_ASSERT(lut.size == 2);
	TEST_ASSERT(lut.isValid());

	// Test boundary sampling
	float r = 0, g = 0, b = 0;
	lut.sample(0.0f, 0.0f, 0.0f, r, g, b);
	TEST_ASSERT(std::abs(r - 0.0f) < 0.001f);
	TEST_ASSERT(std::abs(g - 0.0f) < 0.001f);
	TEST_ASSERT(std::abs(b - 0.0f) < 0.001f);

	lut.sample(1.0f, 1.0f, 1.0f, r, g, b);
	TEST_ASSERT(std::abs(r - 1.0f) < 0.001f);
	TEST_ASSERT(std::abs(g - 1.0f) < 0.001f);
	TEST_ASSERT(std::abs(b - 1.0f) < 0.001f);

	// Test midpoint trilinear interpolation
	lut.sample(0.5f, 0.5f, 0.5f, r, g, b);
	TEST_ASSERT(std::abs(r - 0.5f) < 0.01f);
	TEST_ASSERT(std::abs(g - 0.5f) < 0.01f);
	TEST_ASSERT(std::abs(b - 0.5f) < 0.01f);

	std::cout << "✓ Synthetic cube parsing & trilinear interpolation test passed." << std::endl;
}

static void testPresetLuts(const QString &dataPath)
{
	QDir looksDir(dataPath + "/looks");
	TEST_ASSERT(looksDir.exists());

	QStringList files = looksDir.entryList(QStringList() << "*.cube", QDir::Files);
	TEST_ASSERT(!files.isEmpty());
	std::cout << "Found " << files.size() << " built-in .cube LUTs to verify..." << std::endl;

	for (const QString &file : files) {
		QString fullPath = looksDir.absoluteFilePath(file);
		CubeLut lut;
		bool ok = CubeParser::loadCubeFile(fullPath, lut);
		TEST_ASSERT(ok);
		TEST_ASSERT(lut.isValid());
		TEST_ASSERT(lut.size == 33);

		// Sample test in dark melanin skin tone region (R=0.35, G=0.22, B=0.15)
		float r = 0, g = 0, b = 0;
		lut.sample(0.35f, 0.22f, 0.15f, r, g, b);
		TEST_ASSERT(!std::isnan(r) && !std::isnan(g) && !std::isnan(b));
		TEST_ASSERT(r >= 0.0f && r <= 1.0f);
		TEST_ASSERT(g >= 0.0f && g <= 1.0f);
		TEST_ASSERT(b >= 0.0f && b <= 1.0f);
	}

	std::cout << "✓ All " << files.size() << " built-in .cube LUT files verified successfully." << std::endl;
}

static void testLookLibrary(const QString &dataPath)
{
	LookLibrary lib;
	lib.initialize(dataPath);

	auto allLooks = lib.getAllLooks();
	TEST_ASSERT(allLooks.size() >= 16);

	// Verify Wakanda Royal Gold
	const LookItem *wakanda = lib.findLookById("wakanda-royal-gold");
	TEST_ASSERT(wakanda != nullptr);
	TEST_ASSERT(wakanda->category == "Iconic Movies");
	TEST_ASSERT(wakanda->title == "Wakanda Royal Gold");

	// Verify Afro Glow
	const LookItem *afro = lib.findLookById("afro-glow");
	TEST_ASSERT(afro != nullptr);
	TEST_ASSERT(afro->category == "African Cinema");

	// Verify categories
	auto movieLooks = lib.getLooksByCategory("Iconic Movies");
	TEST_ASSERT(!movieLooks.empty());

	auto africanLooks = lib.getLooksByCategory("African Cinema");
	TEST_ASSERT(africanLooks.size() >= 4);

	std::cout << "✓ LookLibrary catalog & categorization test passed." << std::endl;
}

static void testHalfFloatConversion()
{
	struct ld_half zero = ld_half_from_float(0.0f);
	TEST_ASSERT(zero.u == 0x0000);

	struct ld_half one = ld_half_from_float(1.0f);
	TEST_ASSERT(one.u == 0x3c00);

	struct ld_half half_val = ld_half_from_float(0.5f);
	TEST_ASSERT(half_val.u == 0x3800);

	struct ld_half raw = ld_half_from_bits(0x3c00);
	TEST_ASSERT(raw.u == 0x3c00);

	std::cout << "✓ 16-bit Half-Float precision conversion test passed." << std::endl;
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);

	std::cout << "===========================================" << std::endl;
	std::cout << "Running LD Look Studio Test Suite..." << std::endl;
	std::cout << "===========================================" << std::endl;

	testHalfFloatConversion();
	testSyntheticCubeParsing();

	QString dataPath = QDir::currentPath() + "/data";
	if (!QDir(dataPath).exists()) {
		dataPath = app.applicationDirPath() + "/../data";
	}
	if (!QDir(dataPath).exists()) {
		dataPath = "/Users/GeneralD/.gemini/antigravity-ide/scratch/lastdmedia/ld-look-studio/data";
	}

	testPresetLuts(dataPath);
	testLookLibrary(dataPath);

	std::cout << "===========================================" << std::endl;
	std::cout << "ALL 3 TEST SUITES PASSED CLEANLY (100% OK)!" << std::endl;
	std::cout << "===========================================" << std::endl;
	return 0;
}
