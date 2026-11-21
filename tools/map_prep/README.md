# map_prep — 벡터지도 데이터 생성

`resources/map/*.geojson` 은 이 폴더의 스크립트로 생성한 결과물입니다. Python 3.8+ 표준 라이브러리만 사용합니다.

```bash
python3 tools/map_prep/download_ne.py /tmp/ne_raw                     # Natural Earth 10m 원본 (~60MB)
python3 tools/map_prep/prep_map.py --src /tmp/ne_raw --out resources/map
```

| 출력 | 원본 | 처리 |
|---|---|---|
| `countries` | admin_0_countries (한국·북한 제외) | 면, clip + 단순화 |
| `kr_land` | admin_0_countries (KOR, PRK) | 면 |
| `coastline_world` / `kr_coastline` | coastline | 정점 단위로 한반도 구간 분리(국가 폴리곤 정점 근접 판정) |
| `borders` | admin_0_boundary_lines_land | 군사분계선·DMZ 제외 |
| `kr_mdl` | 〃 "Military Demarcation Line 1953" | |
| `kr_dmz` | 〃 "Korean Demilitarized Zone" | |
| `kr_admin1` | admin_1_states_provinces_lines (KOR, PRK) | 시/도 경계 |
| `rivers` | rivers_lake_centerlines | |
| `kr_mountains` | 한반도: 스크립트 내 `KOREAN_PEAKS` 직접 입력(근사 좌표) / 그 외: geography_regions_elevation_points | |

- 영역: E116°~137°, N29°~46° (`BBOX`), 좌표 소수 4자리(~11 m)
- 라이선스: Natural Earth — Public Domain (출처 표기 권장: "Made with Natural Earth")
- 시/군/구 경계(`kr_admin2`)는 Natural Earth에 없음 → 필요 시 통계청 SGIS 등 공개 데이터로 추가 (라이선스 확인)
