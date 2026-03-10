#include "../include/GeoFence.h"
#include <cmath>
#include <iostream>

// ============================================================
// GeoFence.cpp — Haversine formula geofencing
// ============================================================

// Earth radius in meters
static const double EARTH_RADIUS_M = 6371000.0;

// Helper: convert degrees to radians
static double toRadians(double degrees) { return degrees * M_PI / 180.0; }

// Constructor
GeoFence::GeoFence(double lat, double lng, double radiusMeters)
    : centerLatitude(lat), centerLongitude(lng), radius(radiusMeters) {
  std::cout << "[GeoFence] Initialized. Center: (" << lat << ", " << lng
            << ") Radius: " << radiusMeters << "m\n";
}

// Destructor
GeoFence::~GeoFence() { std::cout << "[GeoFence] Destroyed.\n"; }

// Haversine formula: returns distance in meters between two GPS points
double GeoFence::calculateDistance(double lat1, double lng1, double lat2,
                                   double lng2) const {
  double dLat = toRadians(lat2 - lat1); // Difference in latitudes
  double dLng = toRadians(lng2 - lng1); // Difference in longitudes

  // Haversine formula components
  double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
             std::cos(toRadians(lat1)) * std::cos(toRadians(lat2)) *
                 std::sin(dLng / 2) * std::sin(dLng / 2);

  double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

  return EARTH_RADIUS_M * c; // Distance in meters
}

// Returns true if student's coordinates are within the allowed zone
bool GeoFence::isInsideFence(double lat, double lng) const {
  double distance =
      calculateDistance(centerLatitude, centerLongitude, lat, lng);
  std::cout << "[GeoFence] Distance from center: " << distance
            << "m (Allowed: " << radius << "m)\n";
  return distance <= radius;
}

// Getters
double GeoFence::getCenterLat() const { return centerLatitude; }
double GeoFence::getCenterLng() const { return centerLongitude; }
double GeoFence::getRadius() const { return radius; }
