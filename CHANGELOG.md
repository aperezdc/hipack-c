# Change Log
All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased]

## [v0.1.2] - 2015-12-27
### Added
- [API reference documentation](http://hipack-c.readthedocs.org/en/latest/apiref.html) and [Quickstart guide](http://hipack-c.readthedocs.org/en/latest/quickstart.html).
- Support for [HEP-1 Value Annotations](https://github.com/aperezdc/hipack/blob/gh-pages/heps/hep-001.rst).
- Utility functions to create `hipack_value_t` values.
- Improved test suite with additional test cases.
- Improve Travis-CI build times by using containerized builds and preserving the contents of `~/.cache/pip` between builds.

### Fixed
- Do not allow exponent letter (`e` or `E`) in octal numbers.
- Fixed checkout of `fpconv` submodule to avoid main targets being always rebuilt.

## v0.1.0
- First release.

[Unreleased]: https://github.com/aperezdc/hipack-c/compare/v0.1.2...HEAD
[v0.1.2]: https://github.com/aperezdc/hipack-c/compare/v0.1.0...v0.1.2
