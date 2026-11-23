#!/usr/bin/env python3
"""
Natural Earth 10m GeoJSON → 경량 GeoJSON (resources/map) 변환 스크립트.

외부 패키지 없이 Python 3.8+ 표준 라이브러리만 사용 (macOS / Windows 공통).

사용법:
    python3 tools/map_prep/download_ne.py  <원본폴더>         # 원본 다운로드
    python3 tools/map_prep/prep_map.py --src <원본폴더> --out resources/map

처리:
    1) 영역 clip (기본 E116~137, N29~46) — 선: 선분 단위 절단, 면: Sutherland-Hodgman
    2) Douglas-Peucker 단순화 (레이어별 허용오차, 단위 deg)
    3) 좌표 소수점 4자리 반올림 (~11 m)
"""
import argparse
import json
import math
import os
import sys

BBOX = (116.0, 29.0, 137.0, 46.0)  # minLon, minLat, maxLon, maxLat
KOREA = {"KOR", "PRK"}
NDIGITS = 4

# 한반도 주요 산 (Natural Earth 포인트가 부족·오표기되어 직접 보강). 좌표는 정상 기준 근사값.
# Natural Earth 10m 에 없는 작은 섬 (면적 기준 누락) — 직접 추가.
# (이름, 중심 경도, 중심 위도, 반경 deg) 근사 팔각형 윤곽
KOREAN_ISLETS = [
    ("독도 서도", 131.8646, 37.2426, 0.0016),
    ("독도 동도", 131.8697, 37.2411, 0.0013),
]
# 섬 이름 라벨 (작아서 지도 축척에서 윤곽이 보이지 않으므로 이름 표시)
KOREAN_ISLAND_LABELS = [
    ("독도", 131.8670, 37.2420),
    ("울릉도", 130.8624, 37.4977),
]


def islet_ring(lon, lat, r):
    pts = []
    for k in range(8):
        a = math.radians(22.5 + 45 * k)
        pts.append([round(lon + r * math.cos(a) / math.cos(math.radians(lat)), 5), round(lat + r * math.sin(a), 5)])
    pts.append(pts[0])
    return pts


KOREAN_PEAKS = [
    ("백두산", 2744, 128.0560, 42.0063),
    ("관모봉", 2541, 129.2075, 41.7064),
    ("묘향산", 1909, 126.3100, 40.0128),
    ("낭림산", 2186, 126.9000, 40.5050),
    ("금강산", 1638, 128.0600, 38.6560),
    ("설악산", 1708, 128.4653, 38.1194),
    ("오대산", 1563, 128.5433, 37.7944),
    ("태백산", 1567, 128.9164, 37.0972),
    ("소백산", 1440, 128.4853, 36.9578),
    ("속리산", 1058, 127.8711, 36.5431),
    ("덕유산", 1614, 127.7458, 35.8597),
    ("지리산", 1915, 127.7306, 35.3369),
    ("한라산", 1947, 126.5331, 33.3617),
    ("북한산", 836, 126.9817, 37.6589),
    ("계룡산", 845, 127.2056, 36.3439),
    ("치악산", 1288, 128.0567, 37.3653),
    ("가야산", 1433, 128.1194, 35.8228),
    ("월출산", 809, 126.7136, 34.7692),
    ("가리왕산", 1561, 128.5622, 37.4614),
    ("팔공산", 1193, 128.6950, 36.0161),
]


# ----------------------------------------------------------------------------- geometry
def inside(p, b=BBOX):
    return b[0] <= p[0] <= b[2] and b[1] <= p[1] <= b[3]


def clip_segment(p, q, b=BBOX):
    """Liang-Barsky 선분 clip. 반환: (p', q') 또는 None"""
    x0, y0 = p
    dx, dy = q[0] - x0, q[1] - y0
    t0, t1 = 0.0, 1.0
    for pk, qk in ((-dx, x0 - b[0]), (dx, b[2] - x0), (-dy, y0 - b[1]), (dy, b[3] - y0)):
        if pk == 0:
            if qk < 0:
                return None
        else:
            r = qk / pk
            if pk < 0:
                t0 = max(t0, r)
            else:
                t1 = min(t1, r)
            if t0 > t1:
                return None
    return (x0 + t0 * dx, y0 + t0 * dy), (x0 + t1 * dx, y0 + t1 * dy)


