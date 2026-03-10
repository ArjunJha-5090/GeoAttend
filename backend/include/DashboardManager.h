#pragma once
#include "AttendanceManager.h"
#include <string>

// ============================================================
// DashboardManager.h — Computes dashboard statistics
// Risk levels, attendance margin, and summary data
// ============================================================

class DashboardManager {
private:
  AttendanceManager &attendanceManager; // Reference to shared attendance data

  // Risk level thresholds
  static constexpr double SAFE_THRESHOLD = 85.0;
  static constexpr double WARNING_THRESHOLD = 80.0;
  static constexpr double DANGER_THRESHOLD = 75.0;

public:
  // Constructor takes reference to AttendanceManager
  DashboardManager(AttendanceManager &manager);
  ~DashboardManager();

  // Calculate attendance percentage for a student
  double calculateAttendancePercentage(int studentId) const;

  // Calculate margin: how many more classes can be missed
  int calculateMargin(int studentId) const;

  // Get risk level string for a student
  std::string getRiskLevel(int studentId) const;

  // Build a full JSON string for the dashboard endpoint
  std::string buildDashboardJSON(int studentId) const;

  // Print dashboard to console
  void displayDashboard(int studentId) const;
};
