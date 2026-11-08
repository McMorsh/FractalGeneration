#ifndef FRACTALGENERATION_RENDERER_BOOKMARKS_HPP_
#define FRACTALGENERATION_RENDERER_BOOKMARKS_HPP_

#include <array>
#include <filesystem>
#include <string>

#include "fractal_type.hpp"
#include "view_snapshot.hpp"

/// Number of bookmark slots, matching the 1...9 keys.
inline constexpr int kBookmarkSlotCount = 9;

/// Saved view: camera, iterations, palette, fractal type, and Julia c.
struct Bookmark {
  bool used = false;
  std::string name;
  ViewSnapshot view;
  FractalParams fractal;
};

/// Bookmark slots; slot n is stored at index n - 1.
struct Bookmarks {
  std::array<Bookmark, kBookmarkSlotCount> slots;
};

/// Parses bookmark blocks. Invalid slots and incomplete blocks are skipped.
Bookmarks ParseBookmarks(const std::string& text);

/// Writes used slots as "[n] name" blocks. Empty slots are omitted.
std::string FormatBookmarks(const Bookmarks& bookmarks);

/// Reads bookmarks. A missing file returns empty slots.
Bookmarks LoadBookmarks(const std::filesystem::path& path);

/// Writes bookmarks, creating the directory if needed.
/// @return false if the file could not be created or written.
bool SaveBookmarks(const Bookmarks& bookmarks,
                   const std::filesystem::path& path);

#endif  // FRACTALGENERATION_RENDERER_BOOKMARKS_HPP_
