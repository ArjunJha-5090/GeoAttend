#pragma once
#include "Student.h"
#include <string>
#include <vector>

// ============================================================
// AttendanceManager.h — Manages all students and records
// Stores a list of students and provides CRUD operations
// ============================================================

class AttendanceManager {
private:
  std::vector<Student> students; // List of all students

public:
  // Constructor — initializes empty student list
  AttendanceManager();

  // Destructor — logs shutdown
  ~AttendanceManager();

  // Add a new student to the list
  void addStudent(int id, const std::string &name);

  // Mark attendance for a student by ID (returns true if found)
  bool markAttendance(int studentId);

  // Increment total classes for all students
  void incrementTotalClasses();

  // Get attendance percentage for a specific student
  double getAttendancePercentage(int studentId) const;

  // Get student by ID (returns nullptr if not found)
  Student *getStudent(int studentId);

  // Print all student records
  void generateReport() const;

  // Get all students (for JSON serialization)
  const std::vector<Student> &getAllStudents() const;
};
