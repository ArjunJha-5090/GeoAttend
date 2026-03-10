#pragma once
#include "Person.h"

// ============================================================
// Student.h — Inherits from Person
// Adds attendance tracking features
// ============================================================

class Student : public Person {
private:
  int totalClasses;    // Total number of classes held
  int attendedClasses; // Number of classes the student attended

public:
  // Constructor
  Student(int id, std::string name, int totalClasses = 0,
          int attendedClasses = 0);

  // Destructor
  ~Student();

  // Attendance operations
  void markAttendance();        // Increment attendedClasses by 1
  void incrementTotalClasses(); // Increment totalClasses by 1

  // Getters
  int getTotalClasses() const;
  int getAttendedClasses() const;

  // Calculate attendance percentage
  double calculateAttendancePercentage() const;

  // Override displayInfo from Person
  void displayInfo() const override;
};
