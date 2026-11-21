#!/usr/bin/env python3
"""prep_map.py 입력용 Natural Earth 10m GeoJSON 원본 다운로드 (표준 라이브러리만 사용).

사용법: python3 tools/map_prep/download_ne.py <저장폴더>
"""
import os
import sys
import urllib.request

BASE = "https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/"
FILES = [
    "ne_10m_coastline",
    "ne_10m_admin_0_countries",
    "ne_10m_admin_0_boundary_lines_land",
    "ne_10m_admin_1_states_provinces_lines",
    "ne_10m_rivers_lake_centerlines",
    "ne_10m_geography_regions_elevation_points",
]


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 1
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    for name in FILES:
        dst = os.path.join(out, name + ".geojson")
        if os.path.exists(dst):
            print("skip", dst)
            continue
        print("download", name)
        urllib.request.urlretrieve(BASE + name + ".geojson", dst)
    return 0


if __name__ == "__main__":
    sys.exit(main())
