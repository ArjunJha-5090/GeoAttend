#pragma once
#include <sqlite3.h>
#include <string>

// ============================================================
// Database.h — SQLite3 integration
// Handles persistent storage for students and attendance logs
// ============================================================

class Database {
private:
  sqlite3 *db;        // SQLite database handle
  std::string dbPath; // Path to the .db file

  // Create tables if they don't exist yet
  void createTables();

public:
  // Constructor — opens the database
  Database(const std::string &path);

  // Destructor — closes the database
  ~Database();

  // Save a student record
  void saveStudent(int id, const std::string &name, int totalClasses,
                   int attendedClasses);

  // Update attendance record for a student
  void updateAttendance(int studentId, int totalClasses, int attendedClasses);

  // Log a single attendance event
  void logAttendance(int studentId, const std::string &date, double lat,
                     double lng, bool status);

  // Load students from DB (returns row count)
  int loadStudents(void *managerPtr);

  // Check if a student already exists
  bool studentExists(int id);
};
