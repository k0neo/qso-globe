#!/bin/sh

# QSO Globe 1.0.0
# NASA Blue Marble and Black Marble texture preparation.

set -eu

force=0

case "${1:-}" in
    --force)
        force=1
        ;;
    --help)
        printf 'Usage: %s [--force]\n' "$0"
        exit 0
        ;;
    "")
        ;;
    *)
        printf 'Unknown option: %s\n' "$1" >&2
        exit 1
        ;;
esac

for tool in curl ffmpeg ffprobe awk mktemp
do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'Required utility missing: %s\n' "$tool" >&2
        exit 1
    fi
done

if command -v sha256sum >/dev/null 2>&1; then
    checksum_tool=sha256sum
elif command -v sha256 >/dev/null 2>&1; then
    checksum_tool=sha256
else
    printf 'No SHA-256 utility available\n' >&2
    exit 1
fi

checksum()
{
    case "$checksum_tool" in
        sha256sum)
            sha256sum "$1" | awk '{print $1}'
            ;;
        sha256)
            sha256 -q "$1"
            ;;
    esac
}

assets="${HOME:?HOME is not set}/.local/share/qso-globe/textures"

mkdir -p "$assets/original"

work=$(mktemp -d "$assets/.prepare.XXXXXX")

trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

download()
{
    name="$1"
    url="$2"
    expected="$3"

    original="$assets/original/$name"

    if [ -f "$original" ]; then
        if [ "$(checksum "$original")" = "$expected" ]; then
            printf 'Verified existing source: %s\n' "$name"
            return
        fi
    fi

    printf 'Downloading %s\n' "$name"

    curl -fL --retry 3 \
        "$url" \
        -o "$work/$name"

    actual=$(checksum "$work/$name")

    if [ "$actual" != "$expected" ]; then
        printf 'Source checksum mismatch: %s\n' "$name" >&2
        printf 'Expected: %s\n' "$expected" >&2
        printf 'Received: %s\n' "$actual" >&2
        exit 1
    fi

    mv "$work/$name" "$original"

    printf 'Source checksum verified: %s\n' "$name"
}

dimensions()
{
    ffprobe \
        -v error \
        -select_streams v:0 \
        -show_entries stream=width,height \
        -of csv=p=0:s=x \
        "$1"
}

prepare()
{
    source="$1"
    destination="$2"

    output="$assets/$destination"

    if [ "$force" -eq 0 ] && [ -f "$output" ]; then

        actual=$(dimensions "$output" 2>/dev/null || true)

        if [ "$actual" = "8192x4096" ]; then
            printf 'Preserving existing texture: %s\n' \
                "$destination"
            return
        fi
    fi

    printf 'Preparing %s\n' "$destination"

    ffmpeg \
        -hide_banner -loglevel error -y \
        -i "$assets/original/$source" \
        -vf 'scale=8192:4096:flags=lanczos' \
        -frames:v 1 \
        -q:v 2 \
        "$work/$destination"

    actual=$(dimensions "$work/$destination")

    if [ "$actual" != "8192x4096" ]; then
        printf 'Unexpected texture dimensions: %s\n' \
            "$actual" >&2
        exit 1
    fi

    mv "$work/$destination" "$output"

    printf 'Texture verified: %s (%s)\n' \
        "$destination" "$actual"
}

printf '\n=== NASA source imagery ===\n'

download \
    blue-marble-november.jpg \
    'https://assets.science.nasa.gov/content/dam/science/esd/eo/images/bmng/bmng-base/november/world.200411.3x21600x10800.jpg' \
    '1959bbe72897d3e9526c440dae3ec04cfdbdc4080c8ff0b14632925ae31167b1'

download \
    black-marble-2016.jpg \
    'https://assets.science.nasa.gov/content/dam/science/esd/eo/images/imagerecords/144000/144898/BlackMarble_2016_3km.jpg' \
    '230aac448ae68c358be433dd518888cccb3a85ccf66f7b44326441c324ad6725'

printf '\n=== Prepare production textures ===\n'

prepare blue-marble-november.jpg earth-8192.jpg
prepare black-marble-2016.jpg night-8192.jpg

printf '\n=== Installed texture inventory ===\n'

ls -lh \
    "$assets/earth-8192.jpg" \
    "$assets/night-8192.jpg"

printf '\n=== Installed texture checksums ===\n'

for image in earth-8192.jpg night-8192.jpg
do
    printf '%s  %s\n' \
        "$(checksum "$assets/$image")" "$image"
done

printf '\nNASA imagery preparation complete.\n'
printf 'Texture directory: %s\n' "$assets"
