#pragma once

// ============================================================
// GeoFence.h — Geofencing logic using Haversine formula
// Checks if a student's GPS coordinates are within the zone
// ============================================================

class GeoFence {
private:
  double centerLatitude;  // Center of the allowed zone (latitude)
  double centerLongitude; // Center of the allowed zone (longitude)
  double radius;          // Allowed radius in meters

public:
  // Constructor
  GeoFence(double lat, double lng, double radiusMeters);

  // Destructor
  ~GeoFence();

  // Calculate distance between two coordinates (Haversine formula)
  // Returns distance in meters
  double calculateDistance(double lat1, double lng1, double lat2,
                           double lng2) const;

  // Returns true if (lat, lng) is within the allowed radius
  bool isInsideFence(double lat, double lng) const;

  // Getters for frontend use
  double getCenterLat() const;
  double getCenterLng() const;
  double getRadius() const;
};
