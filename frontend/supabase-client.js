// ============================================================
// supabase-client.js — v2 (class-wise attendance)
// ============================================================

import { createClient } from 'https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2/+esm';

const SUPABASE_URL = 'https://gvjjlkoydrtptzaexdhp.supabase.co';
const SUPABASE_ANON = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imd2ampsa295ZHJ0cHR6YWV4ZGhwIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzMxMjE2MjMsImV4cCI6MjA4ODY5NzYyM30.URNV0PX4l2sdnEACvmHwbcZH14YnG59MJq-cj4NuTKQ';

export const supabase = createClient(SUPABASE_URL, SUPABASE_ANON);

// ─────────────────────────────────────────────
// AUTH HELPERS
// ─────────────────────────────────────────────

// Sign up with role + student details
export async function signUp(email, password, fullName, role, extraInfo = {}) {
    const { data, error } = await supabase.auth.signUp({
        email, password,
        options: {
            data: {
                full_name: fullName,
                role,
                roll_number: extraInfo.rollNumber || null,
                class_name: extraInfo.className || null,
                section: extraInfo.section || null,
                branch: extraInfo.branch || null
            }
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

// Get current user profile (with role + class info)
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

// Auth guard
export async function requireAuth(redirectTo = 'login.html') {
    const session = await getSession();
    if (!session) { window.location.href = redirectTo; return null; }
    return session;
}

// ─────────────────────────────────────────────
// ATTENDANCE SESSION HELPERS (Teacher)
// ─────────────────────────────────────────────

// Start a class-targeted session
export async function startSession(teacherId, teacherName, subjectName, targetClass, targetSection, targetBranch, lat, lng, radiusM = 100) {
    // End any existing active sessions
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
            target_class: targetClass || null,
            target_section: targetSection || null,
            target_branch: targetBranch || null,
            lat, lng,
            radius: radiusM,
            active: true
        })
        .select()
        .single();

    if (error) throw error;
    return data;
}

// End active session
export async function endSession(teacherId) {
    const { error } = await supabase
        .from('attendance_sessions')
        .update({ active: false, ended_at: new Date().toISOString() })
        .eq('teacher_id', teacherId)
        .eq('active', true);
    if (error) throw error;
}

// Get active session (for students to detect)
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

// Get all sessions by a teacher (for teacher dashboard)
export async function getTeacherSessions(teacherId) {
    const { data, error } = await supabase
        .from('attendance_sessions')
        .select('*')
        .eq('teacher_id', teacherId)
        .order('started_at', { ascending: false });
    if (error) throw error;
    return data || [];
}

// Real-time subscriptions
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

// Get absentees for a session
// Returns all students in the session's target class/section/branch who did NOT mark attendance
export async function getAbsentees(session) {
    if (!session || !session.id) return [];

    // 1. Get all students matching the class filter
    let query = supabase.from('profiles').select('*').eq('role', 'student');
    if (session.target_class) query = query.eq('class_name', session.target_class);
    if (session.target_section) query = query.eq('section', session.target_section);
    if (session.target_branch) query = query.eq('branch', session.target_branch);

    const { data: students, error: sErr } = await query;
    if (sErr) throw sErr;

    // 2. Get IDs of students who DID mark attendance
    const { data: records, error: rErr } = await supabase
        .from('attendance_records')
        .select('student_id')
        .eq('session_id', session.id);
    if (rErr) throw rErr;

    const presentIds = new Set((records || []).map(r => r.student_id));

    // 3. Return students NOT in present set
    return (students || []).filter(s => !presentIds.has(s.id));
}

// Subscribe to new records (real-time)
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

// Mark attendance — validates class match + geofence
export async function markAttendance(session, student, studentLat, studentLng) {
    // Class/section/branch match check
    if (session.target_class && session.target_class !== student.class_name) {
        throw new Error(`This session is for ${session.target_class}. You are in ${student.class_name || 'unknown class'}.`);
    }
    if (session.target_section && session.target_section !== student.section) {
        throw new Error(`This session is for Section ${session.target_section}. You are in Section ${student.section || '?'}.`);
    }
    if (session.target_branch && session.target_branch !== student.branch) {
        throw new Error(`This session is for ${session.target_branch}. You are in ${student.branch || 'unknown branch'}.`);
    }

    // Geofence check
    const dist = haversineDistance(session.lat, session.lng, studentLat, studentLng);
    if (dist > (session.radius || 100)) {
        throw new Error(`You are ${Math.round(dist)}m away. You must be within ${session.radius}m of the teacher.`);
    }

    const { data, error } = await supabase
        .from('attendance_records')
        .insert({
            session_id: session.id,
            student_id: student.id,
            student_name: student.full_name,
            roll_number: student.roll_number,
            class_name: student.class_name,
            section: student.section,
            branch: student.branch,
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

// Check if student already marked
export async function alreadyMarked(sessionId, studentId) {
    const { data } = await supabase
        .from('attendance_records')
        .select('id')
        .eq('session_id', sessionId)
        .eq('student_id', studentId)
        .maybeSingle();
    return !!data;
}

// Get full attendance history for a student
export async function getStudentHistory(studentId) {
    const { data, error } = await supabase
        .from('attendance_records')
        .select(`
            id, marked_at, distance_m,
            attendance_sessions (
                subject_name, teacher_name, started_at,
                target_class, target_section, target_branch
            )
        `)
        .eq('student_id', studentId)
        .order('marked_at', { ascending: false });

    if (error) throw error;
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
    const R = 6371000;
    const dLat = (lat2 - lat1) * Math.PI / 180;
    const dLng = (lng2 - lng1) * Math.PI / 180;
    const a = Math.sin(dLat / 2) ** 2
        + Math.cos(lat1 * Math.PI / 180)
        * Math.cos(lat2 * Math.PI / 180)
        * Math.sin(dLng / 2) ** 2;
    return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
}
