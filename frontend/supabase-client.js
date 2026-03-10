// ============================================================
// supabase-client.js — Supabase initialization & helpers
// ============================================================

import { createClient } from 'https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2/+esm';

const SUPABASE_URL = 'https://gvjjlkoydrtptzaexdhp.supabase.co';
const SUPABASE_ANON = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imd2ampsa295ZHJ0cHR6YWV4ZGhwIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzMxMjE2MjMsImV4cCI6MjA4ODY5NzYyM30.URNV0PX4l2sdnEACvmHwbcZH14YnG59MJq-cj4NuTKQ';

export const supabase = createClient(SUPABASE_URL, SUPABASE_ANON);

// ─────────────────────────────────────────────
// AUTH HELPERS
// ─────────────────────────────────────────────

// Sign up with role (teacher/student)
export async function signUp(email, password, fullName, role) {
    const { data, error } = await supabase.auth.signUp({
        email, password,
        options: {
            data: { full_name: fullName, role }
        }
    });
    if (error) throw error;
    return data;
}

// Sign in
export async function signIn(email, password) {
    const { data, error } = await supabase.auth.signInWithPassword({ email, password });
    if (error) throw error;
    return data;
}

// Sign out
export async function signOut() {
    await supabase.auth.signOut();
}

// Get current session
export async function getSession() {
    const { data } = await supabase.auth.getSession();
    return data.session;
}

// Get current user profile (with role)
export async function getProfile() {
    const session = await getSession();
    if (!session) return null;
    const { data, error } = await supabase
        .from('profiles')
        .select('*')
        .eq('id', session.user.id)
        .single();
    if (error) throw error;
    return data;
}

// Auth guard — redirect to login if not signed in
export async function requireAuth(redirectTo = 'login.html') {
    const session = await getSession();
    if (!session) {
        window.location.href = redirectTo;
        return null;
    }
    return session;
}

// ─────────────────────────────────────────────
// ATTENDANCE SESSION HELPERS (Teacher)
// ─────────────────────────────────────────────

// Start a new attendance session with teacher's GPS and subject name
export async function startSession(teacherId, teacherName, subjectName, lat, lng, radiusM = 100) {
    // End any existing active sessions for this teacher
    await supabase
        .from('attendance_sessions')
        .update({ active: false, ended_at: new Date().toISOString() })
        .eq('teacher_id', teacherId)
        .eq('active', true);

    const { data, error } = await supabase
        .from('attendance_sessions')
        .insert({
            teacher_id: teacherId,
            teacher_name: teacherName,
            subject_name: subjectName || 'General',
            lat, lng,
            radius: radiusM,
            active: true
        })
        .select()
        .single();

    if (error) throw error;
    return data;
}

// End the active session for a teacher
export async function endSession(teacherId) {
    const { error } = await supabase
        .from('attendance_sessions')
        .update({ active: false, ended_at: new Date().toISOString() })
        .eq('teacher_id', teacherId)
        .eq('active', true);
    if (error) throw error;
}

// Get the currently active session (if any)
export async function getActiveSession() {
    const { data, error } = await supabase
        .from('attendance_sessions')
        .select('*')
        .eq('active', true)
        .order('started_at', { ascending: false })
        .limit(1)
        .maybeSingle();
    if (error) throw error;
    return data;
}

// Subscribe to active session changes (real-time)
export function subscribeToSessions(callback) {
    return supabase
        .channel('attendance_sessions')
        .on('postgres_changes',
            { event: '*', schema: 'public', table: 'attendance_sessions' },
            callback)
        .subscribe();
}

// Get attendance records for a session
export async function getSessionRecords(sessionId) {
    const { data, error } = await supabase
        .from('attendance_records')
        .select('*')
        .eq('session_id', sessionId)
        .order('marked_at', { ascending: false });
    if (error) throw error;
    return data || [];
}

// Subscribe to new attendance records (real-time)
export function subscribeToRecords(sessionId, callback) {
    return supabase
        .channel('attendance_records_' + sessionId)
        .on('postgres_changes',
            { event: 'INSERT', schema: 'public', table: 'attendance_records', filter: `session_id=eq.${sessionId}` },
            callback)
        .subscribe();
}

// ─────────────────────────────────────────────
// ATTENDANCE MARKING HELPERS (Student)
// ─────────────────────────────────────────────

// Mark student's attendance for the active session
export async function markAttendance(session, studentId, studentName, studentLat, studentLng) {
    // Calculate distance from teacher's location
    const dist = haversineDistance(session.lat, session.lng, studentLat, studentLng);

    if (dist > session.radius) {
        throw new Error(`You are ${Math.round(dist)}m away. You must be within ${session.radius}m of the teacher.`);
    }

    const { data, error } = await supabase
        .from('attendance_records')
        .insert({
            session_id: session.id,
            student_id: studentId,
            student_name: studentName,
            student_lat: studentLat,
            student_lng: studentLng,
            distance_m: Math.round(dist)
        })
        .select()
        .single();

    if (error) {
        if (error.code === '23505') throw new Error('You have already marked attendance for this session!');
        throw error;
    }
    return { record: data, distance: Math.round(dist) };
}

// Check if student already marked for this session
export async function alreadyMarked(sessionId, studentId) {
    const { data } = await supabase
        .from('attendance_records')
        .select('id')
        .eq('session_id', sessionId)
        .eq('student_id', studentId)
        .maybeSingle();
    return !!data;
}

// Get full attendance history for a student (joined with sessions)
export async function getStudentHistory(studentId) {
    const { data, error } = await supabase
        .from('attendance_records')
        .select(`
            id, marked_at, distance_m,
            attendance_sessions (
                subject_name, teacher_name, started_at, lat, lng
            )
        `)
        .eq('student_id', studentId)
        .order('marked_at', { ascending: false });

    if (error) throw error;

    // Flatten the join
    return (data || []).map(r => ({
        id: r.id,
        marked_at: r.marked_at,
        distance_m: r.distance_m,
        subject_name: r.attendance_sessions?.subject_name || 'General',
        teacher_name: r.attendance_sessions?.teacher_name || 'Unknown',
        session_date: r.attendance_sessions?.started_at
    }));
}

// Get subject-wise summary for a student
export async function getSubjectSummary(studentId) {
    const history = await getStudentHistory(studentId);

    // Group by subject
    const subjects = {};
    history.forEach(r => {
        const sub = r.subject_name;
        if (!subjects[sub]) subjects[sub] = { subject: sub, teacher: r.teacher_name, attended: 0 };
        subjects[sub].attended++;
    });
    return Object.values(subjects);
}


// ─────────────────────────────────────────────
// UTIL: Haversine distance (returns meters)
// ─────────────────────────────────────────────
export function haversineDistance(lat1, lng1, lat2, lng2) {
    const R = 6371000; // Earth radius in meters
    const dLat = (lat2 - lat1) * Math.PI / 180;
    const dLng = (lng2 - lng1) * Math.PI / 180;
    const a = Math.sin(dLat / 2) ** 2
        + Math.cos(lat1 * Math.PI / 180)
        * Math.cos(lat2 * Math.PI / 180)
        * Math.sin(dLng / 2) ** 2;
    return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
}
