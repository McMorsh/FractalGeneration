#include "bookmarks.hpp"

#include <cerrno>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "fractal_type.hpp"
#include "view.hpp"
#include "view_snapshot.hpp"

namespace {

constexpr const char* kFractalField = "fractal";
constexpr const char* kJuliaRealField = "julia-re";
constexpr const char* kJuliaImagField = "julia-im";

std::string Trim(const std::string& text) {
  const std::size_t first = text.find_first_not_of(" \t\r");
  if (first == std::string::npos) {
    return {};
  }

  const std::size_t last = text.find_last_not_of(" \t\r");
  return text.substr(first, last - first + 1);
}

bool StartsWithField(const std::string& line, const char* field) {
  const std::size_t length = std::char_traits<char>::length(field);
  if (line.size() <= length || line.compare(0, length, field) != 0) {
    return false;
  }
  return line[length] == ' ' || line[length] == '\t';
}

std::string FieldValue(const std::string& line, const char* field) {
  return Trim(line.substr(std::char_traits<char>::length(field)));
}

bool ParseDoubleValue(const std::string& text, double& value) {
  if (text.empty()) {
    return false;
  }

  errno = 0;
  char* end = nullptr;
  const double parsed = std::strtod(text.c_str(), &end);
  if (end == text.c_str() || errno == ERANGE || !std::isfinite(parsed)) {
    return false;
  }

  while (*end == ' ' || *end == '\t' || *end == '\r') {
    ++end;
  }
  if (*end != '\0') {
    return false;
  }

  value = parsed;
  return true;
}

bool ParseIntValue(const std::string& text, int& value) {
  if (text.empty()) {
    return false;
  }

  errno = 0;
  char* end = nullptr;
  const long parsed = std::strtol(text.c_str(), &end, 10);
  if (end == text.c_str() || errno == ERANGE || parsed < INT_MIN ||
      parsed > INT_MAX) {
    return false;
  }

  while (*end == ' ' || *end == '\t' || *end == '\r') {
    ++end;
  }
  if (*end != '\0') {
    return false;
  }

  value = static_cast<int>(parsed);
  return true;
}

struct SlotHeader {
  /// 1-based slot number; 0 means the header is invalid.
  int slot = 0;
  std::string name;
};

SlotHeader ParseSlotHeader(const std::string& line) {
  SlotHeader header;
  const std::size_t close = line.find(']');
  if (close == std::string::npos) {
    return header;
  }

  int slot = 0;
  if (!ParseIntValue(Trim(line.substr(1, close - 1)), slot) || slot < 1 ||
      slot > kBookmarkSlotCount) {
    return header;
  }

  header.slot = slot;
  header.name = Trim(line.substr(close + 1));
  return header;
}

std::vector<std::string> SplitLines(const std::string& text) {
  std::vector<std::string> lines;
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t end = text.find('\n', start);
    std::string line = text.substr(
        start, end == std::string::npos ? std::string::npos : end - start);
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    lines.push_back(std::move(line));
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return lines;
}

bool ParseBlockParams(const std::string& body, FractalParams& params) {
  bool has_fractal = false;
  for (const std::string& line : SplitLines(body)) {
    if (StartsWithField(line, kFractalField)) {
      if (!ParseFractalType(FieldValue(line, kFractalField), params.type)) {
        return false;
      }
      has_fractal = true;
    } else if (StartsWithField(line, kJuliaRealField)) {
      if (!ParseDoubleValue(FieldValue(line, kJuliaRealField),
                            params.julia_real)) {
        return false;
      }
    } else if (StartsWithField(line, kJuliaImagField)) {
      if (!ParseDoubleValue(FieldValue(line, kJuliaImagField),
                            params.julia_imag)) {
        return false;
      }
    }
  }
  return has_fractal;
}

std::string FormatDouble(double value) {
  constexpr int kDigits = std::numeric_limits<double>::max_digits10;
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.*g", kDigits, value);
  return buffer;
}

}  // namespace

Bookmarks ParseBookmarks(const std::string& text) {
  Bookmarks bookmarks;
  int slot = 0;
  std::string name;
  std::string body;
  bool has_block = false;

  const auto flush = [&]() {
    if (!has_block || slot == 0) {
      return;
    }

    ViewSnapshot view;
    if (!ParseViewSnapshot(body, view)) {
      return;
    }

    FractalParams params;
    if (!ParseBlockParams(body, params)) {
      return;
    }

    Bookmark& bookmark = bookmarks.slots[slot - 1];
    bookmark.used = true;
    bookmark.name = name;
    bookmark.view = view;
    bookmark.fractal = params;
  };

  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t end = text.find('\n', start);
    std::string line = text.substr(
        start, end == std::string::npos ? std::string::npos : end - start);
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    if (!line.empty() && line.front() == '[') {
      flush();
      const SlotHeader header = ParseSlotHeader(line);
      slot = header.slot;
      name = header.name;
      body.clear();
      has_block = true;
    } else if (has_block) {
      body += line;
      body += '\n';
    }

    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  flush();

  return bookmarks;
}

std::string FormatBookmarks(const Bookmarks& bookmarks) {
  std::string text = "# FractalGeneration bookmarks\n";
  for (int i = 0; i < kBookmarkSlotCount; ++i) {
    const Bookmark& bookmark = bookmarks.slots[i];
    if (!bookmark.used) {
      continue;
    }

    text += '\n';
    text += '[';
    text += std::to_string(i + 1);
    text += ']';
    if (!bookmark.name.empty()) {
      text += ' ';
      text += bookmark.name;
    }
    text += '\n';

    const View view(bookmark.view.center_x, bookmark.view.center_y,
                    bookmark.view.span, 1, 1);
    text += FormatViewSnapshot(view, bookmark.view.iterations,
                               bookmark.view.palette);
    text += '\n';
    text += "fractal ";
    text += GetFractalTypeName(bookmark.fractal.type);
    text += '\n';
    text += "julia-re ";
    text += FormatDouble(bookmark.fractal.julia_real);
    text += '\n';
    text += "julia-im ";
    text += FormatDouble(bookmark.fractal.julia_imag);
    text += '\n';
  }
  return text;
}

Bookmarks LoadBookmarks(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return {};
  }

  std::string text;
  std::string line;
  while (std::getline(file, line)) {
    text += line;
    text += '\n';
  }
  return ParseBookmarks(text);
}

bool SaveBookmarks(const Bookmarks& bookmarks,
                   const std::filesystem::path& path) {
  const std::filesystem::path directory = path.parent_path();
  if (!directory.empty()) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
      return false;
    }
  }

  std::ofstream file(path, std::ios::trunc);
  if (!file.is_open()) {
    return false;
  }

  file << FormatBookmarks(bookmarks);
  file.flush();
  return file.good();
}
