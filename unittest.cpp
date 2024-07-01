
#include <gtest/gtest.h>
#include "stridx.hpp"
#include <cmath>
#include <memory>
#include <chrono>
#include <thread>

TEST(SplitString, MatchSize) {
  std::vector<std::string> svec = StrIdx::splitString("foo/bar/test1.txt", '/');
  EXPECT_EQ(svec.size(), 3);
  if (svec.size() == 3) {
    EXPECT_EQ(svec[0].size(), 3);
    EXPECT_EQ(svec[2].size(), 9);
  }
}

std::vector<std::string> flist{
                               "./drivers/char/hw_random/nomadik-rng.c",
                               "./drivers/pinctrl/nomadik",
                               "./drivers/clk/clk-nomadik.c",
                               "./drivers/gpio/gpio-nomadik.c",
                               "./drivers/i2c/busses/i2c-nomadik.c",
                               "./drivers/clocksource/nomadik-mtu.c",
                               "./drivers/gpu/drm/pl111/pl111_nomadik.h",
                               "./drivers/gpu/drm/pl111/pl111_nomadik.c",
                               "./drivers/pinctrl/nomadik/pinctrl-nomadik.c",
                               "./drivers/input/keyboard/nomadik-ske-keypad.c",
                               "./drivers/pinctrl/nomadik/pinctrl-nomadik-db8500.c",
                               "./drivers/pinctrl/nomadik/pinctrl-nomadik-stn8816.c",
                               "./drivers/char/hw_random/omap-rng.c",
                               "./drivers/char/hw_random/omap3-rom-rng.c",
                               "./include/dt-bindings/pinctrl/nomadik.h",
                               "./Documentation/devicetree/bindings/arm/ste-nomadik.txt"};

std::vector<float> target_scores{0.342944, 0.271396,  0.271126, 0.270893, 0.270431, 0.270355,
                                 0.270088, 0.270088, 0.26987,  0.269776, 0.269574, 0.269538,
                                 0.236358, 0.236074, 0.224804, 0.224238};

void createIndex(bool threaded) {

  StrIdx::StringIndex idx('/'); // Separate directories using unix style "/" char
  std::string query = "rngnomadriv";

  int i = 1;
  for (const auto &str : flist) {
    if (threaded && i > 2) {
      idx.addStrToIndexThreaded(str, i);
    } else {
      idx.addStrToIndex(str, i);
    }
    i++;
  }

  idx.waitUntilReady();

  EXPECT_EQ(idx.size(), 16);
}

void scoreTest(bool threaded, bool runSearch) {

  StrIdx::StringIndex idx('/'); // Separate directories using unix style "/" char
  std::string query = "rngnomadriv";

  int i = 1;
  for (const auto &str : flist) {
    if (threaded) {
      idx.addStrToIndexThreaded(str, i);
    } else {
      idx.addStrToIndex(str, i);
    }
    i++;
  }

  idx.waitUntilReady();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  std::cout << "s:" << idx.size() << ":";
  EXPECT_EQ(idx.size(), 16);

  if (runSearch) {
    const std::vector<std::pair<float, int>> &results = idx.findSimilar(query);

    std::cout << results[0].first;
    EXPECT_EQ(results[0].second, 1);
    if (results.size() == 16) {
      int i = 0;
      for (const auto &res : results) {
        // Check if first five digits of the scores match
        std::cout<< "{" << res.first << " " << target_scores[i] << "\n";
        EXPECT_EQ(std::floor(res.first * 1e5), std::floor(1e5 * target_scores[i]));
        i++;
      }
    }
  }
  
  // EXPECT_EQ(0,1);
}

TEST(Index, Create) { createIndex(false); }
TEST(Index, CreateThreaded) { createIndex(true); }
TEST(IndexSearch, MatchingScoresSingleThread) {
  scoreTest(false, true);
}
TEST(IndexSearch, MatchingScoresThreaded) {
  for (int i = 0; i < 3; i++) {
    scoreTest(true, true);
  }
}

class IndexTest : public testing::Test {
protected:
  std::unique_ptr<StrIdx::StringIndex> idx = std::make_unique<StrIdx::StringIndex>('/');

  IndexTest() {}

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
    idx = std::make_unique<StrIdx::StringIndex>('/');
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }
};

TEST_F(IndexTest, BinaryRepresentation1) {
  int64_t num = idx->getKeyAtIdx("abcdefgh", 0, 8);
  std::string s = StrIdx::int64ToBinaryString(num);
  //                a       b       c       d ...
  EXPECT_TRUE(s == "0110000101100010011000110110010001100101011001100110011101101000");
}

TEST_F(IndexTest, BinaryRepresentation2) {
  int64_t num = idx->getKeyAtIdx("abcdefgh", 0, 1);
  std::string s = StrIdx::int64ToBinaryString(num);
  EXPECT_TRUE(
      s == "0000000000000000000000000000000000000000000000000000000001100001"); // 01100001 == "a"
}
TEST_F(IndexTest, BinaryRepresentation3) {
  int64_t num = idx->getKeyAtIdx("abcdefgh", 7, 1);
  std::string s = StrIdx::int64ToBinaryString(num);
  EXPECT_TRUE(
      s == "0000000000000000000000000000000000000000000000000000000001101000"); // 01101000 == "h"
}

TEST_F(IndexTest, AccessString) {
  idx->addStrToIndex("./drivers/i2c/busses/i2c-nomadik.c", 0);
  idx->addStrToIndex("./drivers/i2c/busses/i2c-nomadiksdf.c", 2);
  idx->addStrToIndex("./drivers//i2c///busses////aa-i2c-nomadiksdf.c", 3);
  idx->addStrToIndex("/test/foo/bar.txt", 4);
  idx->addStrToIndex("bar.txt", 5);

  EXPECT_EQ(idx->size(), 5);
  EXPECT_STREQ(idx->getString(0).c_str(), "./drivers/i2c/busses/i2c-nomadik.c");

  // TODO: does not work yet
  //  EXPECT_STREQ(idx->getString(3).c_str(), "./drivers//i2c///busses////aa-i2c-nomadiksdf.c");

  // TODO: does not work yet
  //  EXPECT_STREQ(idx->getString(4).c_str(), "/test/foo/bar.txt");
  EXPECT_STREQ(idx->getString(5).c_str(), "bar.txt");
}

TEST_F(IndexTest, Size1) {
  // Should not add different files with same id
  idx->addStrToIndex("./drivers/i2c/busses/i2c-nomadik.c", 0);
  idx->addStrToIndex("./drivers/i2c/busses/i2c-nomadiksdf.c", 0);

  // Should not be added because essentially same as 0:
  idx->addStrToIndex("./drivers//i2c///busses////i2c-nomadik.c", 44);

  // Should not add same file with different id
  idx->addStrToIndex("./drivers/i2c/busses/i2c-nomadik.c", 1);
  idx->addStrToIndex("./drivers/i2c/busses/i2c-nomadik.c", 2);

  EXPECT_EQ(idx->size(), 1);

  // Test no-overwriting
  EXPECT_STREQ(idx->getString(0).c_str(), "./drivers/i2c/busses/i2c-nomadik.c");
}

TEST_F(IndexTest, Size2) {
  idx->addStrToIndex("./drivers/i2c/busses/i2c-nomadik.c", 22);
  idx->addStrToIndex("./Documentation/devicetree/bindings/arm/ste-nomadik.txt", 1);
  idx->addStrToIndex("./Documentation/devicetree/bindings/arm/ste-nomadik33.txt", 3);
  EXPECT_EQ(idx->size(), 3);
}
