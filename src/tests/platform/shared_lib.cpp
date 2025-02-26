#include "platform/shared_lib.hpp"

#include <filesystem>
#include <string>

using namespace sndx;

#include "../common.hpp"

#define FILENAME_BASE "test_data/binary/testlib"

#ifdef _WIN32
#define FILENAME_EXT ".dll"
#else
#define FILENAME_EXT ".so"
#endif

constexpr const char* filename = FILENAME_BASE FILENAME_EXT;
constexpr const char* badFilename = ".NOT_A_REAL_FILE.;-;";

typedef void(*NO_ARGS)();
typedef int(*ADD_ONE)(int);
typedef int(*INCREMENT)();
typedef bool(*ADVANCED)(const char*, unsigned int);

void stubfunc() {}

class SharedLibTest : public ::testing::Test {
public:
	void SetUp() override {
		set_test_weight(TestWeight::BasicIntegration)
	}
};

TEST_F(SharedLibTest, CanFailToLoadLib) {
	platform::SharedLib lib{ badFilename };
	EXPECT_TRUE(!lib.valid());
}

class GoodSharedLibTest : public ::testing::Test {
public:
	void SetUp() override {
		set_test_weight(TestWeight::BasicIntegration)
		else {
			const auto& info = testing::UnitTest::GetInstance()->current_test_info();
			
			tempname = FILENAME_BASE + std::to_string(info->line()) + FILENAME_EXT;
			
			if (std::filesystem::copy_file(filename, tempname, std::filesystem::copy_options::overwrite_existing)) {
				lib = std::make_unique<platform::SharedLib>(tempname.c_str());

				if (!lib) {
					GTEST_FAIL() << "Failed to load shared lib";
				}
			}
			else {
				GTEST_SKIP() << "Failed to copy shared library to temp library";
			}
		}
	}

	void TearDown() override {
		if (lib) {
			lib.reset();
		}

		if (!tempname.empty() && std::filesystem::exists(tempname)) {
			// this will fail if the dll/so is still loaded
			std::filesystem::remove(std::exchange(tempname, ""));
		}
	}

	std::string tempname = "";
	std::unique_ptr<platform::SharedLib> lib{};
};

TEST_F(GoodSharedLibTest, CanLoadFuncs) {
	ASSERT_TRUE(lib->valid());

	EXPECT_NE(lib->load("no_args"), nullptr);
	EXPECT_NE(lib->load("add_one"), nullptr);
	EXPECT_NE(lib->load("increment"), nullptr);
	EXPECT_NE(lib->load("advanced"), nullptr);

	EXPECT_EQ(lib->load("this function doesn't exist"), nullptr);
}

TEST_F(GoodSharedLibTest, LoadedFuncsWork) {
	ASSERT_TRUE(lib->valid());

	ADD_ONE add_one = (ADD_ONE)lib->load("add_one");
	ASSERT_NE(add_one, nullptr);

	EXPECT_EQ(add_one(1), 2);

	INCREMENT increment = (INCREMENT)lib->load("increment");
	ASSERT_NE(increment, nullptr);

	EXPECT_EQ(increment(), 1);
	EXPECT_EQ(increment(), 2);
	EXPECT_EQ(increment(), 3);

	ADVANCED advanced = (ADVANCED)lib->load("advanced");
	ASSERT_NE(advanced, nullptr);

	EXPECT_FALSE(advanced(nullptr, 1));
	EXPECT_FALSE(advanced("hi", 4));

	EXPECT_TRUE(advanced("", 0));
	EXPECT_TRUE(advanced("hello", 5));
}

TEST_F(GoodSharedLibTest, StaticsResetOnReload) {
	ASSERT_TRUE(lib->valid());

	INCREMENT increment = (INCREMENT)lib->load("increment");
	ASSERT_NE(increment, nullptr);

	EXPECT_EQ(increment(), 1);
	EXPECT_EQ(increment(), 2);
	EXPECT_EQ(increment(), 3);

	lib.reset();
	lib = std::make_unique<platform::SharedLib>(this->tempname.c_str());

	increment = (INCREMENT)lib->load("increment");
	ASSERT_NE(increment, nullptr);

	EXPECT_EQ(increment(), 1);
	EXPECT_EQ(increment(), 2);
	EXPECT_EQ(increment(), 3);
}

TEST_F(GoodSharedLibTest, LibLoaderLoads) {
	ASSERT_TRUE(lib->valid());

	NO_ARGS no_args{};
	ADD_ONE add_one{};
	INCREMENT increment{};
	ADVANCED advanced{};
	NO_ARGS fake{};

	platform::LibLoader loader{};
	loader.bind("no_args", no_args, stubfunc);
	loader.bind("add_one", add_one, nullptr);
	loader.bind("increment", increment, nullptr);
	loader.bind("advanced", advanced, nullptr);
	loader.bind("this does not exist", fake, stubfunc);
	
	EXPECT_EQ(loader.load(*lib), 1);

	EXPECT_NE(no_args, nullptr);
	EXPECT_NE(no_args, stubfunc);
	EXPECT_NE(add_one, nullptr);
	EXPECT_NE(increment, nullptr);
	EXPECT_NE(advanced, nullptr);
	EXPECT_EQ(fake, stubfunc);

	lib->close();

	EXPECT_EQ(loader.load(*lib), 5);

	EXPECT_EQ(no_args, stubfunc);
	EXPECT_EQ(add_one, nullptr);
	EXPECT_EQ(increment, nullptr);
	EXPECT_EQ(advanced, nullptr);
	EXPECT_EQ(fake, stubfunc);
}