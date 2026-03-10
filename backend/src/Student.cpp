#include "../include/Student.h"
#include <iostream>

// ============================================================
// Student.cpp — Extends Person with attendance features
// ============================================================

// Constructor: Call Person constructor + set attendance counts
Student::Student(int id, std::string name, int totalClasses,
                 int attendedClasses)
    : Person(id, name), totalClasses(totalClasses),
      attendedClasses(attendedClasses) {
  std::cout << "[Student] Registered: " << name << "\n";
}

// Destructor
Student::~Student() { std::cout << "[Student] Removed: " << getName() << "\n"; }

// Mark one attendance (increment attended count)
void Student::markAttendance() {
  attendedClasses++;
  std::cout << "[Attendance] Marked for: " << getName() << " ("
            << attendedClasses << "/" << totalClasses << ")\n";
}

// Increment total classes count for everyone
void Student::incrementTotalClasses() { totalClasses++; }

// Getters
int Student::getTotalClasses() const { return totalClasses; }

int Student::getAttendedClasses() const { return attendedClasses; }

// Calculate percentage: (attended / total) * 100
double Student::calculateAttendancePercentage() const {
  if (totalClasses == 0)
    return 0.0;
  return (static_cast<double>(attendedClasses) / totalClasses) * 100.0;
}

// Override displayInfo from Person
void Student::displayInfo() const {
  Person::displayInfo(); // Call base class version first
  std::cout << "  Total Classes : " << totalClasses << "\n"
            << "  Attended      : " << attendedClasses << "\n"
            << "  Attendance %  : " << calculateAttendancePercentage() << "%\n";
}
