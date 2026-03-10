// ============================================================
// app.js — GeoAttend Frontend Logic
// Handles GPS, API calls, map rendering, and chart updates
// ============================================================

const API_BASE = 'http://localhost:8080';

// ─────────────────────────────────────────────
// UTILITY: Show alert box
// type: 'success' | 'error' | 'warning' | 'info'
// ─────────────────────────────────────────────
function showAlert(containerId, type, message, icon = '') {
  const box = document.getElementById(containerId);
  if (!box) return;
  const icons = { success: '✅', error: '❌', warning: '⚠️', info: 'ℹ️' };
  box.className = `alert alert-${type} show`;
  const iconEl = box.querySelector('#alertIcon') || box.querySelector('[data-icon]');
  if (iconEl) iconEl.textContent = icon || icons[type];
  const msgEl = box.querySelector('#alertMsg') || box.querySelector('[data-msg]');
  if (msgEl) msgEl.textContent = message;
  else box.innerHTML = `<span>${icon || icons[type]}</span><span>${message}</span>`;
}

function hideAlert(containerId) {
  const box = document.getElementById(containerId);
  if (box) box.className = 'alert';
}

// ─────────────────────────────────────────────
// UTILITY: Toggle button loading state
// ─────────────────────────────────────────────
function setLoading(btnTextId, spinnerId, loading, text = '') {
  const btnText = document.getElementById(btnTextId);
  const spinner = document.getElementById(spinnerId);
  if (btnText) btnText.textContent = loading ? 'Please wait...' : text;
  if (spinner) spinner.className = loading ? 'spinner active' : 'spinner';
  const btn = spinner?.closest('button');
  if (btn) btn.disabled = loading;
}

// ─────────────────────────────────────────────
// REGISTER MODAL
// ─────────────────────────────────────────────
function showRegisterModal() {
  const modal = document.getElementById('registerModal');
  if (modal) modal.style.display = 'flex';
}

function hideRegisterModal() {
  const modal = document.getElementById('registerModal');
  if (modal) modal.style.display = 'none';
  hideAlert('regAlertBox');
}

async function registerStudent() {
  const id   = document.getElementById('regId')?.value?.trim();
  const name = document.getElementById('regName')?.value?.trim();

  if (!id || !name) {
    showAlert('regAlertBox', 'error', 'Please fill in both Student ID and Name.');
    return;
  }

  setLoading('regBtnText', 'regSpinner', true);

  try {
    const res = await fetch(`${API_BASE}/addStudent`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ id: parseInt(id), name })
    });
    const data = await res.json();

    if (res.ok) {
      showAlert('regAlertBox', 'success', `Student "${name}" registered with ID ${id}!`);
      setTimeout(() => hideRegisterModal(), 2000);
    } else {
      showAlert('regAlertBox', 'error', data.error || 'Registration failed.');
    }
  } catch (err) {
    showAlert('regAlertBox', 'error', 'Cannot connect to server. Make sure the C++ backend is running on port 8080.');
  }

  setLoading('regBtnText', 'regSpinner', false, 'Register');
}

// ─────────────────────────────────────────────
// MARK ATTENDANCE — Main flow
// 1. Get GPS  2. Call API  3. Show result
// ─────────────────────────────────────────────
function markAttendance() {
  const studentId = document.getElementById('studentId')?.value?.trim();

  if (!studentId) {
    showAlert('alertBox', 'error', 'Please enter your Student ID.');
    return;
  }

  // Update location info
  const locText = document.getElementById('locationText');
  if (locText) locText.textContent = 'Fetching GPS location...';

  setLoading('markBtnText', 'markSpinner', true);
  hideAlert('alertBox');

  if (!navigator.geolocation) {
    showAlert('alertBox', 'error', 'Geolocation is not supported by your browser.');
    setLoading('markBtnText', 'markSpinner', false, '🎯 Mark Attendance');
    return;
  }

  navigator.geolocation.getCurrentPosition(
    (position) => sendAttendance(studentId, position.coords.latitude, position.coords.longitude),
    (error) => {
      let msg = 'GPS error. Please allow location access.';
      if (error.code === error.PERMISSION_DENIED) msg = 'Location access denied. Please allow GPS permission.';
      if (error.code === error.TIMEOUT)           msg = 'GPS timed out. Try again.';
      showAlert('alertBox', 'error', msg);
      setLoading('markBtnText', 'markSpinner', false, '🎯 Mark Attendance');
    },
    { timeout: 10000, enableHighAccuracy: true }
  );
}

