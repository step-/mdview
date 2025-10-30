# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog].
This project uses a year.month.date version format,
therefore it does not adhere to [Semantic Versioning].

## [Unreleased] - year-month-day [wiki](https://github.com/step-/mdview/wiki/Z-changelog-unreleased)

## [2025.10.30] - 2025-10-30 [wiki](https://github.com/step-/mdview/wiki/Z-changelog-2025.10.30)

### Changed

- When clicking an internal link, the part after the last '#' determines the destination location.

### Fixed

- Improved left-margin alignment of ordered list items.

## [2025.01.27] - 2025-01-27 [wiki](https://github.com/step-/mdview/wiki/Z-changelog-2025.01.27)

### Added

- File Open dialog button to the tool bar, and the corresponding `Ctrl+O` hotkey.
- The [Linter] reports headings of non-clickable Table of Contents links.
- The `--bare` output mode command-line option.
- The `--license` command-line option.
* The `make` and `meson` builds generate `mdview.1` if `help2man`(1) is installed.

### Changed

- Split version and license information onto separate options.
- Preserve [Backing file] contents if the Table of Contents is empty.
- Prevent reloading a non-existent home page and write a message to the status bar.
* Update POT translation template and the AI-generated French translation.

### Fixed

- In `--text` mode an image name was incorrectly wrapped with angular rather than curly brakets.
- The shell comment `# "A"` within a Markdown code block was mistaken for a heading.
- In some smart text replacements, double quotes could become single quotes.
- The parser could output a spurious U+F611 character.
- Interrupting the program with `Ctrl+C` did not remove the backing file from `/tmp`.
- Squashed some memory leaks and invalid pointer access errors.
- Fixed an issue with the `meson` build picking resources outside the build directory.

## [2025.01.04] - 2025-01-04 [wiki](https://github.com/step-/mdview/wiki/Z-changelog-2025.01.04)

### Added

- Automatically save the [Table of Contents] Markdown to the [Backing file].
- The `--lint` command-line option enables a simple internal [Linter].
- Added a test suite.

### Changed

- Updated AI-generated French translation.
- Updated copyright notices.

### Fixed

- Null pointer accessing an invalid resource URI from the command line (introduced in 2024.12.14).
- Markdown table starting on line 1 is processed as a paragraph.
- Potential Pango markup error when pressing `[F1]` if the `--unsafe-html` option is enabled.

## [2024.12.14] - 2024-12-14 [wiki](https://github.com/step-/mdview/wiki/Z-changelog-2025.12.14)

### Added

- HTML preview of the search results page.

### Changed

- The viewer renders an empty link text as "⯅⯅" (vs. "{-}" previously).

### Fixed

- The viewer showed an indented ATX heading as body text when auto-heading links are enabled.
- Link in search results page wasn't fully underlined.
- Empty link text in search results page.
- `-Wmaybe-uninitialized`" compiler warning.

## [2024.12.08] - 2024-12-08 [wiki](https://github.com/step-/mdview/wiki/Z-changelog-2025.12.08)

### Added

- Internal Help and Welcome pages.
- New command-line options:
  - `--dump-styles`
  - `--html-base`
  - `--html-css=` 1(minimal style sheet) 2([github-markdown-css])
  - `--html-full`
  - `--no-strikethrough`
  - `--no-heading-link`
  - `--output=`
  - `--toc-level=` 1 -- 6
- `MDVIEW_OPTIONS` environment variable (refer to the wiki for details).
- `HOMEPAGE` types (refer to the wiki for details):
  - Local file path
  - `search://` URI
  - `resource://` URI (including image resources)
- Viewer: add custom application icon.
- Viewer and HTML output: Add Table of Contents and `--toc-level` option (refer to the wiki for details).
- Viewer and HTML output: [Auto-heading links] add anchors (jump targets).
- Display an error page (with back-link) when attempting to load a non-existent local file, a missing resource, or a failed file search.
- Viewer: Load pages asynchronously (with progres but and the `[C-x]` hotkey to cancel loading).
- Viewer: Preview the page as an HTML file in the web browser (`[A-p]`) with CSS stylesheet options (refer to the wiki for details and [github-markdown-css]).
- Adding page metadata as an embedded XML header.
- Add meson-based build for the release package.
- Add Gettext translation template (POT) for translators, and an experimental, AI-based translation file (PO) for French.

### Changed

- Viewer: No longer render markdown link titles as page content.
- Rename option `--soft-breaks` to `--soft-break`.
- Extend shebang detection to `<!DOCTYPE` and `<html`.
- Viewer: Display the usage page if the initial file is not found.
- Recognize dot-relative pathnames as auto-code words.
- Extend the scope of the `--unsafe-html` option to all output modes, viewer included.
* The `TEXTDOMAIN` and `TEXTDOMAINDIR` environment variables are no longer supported at runtime.

## [2024.08.26] - 2024-08-26

First working version.

[Unreleased]: <https://github.com/step-/mdview/compare/2025.10.30...HEAD>
[2025.10.30]: <https://github.com/step-/mdview/compare/2025.01.27...2025.10.30>
[2025.01.27]: <https://github.com/step-/mdview/compare/2025.01.04...2025.01.27>
[2025.01.04]: <https://github.com/step-/mdview/compare/2024.12.14...2025.01.04>
[2024.12.14]: <https://github.com/step-/mdview/compare/2024.12.08...2024.12.14>
[2024.12.08]: <https://github.com/step-/mdview/releases/tag/2024.12.08>

[Keep a Changelog]: <https://keepachangelog.com/en/1.1.0/>
[Semantic Versioning]: <https://semver.org/spec/v2.0.0.html>

[Auto-heading links]: <https://github.com/step-/mdview/wiki/Extensions#auto-heading-links>
[Backing file]: <http://github.com/step-/mdview/wiki/Viewer#the-backing-file>
[github-markdown-css]: <https://github.com/sindresorhus/github-markdown-css>
[Linter]: <http://github.com/step-/mdview/wiki/Linter>
[Table of Contents]: <http://github.com/step-/mdview/wiki/Extensions#table-of-contents>
[wiki.vim]: <https://github.com/lervag/wiki.vim>