def clip_line(coords):
    """LineString → bbox 안쪽 조각 목록"""
    parts, cur = [], []
    for p, q in zip(coords, coords[1:]):
        seg = clip_segment(p, q)
        if seg is None:
            if len(cur) >= 2:
                parts.append(cur)
            cur = []
            continue
        a, c = seg
        if cur and cur[-1] == tuple(a):
            cur.append(tuple(c))
        else:
            if len(cur) >= 2:
                parts.append(cur)
            cur = [tuple(a), tuple(c)]
        if tuple(c) != tuple(q):  # 밖으로 나감
            parts.append(cur)
            cur = []
    if len(cur) >= 2:
        parts.append(cur)
    return parts


def clip_ring(ring, b=BBOX):
    """Sutherland-Hodgman: 다각형 링을 사각형으로 clip"""
    def edge_clip(pts, inside_fn, intersect_fn):
        out = []
        if not pts:
            return out
        prev = pts[-1]
        for cur in pts:
            if inside_fn(cur):
                if not inside_fn(prev):
                    out.append(intersect_fn(prev, cur))
                out.append(cur)
            elif inside_fn(prev):
                out.append(intersect_fn(prev, cur))
            prev = cur
        return out

    def ix(x):
        return lambda p, q: (x, p[1] + (q[1] - p[1]) * (x - p[0]) / (q[0] - p[0]))

    def iy(y):
        return lambda p, q: (p[0] + (q[0] - p[0]) * (y - p[1]) / (q[1] - p[1]), y)

    pts = [tuple(p) for p in ring[:-1]] if ring and ring[0] == ring[-1] else [tuple(p) for p in ring]
    pts = edge_clip(pts, lambda p: p[0] >= b[0], ix(b[0]))
    pts = edge_clip(pts, lambda p: p[0] <= b[2], ix(b[2]))
    pts = edge_clip(pts, lambda p: p[1] >= b[1], iy(b[1]))
    pts = edge_clip(pts, lambda p: p[1] <= b[3], iy(b[3]))
    if len(pts) < 3:
        return None
    return pts + [pts[0]]


def simplify(pts, tol):
    """Douglas-Peucker (반복형)"""
    n = len(pts)
    if n < 3 or tol <= 0:
        return list(pts)
    keep = [False] * n
    keep[0] = keep[-1] = True
    stack = [(0, n - 1)]
    tol2 = tol * tol
    while stack:
        i, j = stack.pop()
        ax, ay = pts[i]
        bx, by = pts[j]
        dx, dy = bx - ax, by - ay
        L2 = dx * dx + dy * dy
        best, idx = -1.0, -1
        for k in range(i + 1, j):
            px, py = pts[k]
            if L2 == 0:
                d2 = (px - ax) ** 2 + (py - ay) ** 2
            else:
                t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / L2))
                d2 = (px - ax - t * dx) ** 2 + (py - ay - t * dy) ** 2
            if d2 > best:
                best, idx = d2, k
        if best > tol2:
            keep[idx] = True
            stack.append((i, idx))
            stack.append((idx, j))
    return [p for p, k in zip(pts, keep) if k]


def rnd(pts):
    out = []
    for x, y in pts:
        p = [round(x, NDIGITS), round(y, NDIGITS)]
        if not out or out[-1] != p:
            out.append(p)
    return out


def lines_of(geom):
    t = geom["type"]
    if t == "LineString":
        return [geom["coordinates"]]
    if t == "MultiLineString":
        return geom["coordinates"]
    return []


def polys_of(geom):
    t = geom["type"]
    if t == "Polygon":
        return [geom["coordinates"]]
    if t == "MultiPolygon":
        return geom["coordinates"]
    return []


def process_lines(geom, tol):
    out = []
    for line in lines_of(geom):
        for part in clip_line([tuple(p[:2]) for p in line]):
            s = rnd(simplify(part, tol))
            if len(s) >= 2:
                out.append(s)
    return out