async function sendAttendance(studentId, lat, lng) {
  // Update GPS display
  const locText = document.getElementById('locationText');
  if (locText) locText.textContent = `📍 ${lat.toFixed(5)}, ${lng.toFixed(5)}`;

  try {
    const res = await fetch(`${API_BASE}/markAttendance`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ studentId: parseInt(studentId), lat, lng })
    });

    const data = await res.json();

    if (res.ok && data.success) {
      let msg = `✅ ${data.message} — Attendance: ${data.attendancePercentage?.toFixed(1)}%`;
      showAlert('alertBox', 'success', msg);
      if (data.warning) {
        setTimeout(() => showAlert('alertBox', 'warning', `⚠️ ${data.warning}`), 2000);
      }
    } else {
      showAlert('alertBox', 'error', data.message || data.error || 'Could not mark attendance.');
    }
  } catch (err) {
    showAlert('alertBox', 'error', 'Cannot reach the backend server. Make sure GeoAttend C++ server is running on port 8080.');
  }

  setLoading('markBtnText', 'markSpinner', false, '🎯 Mark Attendance');
}

// ─────────────────────────────────────────────
// DASHBOARD: Fetch and render data
// ─────────────────────────────────────────────
let attendanceChart = null;
let weeklyChart     = null;
let map             = null;
let fenceCircle     = null;
let studentMarker   = null;

async function loadDashboard() {
  const studentId = document.getElementById('dashStudentId')?.value?.trim();
  if (!studentId) return;

  try {
    const res  = await fetch(`${API_BASE}/dashboard/${studentId}`);
    const data = await res.json();

    if (!res.ok || data.error) {
      alert(data.error || 'Student not found');
      return;
    }

    // Populate stats
    updateStat('statTotal',    data.totalClasses);
    updateStat('statAttended', data.attendedClasses);
    updateStat('statPercent',  data.attendancePercentage.toFixed(1) + '%');
    updateStat('statMargin',   data.margin);

    // Risk badge
    const riskEl = document.getElementById('riskBadge');
    if (riskEl) {
      const map = { Safe: 'safe', Warning: 'warning', Danger: 'danger', Critical: 'critical' };
      riskEl.className = `badge badge-${map[data.riskLevel] || 'safe'}`;
      riskEl.textContent = data.riskLevel;
    }

    // Progress bar
    const pb = document.getElementById('progressBar');
    if (pb) pb.style.width = Math.min(data.attendancePercentage, 100) + '%';

    // Update donut chart
    updateAttendanceChart(data.attendedClasses, data.totalClasses - data.attendedClasses);

    // Student name
    const nameEl = document.getElementById('studentName');
    if (nameEl) nameEl.textContent = data.name;

    // Show warning if needed
    const warnEl = document.getElementById('dashWarning');
    if (warnEl) {
      if (data.attendancePercentage < 80) {
        warnEl.style.display = 'flex';
        warnEl.querySelector('span')?.setAttribute('data', `Warning: ${data.name}'s attendance is ${data.attendancePercentage.toFixed(1)}% — below the required 80%.`);
        warnEl.querySelector('span').textContent = `⚠️ Warning: ${data.name}'s attendance is ${data.attendancePercentage.toFixed(1)}% — below the required 80%.`;
      } else {
        warnEl.style.display = 'none';
      }
    }

  } catch (err) {
    console.error('Dashboard fetch error:', err);
  }
}

function updateStat(id, value) {
  const el = document.getElementById(id);
  if (el) {
    el.textContent = value;
  }
}

function updateAttendanceChart(attended, absent) {
  const ctx = document.getElementById('attendanceChart');
  if (!ctx) return;

  if (attendanceChart) attendanceChart.destroy();

  attendanceChart = new Chart(ctx, {
    type: 'doughnut',
    data: {
      labels: ['Attended', 'Absent'],
      datasets: [{
        data: [attended, absent],
        backgroundColor: ['rgba(99,102,241,0.85)', 'rgba(255,255,255,0.06)'],
        borderColor:     ['#6366f1', 'rgba(255,255,255,0.1)'],
        borderWidth: 2,
        hoverOffset: 8
      }]
    },
    options: {
      responsive: true,
      cutout: '72%',
      plugins: {
        legend: {
          labels: { color: '#94a3b8', font: { family: 'Inter', size: 12 } }
        }
      }
    }
  });
}

