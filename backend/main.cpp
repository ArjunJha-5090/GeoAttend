/*
 * ============================================================
 * main.cpp — GeoAttend REST API Server (Crow Framework)
 *
 * Endpoints:
 *   POST /addStudent          — Register a new student
 *   POST /markAttendance      — Mark attendance (with geofence check)
 *   GET  /attendance/:id      — Get a student's attendance data
 *   GET  /dashboard/:id       — Get full dashboard JSON for a student
 *   GET  /geofence            — Get geofence config (for map rendering)
 *   GET  /students            — List all students
 * ============================================================
 */

// NOTE: Install Crow and SQLite3 before building:
//   brew install crow sqlite3
// Build with:
//   g++ -std=c++17 main.cpp src/*.cpp -I./include -I/opt/homebrew/include
//   -L/opt/homebrew/lib -lsqlite3 -lpthread -o geoattend
// Run:
//   ./geoattend

#include "include/AttendanceManager.h"
#include "include/DashboardManager.h"
#include "include/Database.h"
#include "include/GeoFence.h"
#include "include/Notification.h"

#include <crow.h>
#include <ctime>
#include <sstream>

// ============================================================
// GEOFENCE CONFIGURATION — Change these for your location
// Default: IIT Delhi campus coordinates
// ============================================================
static const double FENCE_LAT = 28.5450;  // Center latitude
static const double FENCE_LNG = 77.1926;  // Center longitude
static const double FENCE_RADIUS = 200.0; // Radius in meters

// ============================================================
// Helper: Get current date as string
// ============================================================
std::string getCurrentDate() {
  time_t now = time(nullptr);
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&now));
  return std::string(buf);
}