def process_polys(geom, tol, min_area=0.0):
    out = []
    for poly in polys_of(geom):
        rings = []
        for i, ring in enumerate(poly):
            c = clip_ring(ring)
            if c is None:
                if i == 0:
                    break
                continue
            s = rnd(simplify(c, tol))
            if len(s) >= 4 and abs(ring_area(s)) > min_area:
                rings.append(s)
            elif i == 0:
                break
        if rings:
            out.append(rings)
    return out


def ring_area(r):
    return 0.5 * sum(r[i][0] * r[i + 1][1] - r[i + 1][0] * r[i][1] for i in range(len(r) - 1))


# ----------------------------------------------------------------------------- io
def load(src, name):
    path = os.path.join(src, name + ".geojson")
    with open(path, encoding="utf-8") as f:
        return json.load(f)["features"]


def feature(geom, props=None):
    return {"type": "Feature", "properties": props or {}, "geometry": geom}


def multiline(parts):
    return {"type": "MultiLineString", "coordinates": parts}


def save(out_dir, name, feats, meta):
    path = os.path.join(out_dir, name + ".geojson")
    fc = {"type": "FeatureCollection", "name": name, "metadata": meta, "features": feats}
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(fc, f, ensure_ascii=False, separators=(",", ":"))
    nverts = sum(_count(ft["geometry"]) for ft in feats)
    print(f"  {name:<22} features={len(feats):5d} vertices={nverts:7d} size={os.path.getsize(path)/1024:8.1f} KB")


def _count(g):
    t, c = g["type"], g["coordinates"]
    if t == "Point":
        return 1
    if t == "LineString":
        return len(c)
    if t in ("MultiLineString", "Polygon"):
        return sum(len(x) for x in c)
    if t == "MultiPolygon":
        return sum(len(r) for p in c for r in p)
    return 0


