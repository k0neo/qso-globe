# QSO Globe

<img src="desktop/qso-globe.svg" width="96" align="right" alt="QSO Globe">

A hardware-accelerated, real-time terrestrial globe for
visualizing amateur radio contacts.

**Version 1.0.0**

QSO Globe receives contact notifications from amateur radio
logging software and automatically animates the globe from
the home station to the contacted station, displaying the
great-circle path, distance, and initial bearing.

## Features

- Native C11 implementation.
- Hardware-accelerated OpenGL rendering.
- Smooth spherical camera interpolation.
- Adaptive destination magnification.
- Great-circle path visualization.
- Real-time solar ephemeris.
- Daylight, twilight, and nighttime illumination.
- Gray-line highlighting.
- High-resolution NASA Blue Marble and Black Marble imagery.
- Historical station markers with configurable retention.
- Event-driven rendering.
- UDP contact notification interface.
- Duplicate contact suppression.
- Configurable station coordinates and callsign.
- Persistent window geometry and illumination preferences.
- XDG configuration directory support.

## Performance

Development and acceptance testing were conducted on Linux
Mint 21.3 using an NVIDIA GeForce GTX 970.

Measured performance:

| Metric | Result |
|---|---|
| Animation rendering | Approximately 60 FPS |
| Stationary CPU utilization | 0.0% at measurement resolution |
| Daytime texture | 8192 x 4096 |
| Nighttime texture | 8192 x 4096 |
| Executable size | Approximately 65 KiB |

Performance will vary with graphics hardware, drivers,
display configuration, and system load.

## Dependencies

Build requirements:

- C11 compiler
- GNU Make
- pkg-config
- X11 development libraries
- OpenGL development libraries
- GLU development libraries
- libjpeg development libraries

Texture preparation additionally requires curl, FFmpeg,
ffprobe, and a SHA-256 checksum utility.

### Debian, Ubuntu, and Linux Mint

    sudo apt install build-essential pkg-config \
        libx11-dev libgl-dev libglu1-mesa-dev \
        libjpeg-dev curl ffmpeg

### FreeBSD

The source is designed around portable C and POSIX interfaces.
FreeBSD compatibility has not yet been independently verified.

## Building

    make

## Regression testing

    make test

The automated regression suites cover:

- Configuration parsing and validation.
- Persistent operator state.
- Solar ephemeris calculations.
- Maidenhead locator conversion.
- UDP protocol validation.
- Contact queue and duplicate suppression.
- Filesystem directory creation.

Network regression tests use an ephemeral loopback port
and do not interfere with a running production instance.

## Installation

System-wide installation:

    sudo make install

User-local installation:

    make install PREFIX="$HOME/.local"

The user-local bin directory must be included in PATH.

Prepare the NASA imagery as the operating user:

    qso-globe-prepare-textures

The imagery preparation utility verifies the original NASA
source files using SHA-256 checksums before resampling.

Existing textures with the expected dimensions are preserved.
Use --force to regenerate them.

## Station configuration

Create the configuration directory:

    mkdir -p "$HOME/.config/qso-globe"

Copy the example configuration if one does not already exist:

    cp -n examples/qso-globe.conf.example \
        "$HOME/.config/qso-globe/qso-globe.conf"

Edit the station callsign, latitude, and longitude.

Latitude is positive north of the equator.
Longitude is positive east of Greenwich.

The default UDP listening port is 45832.

## Running

    qso-globe

The application can also be launched from a compatible
desktop application menu.

### Command-line options

    qso-globe --help
    qso-globe --version
    qso-globe --show-config
    qso-globe --test-expiry

### Keyboard controls

| Key | Function |
|---|---|
| D | Full daylight toggle |
| N | Natural illumination toggle |
| G | Gray-line highlighting toggle |
| T | Terminator contour toggle |
| Space | London demonstration |
| Escape | Exit |

## Logging software integration

QSO Globe accepts UDP notifications on the IPv4 loopback
interface.

The protocol is intentionally independent of any particular
logging application.

Production integration has been demonstrated with CQRLOG
using an external database listener, including contacts
originating from CW and JS8Call operating sessions.

See docs/protocol.md for the notification format.

## Configuration and state

Permanent station configuration:

    ~/.config/qso-globe/qso-globe.conf

Automatically maintained operator preferences:

    ~/.config/qso-globe/qso-globe.state

The state file preserves window geometry and illumination
preferences without modifying permanent station settings.

XDG_CONFIG_HOME is supported.

## Imagery

QSO Globe uses NASA Blue Marble and Black Marble imagery.

The original imagery and generated textures are not bundled
with the software distribution.

The texture preparation utility downloads the original
datasets, verifies their checksums, and generates the
production textures locally.

Existing Xplanet textures may be used as fallback imagery.

## Limitations

- Requires an X11 display and OpenGL compatibility context.
- Historical contact markers are retained in memory only.
- Duplicate suppression does not persist across restarts.
- Incoming contact notifications require a valid Maidenhead
  locator.
- Solar illumination is not an ionospheric propagation model.
- FreeBSD runtime compatibility remains unverified.

## Uninstallation

    sudo make uninstall

For a user-local installation:

    make uninstall PREFIX="$HOME/.local"

Uninstallation preserves personal configuration,
operator preferences, and downloaded imagery.

## Documentation

    man qso-globe

## Acknowledgments

NASA Earth Observatory for Blue Marble and Black Marble
terrestrial imagery.

The open-source X11, OpenGL, GLU, FFmpeg, and libjpeg
development communities.

## License

Copyright (c) 2026, R. Lynn Brown.

QSO Globe is distributed under the BSD 2-Clause License.
See [LICENSE](LICENSE) for the complete terms.

External libraries and NASA imagery retain their respective
licensing and usage terms.
