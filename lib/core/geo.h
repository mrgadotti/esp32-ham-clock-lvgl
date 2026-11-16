#pragma once
bool latLonToLocator(double lat, double lon, char out[7]);
bool locatorToLatLon(const char* loc, double& lat, double& lon);  // 4 or 6 chars, square centre
float distanceKm(double lat1, double lon1, double lat2, double lon2);
float bearingDeg(double lat1, double lon1, double lat2, double lon2);
