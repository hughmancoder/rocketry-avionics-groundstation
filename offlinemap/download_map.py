import math
import os
import shutil
import time
import requests
from pathlib import Path

# --- Configuration ---
# These values are filled by the CLI before downloading.
LAT = 0.0
LON = 0.0
RADIUS_KM = 0.0
MIN_LAT = 0.0
MAX_LAT = 0.0
MIN_LON = 0.0
MAX_LON = 0.0

METERS_PER_DEGREE = 111111.0

# Zoom levels:
# 10 = regional view, 15 = high detail, 16 = very high detail
MIN_ZOOM = 10
MAX_ZOOM = 15

# Terrain (elevation) tiles are much coarser than imagery. AWS Terrarium tiles
# top out at zoom 15 globally, but for most terrain-mesh purposes zoom 12-13
# is already more resolution than you need (and keeps download size sane).
TERRAIN_MIN_ZOOM = 10
TERRAIN_MAX_ZOOM = 13

REPOSITORY_DIR = Path(__file__).resolve().parent.parent
IMAGERY_OUTPUT_DIR = str(REPOSITORY_DIR / "gui" / "public" / "maps" / "tiles")
TERRAIN_OUTPUT_DIR = str(REPOSITORY_DIR / "gui" / "public" / "maps" / "terrain")

# Tile provider URL templates.
# Imagery: Esri's endpoint is {z}/{y}/{x}, saved locally as {z}/{x}/{y}.jpg for MapLibre.
IMAGERY_TILE_URL = "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}"

# Terrain: AWS Terrarium tiles (free, no API key). Standard {z}/{x}/{y}.png layout.
# Elevation is encoded in the RGB channels: height = (R*256 + G + B/256) - 32768
TERRAIN_TILE_URL = "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/{z}/{x}/{y}.png"

# Standard User-Agent to avoid generic 403 request drops
HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
}


def read_float(prompt, minimum, maximum):
    """Read and validate a numeric CLI value."""
    while True:
        try:
            value = float(input(prompt).strip())
        except ValueError:
            print("Please enter a number.")
            continue

        if minimum <= value <= maximum:
            return value

        print(f"Please enter a value between {minimum} and {maximum}.")


def configure_area():
    """Ask for the map center and radius, then calculate the download bounds."""
    global LAT, LON, RADIUS_KM, MIN_LAT, MAX_LAT, MIN_LON, MAX_LON

    print("Offline map downloader")
    print("Zoom levels are fixed in this script.")
    LAT = read_float("Center latitude (-90 to 90): ", -90.0, 90.0)
    LON = read_float("Center longitude (-180 to 180): ", -180.0, 180.0)
    RADIUS_KM = read_float("Radius in kilometres (> 0): ", 0.001, 1000.0)

    radius_meters = RADIUS_KM * 1000.0
    latitude_delta = radius_meters / METERS_PER_DEGREE
    longitude_scale = abs(math.cos(math.radians(LAT)))
    longitude_delta = radius_meters / (METERS_PER_DEGREE * longitude_scale)

    MIN_LAT = max(-90.0, LAT - latitude_delta)
    MAX_LAT = min(90.0, LAT + latitude_delta)
    MIN_LON = max(-180.0, LON - longitude_delta)
    MAX_LON = min(180.0, LON + longitude_delta)

    print(f"\nCenter: {LAT:.6f}, {LON:.6f}")
    print(f"Radius: {RADIUS_KM:.2f} km")
    print(
        f"Bounds: lat {MIN_LAT:.6f} to {MAX_LAT:.6f}, "
        f"lon {MIN_LON:.6f} to {MAX_LON:.6f}\n"
    )


def deg2num(lat_deg, lon_deg, zoom):
    """Converts WGS84 lat/lon to Slippy map tile numbers (x, y)."""
    lat_rad = math.radians(lat_deg)
    n = 2.0**zoom
    xtile = int((lon_deg + 180.0) / 360.0 * n)
    ytile = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return xtile, ytile


def _tile_bounds(min_lat, min_lon, max_lat, max_lon, zoom):
    """Returns (x_min, x_max, y_min, y_max) tile indexes for a bbox at a zoom."""
    x_min, y1 = deg2num(min_lat, min_lon, zoom)
    x_max, y2 = deg2num(max_lat, max_lon, zoom)
    # Invert y bounds because tile (0, 0) is North-West
    y_min = min(y1, y2)
    y_max = max(y1, y2)
    return x_min, x_max, y_min, y_max


