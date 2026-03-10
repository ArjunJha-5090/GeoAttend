-- ============================================================
-- GeoAttend Supabase Schema
-- Run this in: Supabase Dashboard → SQL Editor → New Query
-- ============================================================

-- 1. Profiles table — extends auth.users with role
CREATE TABLE IF NOT EXISTS public.profiles (
    id          UUID PRIMARY KEY REFERENCES auth.users(id) ON DELETE CASCADE,
    full_name   TEXT,
    role        TEXT NOT NULL DEFAULT 'student' CHECK (role IN ('teacher', 'student')),
    created_at  TIMESTAMPTZ DEFAULT now()
);

-- Auto-create a profile row when a user signs up
CREATE OR REPLACE FUNCTION public.handle_new_user()
RETURNS TRIGGER AS $$
BEGIN
    INSERT INTO public.profiles (id, full_name, role)
    VALUES (
        NEW.id,
        NEW.raw_user_meta_data ->> 'full_name',
        COALESCE(NEW.raw_user_meta_data ->> 'role', 'student')
    );
    RETURN NEW;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

DROP TRIGGER IF EXISTS on_auth_user_created ON auth.users;
CREATE TRIGGER on_auth_user_created
    AFTER INSERT ON auth.users
    FOR EACH ROW EXECUTE FUNCTION public.handle_new_user();

-- 2. Attendance Sessions — created by teacher
CREATE TABLE IF NOT EXISTS public.attendance_sessions (
    id           UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    teacher_id   UUID REFERENCES auth.users(id) ON DELETE CASCADE,
    teacher_name TEXT,
    subject_name TEXT NOT NULL DEFAULT 'General',
    lat          DOUBLE PRECISION NOT NULL,
    lng          DOUBLE PRECISION NOT NULL,
    radius       INTEGER DEFAULT 100,
    active       BOOLEAN DEFAULT TRUE,
    started_at   TIMESTAMPTZ DEFAULT now(),
    ended_at     TIMESTAMPTZ
);

-- Add subject_name to existing tables (safe to re-run)
ALTER TABLE public.attendance_sessions ADD COLUMN IF NOT EXISTS subject_name TEXT NOT NULL DEFAULT 'General';

-- 3. Attendance Records — created by students
CREATE TABLE IF NOT EXISTS public.attendance_records (
    id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    session_id  UUID REFERENCES public.attendance_sessions(id) ON DELETE CASCADE,
    student_id  UUID REFERENCES auth.users(id) ON DELETE CASCADE,
    student_name TEXT,
    student_lat DOUBLE PRECISION,
    student_lng DOUBLE PRECISION,
    distance_m  DOUBLE PRECISION,        -- distance from teacher at time of marking
    marked_at   TIMESTAMPTZ DEFAULT now(),
    UNIQUE(session_id, student_id)       -- prevent duplicate marks per session
);

-- ============================================================
-- Row Level Security (RLS)
-- ============================================================

ALTER TABLE public.profiles            ENABLE ROW LEVEL SECURITY;
ALTER TABLE public.attendance_sessions ENABLE ROW LEVEL SECURITY;
ALTER TABLE public.attendance_records  ENABLE ROW LEVEL SECURITY;

-- Drop existing policies (safe to re-run)
DROP POLICY IF EXISTS "Profiles are viewable by everyone"          ON public.profiles;
DROP POLICY IF EXISTS "Users can update their own profile"         ON public.profiles;
DROP POLICY IF EXISTS "Sessions viewable by all"                   ON public.attendance_sessions;
DROP POLICY IF EXISTS "Teachers can create sessions"               ON public.attendance_sessions;
DROP POLICY IF EXISTS "Teachers can update their sessions"         ON public.attendance_sessions;
DROP POLICY IF EXISTS "Records viewable by all"                    ON public.attendance_records;
DROP POLICY IF EXISTS "Students can mark their own attendance"     ON public.attendance_records;

-- Profiles: everyone can read, only own row can be updated
CREATE POLICY "Profiles are viewable by everyone"   ON public.profiles FOR SELECT USING (true);
CREATE POLICY "Users can update their own profile"  ON public.profiles FOR UPDATE USING (auth.uid() = id);

-- Sessions: anyone logged in can read; only teachers can insert/update
CREATE POLICY "Sessions viewable by all"             ON public.attendance_sessions FOR SELECT USING (true);
CREATE POLICY "Teachers can create sessions"         ON public.attendance_sessions FOR INSERT WITH CHECK (auth.uid() = teacher_id);
CREATE POLICY "Teachers can update their sessions"   ON public.attendance_sessions FOR UPDATE USING (auth.uid() = teacher_id);

-- Records: anyone logged in can read; students insert their own
CREATE POLICY "Records viewable by all"                    ON public.attendance_records FOR SELECT USING (true);
CREATE POLICY "Students can mark their own attendance"     ON public.attendance_records FOR INSERT WITH CHECK (auth.uid() = student_id);

