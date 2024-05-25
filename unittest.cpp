
#include <gtest/gtest.h>
#include "stridx.hpp"
#include <cmath>

TEST(SplitString, MatchSize) {
  std::vector<std::string> svec = StrIdx::splitString("foo/bar/test1.txt", '/');
  EXPECT_EQ(svec.size(), 3);
  if (svec.size() == 3) {
    EXPECT_EQ(svec[0].size(), 3);
    EXPECT_EQ(svec[2].size(), 9);
  }
}

std::vector<std::string> flist{"./drivers/char/hw_random/nomadik-rng.c",
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
                               "./drivers/pinctrl/nomadik/pinctrl-nomadik-stn8815.c",
                               "./drivers/char/hw_random/omap-rng.c",
                               "./drivers/char/hw_random/omap3-rom-rng.c",
                               "./include/dt-bindings/pinctrl/nomadik.h",
                               "./Documentation/devicetree/bindings/arm/ste-nomadik.txt"};

std::vector<float> target_scores{0.342944, 0.271396, 0.271126, 0.270893, 0.270431, 0.270355,
                                 0.270088, 0.270088, 0.26987,  0.269776, 0.269574, 0.269538,
                                 0.236358, 0.236074, 0.224804, 0.224238};

void scoreTest(bool threaded) {

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
  const std::vector<std::pair<float, int>> &results = idx.findSimilar(query);

  std::cout << results[0].first;
  EXPECT_EQ(results[0].second, 1);
  if (results.size() == 16) {
    int i = 0;
    for (const auto &res : results) {
      // Check if first five digits of the scores match
      EXPECT_EQ(std::floor(res.first * 1e5), std::floor(1e5 * target_scores[i]));
      i++;
    }
  }
}

TEST(IndexSearch, MatchingScoresSingleThread) { scoreTest(false); }
TEST(IndexSearch, MatchingScoresThreaded) { scoreTest(true); }

TEST(IndexSearch, BinaryRepresentation) {

  auto idx = StrIdx::StringIndex();
  {
    int64_t num = idx.getKeyAtIdx("abcdefgh", 0, 8);
    std::string s = StrIdx::int64ToBinaryString(num);
    EXPECT_TRUE(s == "0110000101100010011000110110010001100101011001100110011101101000");
  }

  {
    int64_t num = idx.getKeyAtIdx("abcdefgh", 0, 1);
    std::string s = StrIdx::int64ToBinaryString(num);
    EXPECT_TRUE(s == "0000000000000000000000000000000000000000000000000000000001100001");
  }
  {
    int64_t num = idx.getKeyAtIdx("abcdefgh", 7, 1);
    std::string s = StrIdx::int64ToBinaryString(num);
    EXPECT_TRUE(s == "0000000000000000000000000000000000000000000000000000000001101000");
  }
}
