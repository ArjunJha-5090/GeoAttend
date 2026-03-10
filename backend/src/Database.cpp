#include "../include/Database.h"
#include "../include/AttendanceManager.h"
#include <iostream>
#include <stdexcept>

// ============================================================
// Database.cpp — SQLite3 persistence layer
// ============================================================

// Constructor: open database and create tables
Database::Database(const std::string &path) : dbPath(path) {
  // Open (or create) the SQLite database file
  if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
    std::cerr << "[Database] Error opening DB: " << sqlite3_errmsg(db) << "\n";
    throw std::runtime_error("Failed to open database.");
  }
  std::cout << "[Database] Opened: " << path << "\n";
  createTables(); // Create tables if not already there
}

// Destructor: close the database connection
Database::~Database() {
  sqlite3_close(db);
  std::cout << "[Database] Connection closed.\n";
}

// Create necessary tables
void Database::createTables() {
  const char *createStudents = R"(
        CREATE TABLE IF NOT EXISTS Students (
            id              INTEGER PRIMARY KEY,
            name            TEXT NOT NULL,
            totalClasses    INTEGER DEFAULT 0,
            attendedClasses INTEGER DEFAULT 0
        );
    )";

  const char *createLogs = R"(
        CREATE TABLE IF NOT EXISTS AttendanceLogs (
            logId    INTEGER PRIMARY KEY AUTOINCREMENT,
            studentId INTEGER NOT NULL,
            date      TEXT,
            latitude  REAL,
            longitude REAL,
            status    INTEGER
        );
    )";

  char *errMsg = nullptr;
  if (sqlite3_exec(db, createStudents, nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    std::cerr << "[Database] Error creating Students table: " << errMsg << "\n";
    sqlite3_free(errMsg);
  }
  if (sqlite3_exec(db, createLogs, nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "[Database] Error creating AttendanceLogs table: " << errMsg
              << "\n";
    sqlite3_free(errMsg);
  }
  std::cout << "[Database] Tables ready.\n";
}

// Save a new student
void Database::saveStudent(int id, const std::string &name, int totalClasses,
                           int attendedClasses) {
  std::string sql = "INSERT OR REPLACE INTO Students (id, name, totalClasses, "
                    "attendedClasses) VALUES (" +
                    std::to_string(id) + ",'" + name + "'," +
                    std::to_string(totalClasses) + "," +
                    std::to_string(attendedClasses) + ");";

  char *errMsg = nullptr;
  if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "[Database] saveStudent error: " << errMsg << "\n";
    sqlite3_free(errMsg);
  }
}

// Update attendance counts for a student
void Database::updateAttendance(int studentId, int totalClasses,
                                int attendedClasses) {
  std::string sql =
      "UPDATE Students SET totalClasses=" + std::to_string(totalClasses) +
      ", attendedClasses=" + std::to_string(attendedClasses) +
      " WHERE id=" + std::to_string(studentId) + ";";

  char *errMsg = nullptr;
  if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "[Database] updateAttendance error: " << errMsg << "\n";
    sqlite3_free(errMsg);
  }
}

// Log a single attendance event
void Database::logAttendance(int studentId, const std::string &date, double lat,
                             double lng, bool status) {
  std::string sql = "INSERT INTO AttendanceLogs (studentId, date, latitude, "
                    "longitude, status) VALUES (" +
                    std::to_string(studentId) + ",'" + date + "'," +
                    std::to_string(lat) + "," + std::to_string(lng) + "," +
                    std::to_string(status ? 1 : 0) + ");";

  char *errMsg = nullptr;
  if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "[Database] logAttendance error: " << errMsg << "\n";
    sqlite3_free(errMsg);
  }
}

// Check if a student already exists
bool Database::studentExists(int id) {
  std::string sql =
      "SELECT COUNT(*) FROM Students WHERE id=" + std::to_string(id) + ";";
  sqlite3_stmt *stmt;
  int count = 0;

  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      count = sqlite3_column_int(stmt, 0);
    }
  }
  sqlite3_finalize(stmt);
  return count > 0;
}