def _print_progress(label, zoom, completed, total):
    """Render a compact in-place progress bar for one zoom level."""
    width = 30
    progress = completed / total if total else 1.0
    filled = int(width * progress)
    bar = "=" * filled + "-" * (width - filled)
    print(
        f"\r[{label}] Zoom {zoom}: [{bar}] {completed}/{total} "
        f"({progress * 100:5.1f}%)",
        end="",
        flush=True,
    )


def _download_tile_grid(session, url_template, output_dir, min_zoom, max_zoom,
                         file_ext, label):
    """Generic slippy-map tile grid downloader, saved as {z}/{x}/{y}.{ext}."""
    total_downloaded = 0

    for z in range(min_zoom, max_zoom + 1):
        x_min, x_max, y_min, y_max = _tile_bounds(MIN_LAT, MIN_LON, MAX_LAT, MAX_LON, z)

        x_count = (x_max - x_min) + 1
        y_count = (y_max - y_min) + 1
        print(
            f"[{label}] Zoom {z}: downloading {x_count}x{y_count} = {x_count * y_count} tiles..."
        )
        total_tiles = x_count * y_count
        completed_tiles = 0

        for x in range(x_min, x_max + 1):
            dir_path = os.path.join(output_dir, str(z), str(x))
            os.makedirs(dir_path, exist_ok=True)

            for y in range(y_min, y_max + 1):
                file_path = os.path.join(dir_path, f"{y}.{file_ext}")

                # Skip if already downloaded
                if os.path.exists(file_path):
                    completed_tiles += 1
                    _print_progress(label, z, completed_tiles, total_tiles)
                    continue

                url = url_template.format(z=z, y=y, x=x)

                try:
                    resp = session.get(url, headers=HEADERS, timeout=10)
                    if resp.status_code == 200:
                        with open(file_path, "wb") as f:
                            f.write(resp.content)
                        total_downloaded += 1
                    else:
                        print(f"[{label}] Failed {url} (HTTP {resp.status_code})")
                except Exception as e:
                    print(f"[{label}] Error on tile {z}/{x}/{y}: {e}")

                # Gentle delay to avoid server rate-limiting
                time.sleep(0.05)

                completed_tiles += 1
                _print_progress(label, z, completed_tiles, total_tiles)

            print()

    print(f"[{label}] Done! Downloaded {total_downloaded} new tiles to {output_dir}")
    return total_downloaded


def download_tiles():
    """Downloads satellite/aerial imagery tiles (color texture only, no elevation)."""
    session = requests.Session()
    return _download_tile_grid(
        session,
        IMAGERY_TILE_URL,
        IMAGERY_OUTPUT_DIR,
        MIN_ZOOM,
        MAX_ZOOM,
        file_ext="jpg",
        label="imagery",
    )


def download_terrain_tiles():
    """Downloads AWS Terrarium terrain-RGB elevation tiles.

    Each pixel encodes an elevation value in meters:
        height = (R * 256 + G + B / 256) - 32768

    These are what you feed into MapLibre GL JS as a `raster-dem` source
    (tileSize 256, encoding "terrarium") to get actual 3D terrain shape,
    separate from the color imagery draped on top of it.
    """
    session = requests.Session()
    return _download_tile_grid(
        session,
        TERRAIN_TILE_URL,
        TERRAIN_OUTPUT_DIR,
        TERRAIN_MIN_ZOOM,
        TERRAIN_MAX_ZOOM,
        file_ext="png",
        label="terrain",
    )

def check_directories():
    """Ensures that the output directories for imagery and terrain tiles exist. Delete them if they already exist."""
    if os.path.exists(IMAGERY_OUTPUT_DIR) or os.path.exists(TERRAIN_OUTPUT_DIR):
        print("Are you sure you want to delete existing directories?   This will remove all existing tiles.")
        input("Press Enter to continue or Ctrl+C to abort...")
    if os.path.exists(IMAGERY_OUTPUT_DIR):
        shutil.rmtree(IMAGERY_OUTPUT_DIR)
    if os.path.exists(TERRAIN_OUTPUT_DIR):
        shutil.rmtree(TERRAIN_OUTPUT_DIR)
    os.makedirs(IMAGERY_OUTPUT_DIR, exist_ok=True)
    os.makedirs(TERRAIN_OUTPUT_DIR, exist_ok=True)
    print(f"Checked directories. Imagery: {IMAGERY_OUTPUT_DIR}, Terrain: {TERRAIN_OUTPUT_DIR}")



if __name__ == "__main__":
    configure_area()
    check_directories()
    download_tiles()
    download_terrain_tiles()