# ----------------------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--src", required=True, help="Natural Earth 10m GeoJSON 원본 폴더")
    ap.add_argument("--out", required=True, help="출력 폴더 (resources/map)")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    meta = {"source": "Natural Earth 10m (public domain), naturalearthdata.com", "bbox": BBOX}

    print("countries / kr_land ...")
    countries, kr_land, kr_vertex_cells = [], [], set()
    for ft in load(args.src, "ne_10m_admin_0_countries"):
        p = ft["properties"]
        a3 = p.get("ADM0_A3") or p.get("adm0_a3")
        polys = process_polys(ft["geometry"], 0.0015, min_area=1e-6)
        if not polys:
            continue
        props = {"a3": a3, "name": p.get("NAME_KO") or p.get("NAME")}
        if a3 in KOREA:
            kr_land.append(feature({"type": "MultiPolygon", "coordinates": polys}, props))
            # 한반도 해안선 분류용: 원본 정점 격자 (0.02°)
            for poly in polys_of(ft["geometry"]):
                for ring in poly:
                    for x, y in ring:
                        kr_vertex_cells.add((math.floor(x / 0.02), math.floor(y / 0.02)))
        else:
            countries.append(feature({"type": "MultiPolygon", "coordinates": polys}, props))
    save(args.out, "countries", countries, meta)
    # 독도 등 누락된 작은 섬 → 한반도 육지(대한민국 feature) 에 추가
    islets = [[islet_ring(lon, lat, r)] for _, lon, lat, r in KOREAN_ISLETS]
    for ft in kr_land:
        if ft["properties"].get("a3") == "KOR":
            ft["geometry"]["coordinates"].extend(islets)
            break
    save(args.out, "kr_land", kr_land, meta)

    def near_korea(x, y):
        cx, cy = math.floor(x / 0.02), math.floor(y / 0.02)
        return any((cx + i, cy + j) in kr_vertex_cells for i in (-1, 0, 1) for j in (-1, 0, 1))

    def split_korean_runs(line, min_run=3):
        """선을 한반도/그 외 구간으로 분할 (본토 해안선은 중국~한반도~러시아가 한 선이므로 정점 단위 판정).
        min_run 미만의 짧은 뒤집힘은 앞 구간에 흡수. 구간 경계 정점은 양쪽에 공유해 끊김 없게 함"""
        flags = [near_korea(x, y) for x, y in line]
        runs = []  # [flag, start, end]
        for i, f in enumerate(flags):
            if runs and runs[-1][0] == f:
                runs[-1][2] = i
            else:
                runs.append([f, i, i])
        merged = []
        for r in runs:
            if merged and (r[2] - r[1] + 1 < min_run or merged[-1][0] == r[0]):
                merged[-1][2] = r[2]
            else:
                merged.append(r)
        return [(f, line[a:b + 2] if b + 1 < len(line) else line[a:b + 1]) for f, a, b in merged]

    print("coastline ...")
    world, kr = [], []
    for ft in load(args.src, "ne_10m_coastline"):
        for line in lines_of(ft["geometry"]):
            for target_kr, seg in split_korean_runs([tuple(p[:2]) for p in line]):
                for part in clip_line(seg):
                    s = rnd(simplify(part, 0.0004 if target_kr else 0.0015))
                    if len(s) >= 2:
                        (kr if target_kr else world).append(s)
    save(args.out, "coastline_world", [feature(multiline(world))], meta)
    kr += [islet_ring(lon, lat, r) for _, lon, lat, r in KOREAN_ISLETS]   # 독도 해안선
    save(args.out, "kr_coastline", [feature(multiline(kr))], meta)

    print("islands ...")
    save(args.out, "kr_islands",
         [feature({"type": "Point", "coordinates": [lon, lat]}, {"name": n}) for n, lon, lat in KOREAN_ISLAND_LABELS],
         dict(meta, note="섬 이름 라벨 (직접 입력)"))

    print("borders / mdl / dmz ...")
    borders, mdl, dmz = [], [], []
    for ft in load(args.src, "ne_10m_admin_0_boundary_lines_land"):
        p = ft["properties"]
        name = p.get("NAME") or ""
        parts = process_lines(ft["geometry"], 0.0008)
        if not parts:
            continue
        if "Military Demarcation" in name:
            mdl += parts
        elif "Demilitarized" in name:
            dmz += parts
        else:
            borders += parts
    save(args.out, "borders", [feature(multiline(borders))], meta)
    save(args.out, "kr_mdl", [feature(multiline(mdl), {"name": "군사분계선"})], meta)
    save(args.out, "kr_dmz", [feature(multiline(dmz), {"name": "비무장지대"})], meta)

    print("admin1 ...")
    admin1 = []
    for ft in load(args.src, "ne_10m_admin_1_states_provinces_lines"):
        p = ft["properties"]
        if (p.get("ADM0_A3") or p.get("adm0_a3")) in KOREA:
            admin1 += process_lines(ft["geometry"], 0.0008)
    save(args.out, "kr_admin1", [feature(multiline(admin1))], meta)

    print("rivers ...")
    rivers = []
    for ft in load(args.src, "ne_10m_rivers_lake_centerlines"):
        p = ft["properties"]
        parts = process_lines(ft["geometry"], 0.001)
        if parts:
            rivers.append(feature(multiline(parts), {"name": p.get("name_ko") or p.get("name")}))
    save(args.out, "rivers", rivers, meta)

    print("mountains ...")
    peaks = [feature({"type": "Point", "coordinates": [lon, lat]}, {"name": n, "elevation": e})
             for n, e, lon, lat in KOREAN_PEAKS]
    for ft in load(args.src, "ne_10m_geography_regions_elevation_points"):
        g = ft["geometry"]
        if not g or g["type"] != "Point" or not inside(g["coordinates"]):
            continue
        lon, lat = g["coordinates"][:2]
        # 한반도 내부는 직접 목록 사용
        if (math.floor(lon / 0.02), math.floor(lat / 0.02)) in kr_vertex_cells or (124 <= lon <= 131 and 33 <= lat <= 43):
            continue
        p = ft["properties"]
        peaks.append(feature({"type": "Point", "coordinates": [round(lon, NDIGITS), round(lat, NDIGITS)]},
                             {"name": p.get("name"), "elevation": p.get("elevation")}))
    save(args.out, "kr_mountains", peaks, dict(meta, note="한반도 산: 직접 입력 근사 좌표, 그 외: Natural Earth"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
