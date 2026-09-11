import math
import os
import time
import requests

# --- Configuration ---
# Target coordinates (Launch site: 30°39'53.3"S, 143°11'46.7"E)
LAT = -30.664806
LON = 143.196306

# Rough bounding box offset in degrees (0.05 deg is roughly ~5-6 km radius)
LAT_DELTA = 0.05
LON_DELTA = 0.05

MIN_LAT = LAT - LAT_DELTA
MAX_LAT = LAT + LAT_DELTA
MIN_LON = LON - LON_DELTA
MAX_LON = LON + LON_DELTA

# Zoom levels:
# 10 = regional view, 15 = high detail, 16 = very high detail
MIN_ZOOM = 10
MAX_ZOOM = 16

# Terrain (elevation) tiles are much coarser than imagery. AWS Terrarium tiles
# top out at zoom 15 globally, but for most terrain-mesh purposes zoom 12-13
# is already more resolution than you need (and keeps download size sane).
TERRAIN_MIN_ZOOM = 10
TERRAIN_MAX_ZOOM = 13

IMAGERY_OUTPUT_DIR = "./maps/tiles"
TERRAIN_OUTPUT_DIR = "./maps/terrain"

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

        for x in range(x_min, x_max + 1):
            dir_path = os.path.join(output_dir, str(z), str(x))
            os.makedirs(dir_path, exist_ok=True)

            for y in range(y_min, y_max + 1):
                file_path = os.path.join(dir_path, f"{y}.{file_ext}")

                # Skip if already downloaded
                if os.path.exists(file_path):
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


if __name__ == "__main__":
    download_tiles()
    download_terrain_tiles()