function initWeeklyChart() {
  const ctx = document.getElementById('weeklyChart');
  if (!ctx) return;

  // Demo weekly data (replace with API in production)
  const labels = ['Mon','Tue','Wed','Thu','Fri','Sat'];
  const data   = [1, 1, 0, 1, 1, 0];

  weeklyChart = new Chart(ctx, {
    type: 'bar',
    data: {
      labels,
      datasets: [{
        label: 'Present',
        data,
        backgroundColor: data.map(v => v ? 'rgba(99,102,241,0.7)' : 'rgba(239,68,68,0.5)'),
        borderRadius: 6,
        borderSkipped: false
      }]
    },
    options: {
      responsive: true,
      plugins: { legend: { display: false } },
      scales: {
        x: { ticks: { color: '#94a3b8' }, grid: { color: 'rgba(255,255,255,0.04)' } },
        y: {
          ticks: { color: '#94a3b8', stepSize: 1 },
          grid: { color: 'rgba(255,255,255,0.04)' },
          max: 1
        }
      }
    }
  });
}

// ─────────────────────────────────────────────
// GOOGLE MAPS — Initialize and update geofence
// ─────────────────────────────────────────────
async function initMap() {
  const mapEl = document.getElementById('map');
  if (!mapEl || typeof google === 'undefined') return;

  // Fetch geofence config from backend
  let center = { lat: 28.5450, lng: 77.1926 };
  let radius = 200;

  try {
    const res  = await fetch(`${API_BASE}/geofence`);
    const data = await res.json();
    center = { lat: data.centerLat, lng: data.centerLng };
    radius = data.radius;
  } catch (e) {
    console.warn('Could not fetch geofence from backend, using defaults');
  }

  map = new google.maps.Map(mapEl, {
    center,
    zoom: 16,
    styles: [
      { elementType: 'geometry',   stylers: [{ color: '#0f0f1e' }] },
      { elementType: 'labels.text.stroke', stylers: [{ color: '#0f0f1e' }] },
      { elementType: 'labels.text.fill',   stylers: [{ color: '#94a3b8' }] },
      { featureType: 'road', elementType: 'geometry', stylers: [{ color: '#1e2035' }] },
      { featureType: 'water', elementType: 'geometry', stylers: [{ color: '#06b6d4', lightness: -50 }] }
    ]
  });

  // Draw geofence circle
  fenceCircle = new google.maps.Circle({
    map,
    center,
    radius,
    strokeColor: '#6366f1',
    strokeOpacity: 0.9,
    strokeWeight: 2,
    fillColor: '#6366f1',
    fillOpacity: 0.08
  });

  // Center marker (campus)
  new google.maps.Marker({
    position: center,
    map,
    title: 'Geofence Center (Campus)',
    icon: {
      path: google.maps.SymbolPath.CIRCLE,
      scale: 8,
      fillColor: '#6366f1',
      fillOpacity: 1,
      strokeColor: '#a5b4fc',
      strokeWeight: 2
    }
  });

  // Try to show user's current location
  if (navigator.geolocation) {
    navigator.geolocation.getCurrentPosition((pos) => {
      const userPos = { lat: pos.coords.latitude, lng: pos.coords.longitude };
      if (studentMarker) studentMarker.setMap(null);
      studentMarker = new google.maps.Marker({
        position: userPos,
        map,
        title: 'Your Location',
        icon: {
          path: google.maps.SymbolPath.CIRCLE,
          scale: 10,
          fillColor: '#06b6d4',
          fillOpacity: 1,
          strokeColor: '#67e8f9',
          strokeWeight: 2
        }
      });
    });
  }
}

// ─────────────────────────────────────────────
// INIT on page load
// ─────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', () => {
  // Dashboard page init
  if (document.getElementById('attendanceChart')) {
    initWeeklyChart();
    // Auto-load default student (ID 1)
    const idInput = document.getElementById('dashStudentId');
    if (idInput && !idInput.value) idInput.value = 1;
    loadDashboard();
  }
});