// ============================================================
// MAIN — Entry point: Initialize systems and start server
// ============================================================
int main() {
  std::cout << "==============================\n";
  std::cout << "  GeoAttend Server Starting  \n";
  std::cout << "==============================\n";

  // Initialize core systems
  AttendanceManager attendanceMgr;
  GeoFence geoFence(FENCE_LAT, FENCE_LNG, FENCE_RADIUS);
  DashboardManager dashboardMgr(attendanceMgr);
  NotificationManager notifMgr;
  Database db("../database/attendance.db");

  // Add notification channels (simulated)
  notifMgr.addNotifier(
      std::make_unique<EmailNotification>("admin@geoattend.com"));
  notifMgr.addNotifier(std::make_unique<SMSNotification>("+91-9999999999"));

  // Crow web app
  crow::SimpleApp app;

  // ─────────────────────────────────────────────
  // CORS Middleware (inline, for browser support)
  // ─────────────────────────────────────────────
  auto setCORS = [](crow::response &res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
    res.add_header("Content-Type", "application/json");
  };

  // ─────────────────────────────────────────────
  // POST /addStudent
  // Body: { "id": 1, "name": "Arjun" }
  // ─────────────────────────────────────────────
  CROW_ROUTE(app, "/addStudent")
      .methods("POST"_method, "OPTIONS"_method)([&](const crow::request &req) {
        crow::response res;
        setCORS(res);
        if (req.method == crow::HTTPMethod::Options) {
          res.code = 200;
          return res;
        }

        auto body = crow::json::load(req.body);
        if (!body || !body.has("id") || !body.has("name")) {
          res.code = 400;
          res.body = R"({"error":"Missing id or name"})";
          return res;
        }

        int id = body["id"].i();
        std::string name = body["name"].s();

        attendanceMgr.addStudent(id, name);
        db.saveStudent(id, name, 0, 0);

        res.code = 200;
        res.body = "{\"message\":\"Student added successfully\",\"id\":" +
                   std::to_string(id) + "}";
        return res;
      });

  // ─────────────────────────────────────────────
  // POST /markAttendance
  // Body: { "studentId": 1, "lat": 28.5450, "lng": 77.1926 }
  // ─────────────────────────────────────────────
  CROW_ROUTE(app, "/markAttendance")
      .methods("POST"_method, "OPTIONS"_method)([&](const crow::request &req) {
        crow::response res;
        setCORS(res);
        if (req.method == crow::HTTPMethod::Options) {
          res.code = 200;
          return res;
        }

        auto body = crow::json::load(req.body);
        if (!body || !body.has("studentId") || !body.has("lat") ||
            !body.has("lng")) {
          res.code = 400;
          res.body = R"({"error":"Missing studentId, lat, or lng"})";
          return res;
        }

        int studentId = body["studentId"].i();
        double lat = body["lat"].d();
        double lng = body["lng"].d();

        // Check geofence FIRST
        if (!geoFence.isInsideFence(lat, lng)) {
          res.code = 403;
          res.body =
              R"({"success":false,"message":"You are outside the allowed location. Attendance not marked."})";
          return res;
        }

        // Increment total and mark attendance
        attendanceMgr.incrementTotalClasses();
        bool marked = attendanceMgr.markAttendance(studentId);

        if (!marked) {
          res.code = 404;
          res.body = R"({"error":"Student not found"})";
          return res;
        }

        // Persist to database
        Student *s = attendanceMgr.getStudent(studentId);
        if (s) {
          db.updateAttendance(studentId, s->getTotalClasses(),
                              s->getAttendedClasses());
          db.logAttendance(studentId, getCurrentDate(), lat, lng, true);
        }

        // Check if notification is needed
        double pct = attendanceMgr.getAttendancePercentage(studentId);
        std::string warning =
            notifMgr.checkAndNotify(s ? s->getName() : "Unknown", pct);

        std::ostringstream json;
        json << "{\"success\":true,"
             << "\"message\":\"Attendance marked successfully\","
             << "\"attendancePercentage\":" << std::fixed
             << std::setprecision(2) << pct;
        if (!warning.empty()) {
          json << ",\"warning\":\"" << warning << "\"";
        }
        json << "}";

        res.code = 200;
        res.body = json.str();
        return res;
      });

  // ─────────────────────────────────────────────
  // GET /attendance/:id
  // Returns attendance data for a student
  // ─────────────────────────────────────────────
  CROW_ROUTE(app, "/attendance/<int>")
  ([&](const crow::request &, int studentId) {
    crow::response res;
    setCORS(res);

    Student *s = attendanceMgr.getStudent(studentId);
    if (!s) {
      res.code = 404;
      res.body = R"({"error":"Student not found"})";
      return res;
    }

    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    json << "{"
         << "\"id\":" << s->getID() << ","
         << "\"name\":\"" << s->getName() << "\","
         << "\"totalClasses\":" << s->getTotalClasses() << ","
         << "\"attendedClasses\":" << s->getAttendedClasses() << ","
         << "\"attendancePercentage\":" << s->calculateAttendancePercentage()
         << "}";

    res.code = 200;
    res.body = json.str();
    return res;
  });

  // ─────────────────────────────────────────────
  // GET /dashboard/:id
  // Returns full dashboard stats with risk level
  // ─────────────────────────────────────────────
  CROW_ROUTE(app, "/dashboard/<int>")
  ([&](const crow::request &, int studentId) {
    crow::response res;
    setCORS(res);
    res.code = 200;
    res.body = dashboardMgr.buildDashboardJSON(studentId);
    return res;
  });

  // ─────────────────────────────────────────────
  // GET /geofence
  // Returns geofence center and radius for the map
  // ─────────────────────────────────────────────
  CROW_ROUTE(app, "/geofence")
  ([&]() {
    crow::response res;
    setCORS(res);
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    json << "{"
         << "\"centerLat\":" << FENCE_LAT << ","
         << "\"centerLng\":" << FENCE_LNG << ","
         << "\"radius\":" << FENCE_RADIUS << "}";
    res.code = 200;
    res.body = json.str();
    return res;
  });

  // ─────────────────────────────────────────────
  // GET /students
  // Returns all students
  // ─────────────────────────────────────────────
  CROW_ROUTE(app, "/students")
  ([&]() {
    crow::response res;
    setCORS(res);
    const auto &students = attendanceMgr.getAllStudents();
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < students.size(); ++i) {
      if (i > 0)
        json << ",";
      json << "{\"id\":" << students[i].getID() << ",\"name\":\""
           << students[i].getName() << "\""
           << ",\"percentage\":" << std::fixed << std::setprecision(2)
           << students[i].calculateAttendancePercentage() << "}";
    }
    json << "]";
    res.code = 200;
    res.body = json.str();
    return res;
  });

  // Start server on port 8080
  std::cout << "[Server] GeoAttend running on http://localhost:8080\n";
  app.port(8080).multithreaded().run();

  return 0;
}
