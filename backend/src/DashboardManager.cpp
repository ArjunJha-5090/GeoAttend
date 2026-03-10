#include "../include/DashboardManager.h"
#include <iomanip>
#include <iostream>
#include <sstream>

// ============================================================
// DashboardManager.cpp — Dashboard stats and risk levels
// ============================================================

// Constructor: takes a reference to the shared AttendanceManager
DashboardManager::DashboardManager(AttendanceManager &manager)
    : attendanceManager(manager) {
  std::cout << "[DashboardManager] Initialized.\n";
}

DashboardManager::~DashboardManager() {
  std::cout << "[DashboardManager] Destroyed.\n";
}

// Get attendance percentage for a student
double DashboardManager::calculateAttendancePercentage(int studentId) const {
  return attendanceManager.getAttendancePercentage(studentId);
}

// Calculate how many more classes can be missed
// Formula: Allowed absences = total * 0.25; Margin = Allowed - Current absences
int DashboardManager::calculateMargin(int studentId) const {
  Student *s = attendanceManager.getStudent(studentId);
  if (!s)
    return -1;

  int total = s->getTotalClasses();
  int attended = s->getAttendedClasses();
  int absent = total - attended;

  int allowedAbsences = static_cast<int>(total * 0.25); // 25% absence allowed
  int margin = allowedAbsences - absent;

  return margin; // Positive = still safe, Negative = already overshot
}

// Return risk level string based on percentage
std::string DashboardManager::getRiskLevel(int studentId) const {
  double pct = calculateAttendancePercentage(studentId);
  if (pct > SAFE_THRESHOLD)
    return "Safe";
  if (pct > WARNING_THRESHOLD)
    return "Warning";
  if (pct > DANGER_THRESHOLD)
    return "Danger";
  return "Critical";
}

// Build a JSON string for the /dashboard API response
std::string DashboardManager::buildDashboardJSON(int studentId) const {
  Student *s = attendanceManager.getStudent(studentId);
  if (!s)
    return "{\"error\": \"Student not found\"}";

  double pct = s->calculateAttendancePercentage();
  int margin = calculateMargin(studentId);
  std::string risk = getRiskLevel(studentId);

  std::ostringstream json;
  json << std::fixed << std::setprecision(2);
  json << "{"
       << "\"studentId\":" << s->getID() << ","
       << "\"name\":\"" << s->getName() << "\","
       << "\"totalClasses\":" << s->getTotalClasses() << ","
       << "\"attendedClasses\":" << s->getAttendedClasses() << ","
       << "\"attendancePercentage\":" << pct << ","
       << "\"margin\":" << margin << ","
       << "\"riskLevel\":\"" << risk << "\""
       << "}";

  return json.str();
}

// Print to console (nice format)
void DashboardManager::displayDashboard(int studentId) const {
  Student *s = attendanceManager.getStudent(studentId);
  if (!s) {
    std::cout << "Student not found.\n";
    return;
  }

  double pct = s->calculateAttendancePercentage();
  int margin = calculateMargin(studentId);
  std::string risk = getRiskLevel(studentId);

  std::cout << "\n========== DASHBOARD ==========\n";
  std::cout << "Student       : " << s->getName() << " (ID: " << s->getID()
            << ")\n";
  std::cout << "Total Classes : " << s->getTotalClasses() << "\n";
  std::cout << "Attended      : " << s->getAttendedClasses() << "\n";
  std::cout << "Attendance %  : " << std::fixed << std::setprecision(2) << pct
            << "%\n";
  std::cout << "Margin Left   : " << margin << " classes\n";
  std::cout << "Risk Level    : " << risk << "\n";
  std::cout << "================================\n";
}
