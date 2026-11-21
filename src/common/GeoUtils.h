#pragma once

#include "Types.h"

// 구면 지구 근사(평균 반경) 기반 측지 계산. 거리 단위: km, 각도 단위: deg
namespace GeoUtils {

inline constexpr double EarthRadiusKm = 6371.0088;

double distanceKm(const GeoPoint& a, const GeoPoint& b);

// a → b 초기 방위각 [0, 360), 북쪽 0, 시계방향 증가
double bearingDeg(const GeoPoint& a, const GeoPoint& b);

// start에서 bearing 방향으로 distance 이동한 지점
GeoPoint destination(const GeoPoint& start, double bearingDeg, double distanceKm);

// 대권 경로상 비율 f(0~1) 지점
GeoPoint interpolate(const GeoPoint& a, const GeoPoint& b, double f);

double normalizeDeg(double deg);          // [0, 360)
double angleDiffDeg(double a, double b);  // (-180, 180], a - b

} // namespace GeoUtils
