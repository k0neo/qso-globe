# Changelog

## Version 1.0.0

Initial production release.

### Rendering

- Native C11 OpenGL terrestrial globe.
- Hardware-accelerated sphere rendering.
- Smooth spherical camera interpolation.
- Adaptive contact magnification.
- Great-circle paths and station markers.
- High-resolution NASA daytime and nighttime textures.
- Solar illumination and twilight modeling.
- Gray-line and terminator visualization.
- Event-driven stationary rendering.

### Contact processing

- Local UDP notification protocol.
- Four-, six-, and eight-character Maidenhead locators.
- Geographic distance and initial bearing calculations.
- FIFO contact scheduling.
- Bounded duplicate suppression.
- Automatic historical marker expiration.

### Configuration

- INI-style station configuration.
- Configurable station identity and coordinates.
- Configurable animation timing.
- Configurable listening port and texture paths.
- XDG configuration support.
- Atomic operator preference persistence.
- Diagnostic and expiration-test modes.

### Packaging

- GNU Make build system.
- Automated regression suites.
- UNIX manual page.
- Desktop launcher and scalable SVG icon.
- NASA imagery preparation utility.
- Installation and uninstallation targets.
- PREFIX and DESTDIR support.

### Verified deployment

- Linux Mint 21.3.
- NVIDIA GeForce GTX 970.
- Approximately 60 FPS during animation.
- No measurable stationary CPU utilization in acceptance samples.
- External CQRLOG database listener integration.
