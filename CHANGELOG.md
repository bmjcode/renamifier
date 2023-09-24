# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Changed
* Update the bundled dependencies on Windows.
### Fixed
* Clean up Qt support files when upgrading the Windows version.
* Clean up the existing GhostXPS installation when upgrading the Windows version.

## [0.1.3] - 2022-03-25
### Changed
* Open the rename and move dialog in the file's original location the first time it is used.
* Don't quit the program if the initial browse dialog is canceled. (A user might want to open files another way like dragging and dropping.)
### Fixed
* Don't process Unix-style hidden files whose names start with a `.` character.
* Always preserve the original file extension when rename-and-moving.
* Remove obsolete dependencies when upgrading the Windows version.

## [0.1.2] - 2022-10-15
### Changed
* Prompt to exit the application after rename-and-moving the last file.
* Update the bundled dependencies on Windows.
### Fixed
* Recognize `application/xps` as an alternate MIME type for XPS documents.

## [0.1.1] - 2021-12-05
### Changed
* Prompt to exit the application after renaming the last file.
* Follow Qt's coding style.
* Cleaned up some messy spots in the code.
### Fixed
* `Enter` on the numeric keypad also triggers the rename action.
* Improve paged content rendering on high-DPI screens.
* Create Start menu and desktop shortcuts system-wide rather than per-user.

## [0.1.0] - 2021-02-27
### Added
* Initial public release.

[Unreleased]: https://github.com/bmjcode/renamifier/compare/v0.1.3...HEAD
[0.1.3]: https://github.com/bmjcode/renamifier/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/bmjcode/renamifier/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/bmjcode/renamifier/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/bmjcode/renamifier/releases/tag/v0.1.0
