#include "../include/AttendanceManager.h"
#include <algorithm>
#include <iostream>

// ============================================================
// AttendanceManager.cpp — Manages all students
// ============================================================

// Constructor
AttendanceManager::AttendanceManager() {
  std::cout << "[AttendanceManager] Initialized.\n";
}

// Destructor
AttendanceManager::~AttendanceManager() {
  std::cout << "[AttendanceManager] Shutting down. Total students: "
            << students.size() << "\n";
}

// Add a new student
void AttendanceManager::addStudent(int id, const std::string &name) {
  // Check if student with same ID already exists
  for (const auto &s : students) {
    if (s.getID() == id) {
      std::cout << "[AttendanceManager] Student ID " << id
                << " already exists.\n";
      return;
    }
  }
  students.emplace_back(id, name); // Construct in place
  std::cout << "[AttendanceManager] Added student: " << name << "\n";
}

// Mark attendance for a student by ID
bool AttendanceManager::markAttendance(int studentId) {
  for (auto &s : students) {
    if (s.getID() == studentId) {
      s.markAttendance();
      return true; // Found and marked
    }
  }
  std::cout << "[AttendanceManager] Student ID " << studentId
            << " not found.\n";
  return false;
}

// Increment total class count for ALL students
void AttendanceManager::incrementTotalClasses() {
  for (auto &s : students) {
    s.incrementTotalClasses();
  }
}

// Get attendance percentage for one student
double AttendanceManager::getAttendancePercentage(int studentId) const {
  for (const auto &s : students) {
    if (s.getID() == studentId) {
      return s.calculateAttendancePercentage();
    }
  }
  return -1.0; // Student not found
}

// Get a pointer to a student by ID
Student *AttendanceManager::getStudent(int studentId) {
  for (auto &s : students) {
    if (s.getID() == studentId) {
      return &s;
    }
  }
  return nullptr;
}

// Print all students
void AttendanceManager::generateReport() const {
  std::cout << "\n===== Attendance Report =====\n";
  for (const auto &s : students) {
    s.displayInfo();
    std::cout << "-----------------------------\n";
  }
}

// Return reference to all students (for JSON building)
const std::vector<Student> &AttendanceManager::getAllStudents() const {
  return students;
}
