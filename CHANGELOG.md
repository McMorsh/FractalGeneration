# Changelog

**English** | [Русский](CHANGELOG.ru.md)

All notable changes to this project.

## 1.3.0 - 2026-11-08

### Added

- Zoom animation export to GIF: `F3`, 1280x720, 25 fps, 5 seconds,
  a shared 256-color palette for all frames.

## 1.2.0 - 2026-11-08

### Added

- View bookmarks: `1`...`9` switch, `Shift + 1`...`9` save, the name is entered
  in a dialog, and `bookmarks.txt` is stored next to the settings.

## 1.1.0 - 2026-11-08

### Added

- Zoom down to span `1e-12`, about 10 times deeper than the previous limit.
- Automatic iteration count up to 200,000 at deep zoom.
- The `[` and `]` step grows with the iteration count.

### Changed

- Iteration range expanded from 50...5,000 to 50...200,000.
- Headless mode clamps span the same way as the window.

## 1.0.0 - 2026-11-08

### Added

- Mandelbrot, Julia, and Burning Ship sets.
- Interactive viewer with zoom, panning, and a resizable window.
- Smooth coloring and Ember, Ocean, and Mono palettes.
- Automatic iteration count selection and manual adjustment.
- OpenCL, AVX2, and scalar CPU backends, selectable with `--backend`.
- Background rendering, preview, and PNG export in FHD, 2K, 4K, and 8K.
- Headless mode with `--fractal`, `--julia-re`, `--julia-im`, and `--palette`.
- Copying and pasting the view via the clipboard.
- Russian and English UI with saved settings.
- `--version` and the version in the lower right corner of the window.
- GoogleTest and CTest tests plus a GitHub Actions build.
- Fractal gallery and zoom GIF in the README.

### Fixed

- PNG export does not include the UI.
- Stale frames do not overwrite newer ones.
- Manual iteration adjustment works correctly at the 50 and 5,000 boundaries.
- Fullscreen mode does not conflict with raylib's built-in screenshot.
