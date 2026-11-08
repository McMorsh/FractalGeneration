#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "bookmarks.hpp"
#include "fractal_type.hpp"
#include "settings.hpp"

namespace {

Bookmark MakeSeahorseBookmark() {
  Bookmark bookmark;
  bookmark.used = true;
  bookmark.name = "Seahorse Valley";
  bookmark.view.center_x = -0.743643887037151;
  bookmark.view.center_y = 0.131825904205330;
  bookmark.view.span = 1e-12;
  bookmark.view.iterations = 104482;
  bookmark.view.palette = "Ocean";
  bookmark.fractal.type = FractalType::kJulia;
  bookmark.fractal.julia_real = -0.8;
  bookmark.fractal.julia_imag = 0.156;
  return bookmark;
}

TEST(BookmarksTest, FormatThenParseRoundTrip) {
  Bookmarks bookmarks;
  bookmarks.slots[0] = MakeSeahorseBookmark();

  const Bookmarks parsed = ParseBookmarks(FormatBookmarks(bookmarks));
  const Bookmark& bookmark = parsed.slots[0];

  ASSERT_TRUE(bookmark.used);
  EXPECT_EQ(bookmark.name, "Seahorse Valley");
  EXPECT_DOUBLE_EQ(bookmark.view.center_x, -0.743643887037151);
  EXPECT_DOUBLE_EQ(bookmark.view.center_y, 0.131825904205330);
  EXPECT_DOUBLE_EQ(bookmark.view.span, 1e-12);
  EXPECT_EQ(bookmark.view.iterations, 104482);
  EXPECT_EQ(bookmark.view.palette, "Ocean");
  EXPECT_EQ(bookmark.fractal.type, FractalType::kJulia);
  EXPECT_DOUBLE_EQ(bookmark.fractal.julia_real, -0.8);
  EXPECT_DOUBLE_EQ(bookmark.fractal.julia_imag, 0.156);
}

TEST(BookmarksTest, CyrillicNameRoundTrip) {
  Bookmarks bookmarks;
  bookmarks.slots[3] = MakeSeahorseBookmark();
  bookmarks.slots[3].name = "Долина морских коньков";

  const Bookmarks parsed = ParseBookmarks(FormatBookmarks(bookmarks));

  ASSERT_TRUE(parsed.slots[3].used);
  EXPECT_EQ(parsed.slots[3].name, "Долина морских коньков");
}

TEST(BookmarksTest, UnusedSlotsAreOmitted) {
  const Bookmarks parsed = ParseBookmarks(FormatBookmarks(Bookmarks{}));

  for (const Bookmark& bookmark : parsed.slots) {
    EXPECT_FALSE(bookmark.used);
  }
}

TEST(BookmarksTest, HeaderWithoutNameKeepsItEmpty) {
  Bookmarks bookmarks;
  bookmarks.slots[1] = MakeSeahorseBookmark();
  bookmarks.slots[1].name.clear();

  const Bookmarks parsed = ParseBookmarks(FormatBookmarks(bookmarks));

  ASSERT_TRUE(parsed.slots[1].used);
  EXPECT_TRUE(parsed.slots[1].name.empty());
}

TEST(BookmarksTest, InvalidSlotIsSkipped) {
  Bookmarks bookmarks;
  bookmarks.slots[0] = MakeSeahorseBookmark();
  std::string text = FormatBookmarks(bookmarks);
  const std::size_t header = text.find("[1]");
  ASSERT_NE(header, std::string::npos);
  text.replace(header, 3, "[10]");

  const Bookmarks parsed = ParseBookmarks(text);

  for (const Bookmark& bookmark : parsed.slots) {
    EXPECT_FALSE(bookmark.used);
  }
}

TEST(BookmarksTest, BlockWithoutFractalIsSkipped) {
  Bookmarks bookmarks;
  bookmarks.slots[0] = MakeSeahorseBookmark();
  std::string text = FormatBookmarks(bookmarks);
  const std::size_t fractal = text.find("fractal ");
  ASSERT_NE(fractal, std::string::npos);
  text.erase(fractal, text.find('\n', fractal) - fractal + 1);

  const Bookmarks parsed = ParseBookmarks(text);

  EXPECT_FALSE(parsed.slots[0].used);
}

TEST(BookmarksTest, MissingSpanIsSkipped) {
  const std::string text =
      "[2] Broken\n"
      "center (0, +0i)\n"
      "iterations 100\n"
      "palette Ember\n"
      "fractal mandelbrot\n";

  const Bookmarks parsed = ParseBookmarks(text);

  EXPECT_FALSE(parsed.slots[1].used);
}

TEST(BookmarksTest, DuplicateSlotKeepsLastBlock) {
  Bookmarks first;
  first.slots[0] = MakeSeahorseBookmark();
  first.slots[0].name = "First";
  Bookmarks second;
  second.slots[0] = MakeSeahorseBookmark();
  second.slots[0].name = "Second";

  const Bookmarks parsed =
      ParseBookmarks(FormatBookmarks(first) + FormatBookmarks(second));

  ASSERT_TRUE(parsed.slots[0].used);
  EXPECT_EQ(parsed.slots[0].name, "Second");
}

TEST(BookmarksTest, FractalTypeRoundTrip) {
  for (const FractalType type : {FractalType::kMandelbrot, FractalType::kJulia,
                                 FractalType::kBurningShip}) {
    Bookmarks bookmarks;
    bookmarks.slots[0] = MakeSeahorseBookmark();
    bookmarks.slots[0].fractal.type = type;

    const Bookmarks parsed = ParseBookmarks(FormatBookmarks(bookmarks));

    ASSERT_TRUE(parsed.slots[0].used);
    EXPECT_EQ(parsed.slots[0].fractal.type, type);
  }
}

TEST(BookmarksTest, PathPointsToBookmarksFile) {
  const std::filesystem::path path = GetBookmarksPath();

  EXPECT_EQ(path.filename().string(), "bookmarks.txt");
  EXPECT_EQ(path.parent_path().filename().string(), "FractalGeneration");
}

TEST(BookmarksTest, LoadMissingFileReturnsEmptySlots) {
  const std::filesystem::path path = std::filesystem::temp_directory_path() /
                                     "fractal_bookmarks_missing_test.txt";
  std::filesystem::remove(path);

  const Bookmarks parsed = LoadBookmarks(path);

  for (const Bookmark& bookmark : parsed.slots) {
    EXPECT_FALSE(bookmark.used);
  }
}

TEST(BookmarksTest, SaveAndLoadRoundTrip) {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "fractal_bookmarks_test.txt";
  std::filesystem::remove(path);

  Bookmarks bookmarks;
  bookmarks.slots[8] = MakeSeahorseBookmark();
  ASSERT_TRUE(SaveBookmarks(bookmarks, path));

  const Bookmarks parsed = LoadBookmarks(path);
  std::filesystem::remove(path);

  ASSERT_TRUE(parsed.slots[8].used);
  EXPECT_EQ(parsed.slots[8].name, "Seahorse Valley");
  EXPECT_EQ(parsed.slots[8].fractal.type, FractalType::kJulia);
}

}  // namespace
