# 📍 GeoAttend — Geofenced Attendance Tracker

A full-stack OODP project combining a **C++ OOP Backend** with a **modern Web Frontend**.

**Core OOP Concepts Used:** Inheritance, Polymorphism (abstract classes), Encapsulation, Constructors/Destructors

---

## 🏗️ Project Architecture

```
Browser (GPS)
     │  Fetch API
     ▼
C++ Crow REST Backend (port 8080)
     │
 ┌───┼───────────────────────┐
 ▼   ▼                       ▼
AttendanceManager   GeoFence   NotificationManager
(vector<Student>)   (Haversine) (Email + SMS)
      ▲                              ▲
    Student ──inherits──> Person     │ (Polymorphism)
      │                              │
      └──────── DashboardManager ────┘
                     │
                  SQLite DB
```

---

## 🧬 OOP Class Hierarchy

| Class | Type | Inherits From |
|-------|------|---------------|
| `Person` | Base | — |
| `Student` | Derived | `Person` |
| `Notification` | Abstract | — |
| `EmailNotification` | Concrete | `Notification` |
| `SMSNotification` | Concrete | `Notification` |
| `AttendanceManager` | Standalone | — |
| `GeoFence` | Standalone | — |
| `NotificationManager` | Standalone | — |
| `DashboardManager` | Standalone | — |
| `Database` | Standalone | — |

---

## ⚙️ Tech Stack

| Layer | Technology |
|-------|-----------|
| Backend Language | C++17 (OOP) |
| Web Framework | [Crow](https://crowcpp.org) |
| Database | SQLite3 |
| Frontend | HTML5 + CSS3 + JavaScript |
| Maps | Google Maps JS API |
| Charts | Chart.js 4 |

---

## 🚀 Setup & Build

### 1. Install Dependencies (macOS)

```bash
brew install crow sqlite3
```

### 2. Build the C++ Backend

```bash
cd GeoAttend/backend
g++ -std=c++17 main.cpp src/*.cpp \
    -I./include \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib \
    -lsqlite3 -lpthread \
    -o geoattend
```

### 3. Run the Backend Server

```bash
./geoattend
# Server starts at http://localhost:8080
```

### 4. Serve the Frontend

```bash
cd GeoAttend/frontend
python3 -m http.server 3000
# Open http://localhost:3000
```

### 5. Add Google Maps API Key

In `dashboard.html`, replace `YOUR_API_KEY`:
```html
<script src="https://maps.googleapis.com/maps/api/js?key=YOUR_API_KEY&callback=initMap">
```

---

## 🌐 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/addStudent` | Register a new student |
| POST | `/markAttendance` | Mark attendance (with geofence check) |
| GET | `/attendance/:id` | Get attendance data for a student |
| GET | `/dashboard/:id` | Full dashboard stats + risk level |
| GET | `/geofence` | Geofence center + radius (for map) |
| GET | `/students` | List all registered students |

### Example Requests

```bash
# Register student
curl -X POST http://localhost:8080/addStudent \
  -H "Content-Type: application/json" \
  -d '{"id":101,"name":"Arjun Jha"}'

# Mark attendance with GPS
curl -X POST http://localhost:8080/markAttendance \
  -H "Content-Type: application/json" \
  -d '{"studentId":101,"lat":28.5450,"lng":77.1926}'

# Get dashboard
curl http://localhost:8080/dashboard/101
```

---

## 📊 Risk Levels

| Level | Attendance % | Color |
|-------|-------------|-------|
| ✅ Safe | > 85% | Green |
| ⚠️ Warning | 80–85% | Yellow |
| 🔴 Danger | 75–80% | Orange |
| 🚨 Critical | < 75% | Red |

---

## 🗂️ Folder Structure

```
GeoAttend/
├── backend/
│   ├── include/       ← C++ header files (.h)
│   ├── src/           ← C++ source files (.cpp)
│   └── main.cpp       ← Crow REST API server
├── frontend/
│   ├── index.html     ← Landing page
│   ├── dashboard.html ← Dashboard
│   ├── style.css      ← Design system
│   └── app.js         ← Frontend logic
└── database/
    └── attendance.db  ← SQLite (auto-created)
```
