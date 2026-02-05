-- PL/pgSQL Functions for Kanba Clone

-- ============================================
-- USER FUNCTIONS
-- ============================================

-- Create a new user
CREATE OR REPLACE FUNCTION create_user(
    p_email VARCHAR(255),
    p_password_hash VARCHAR(255),
    p_name VARCHAR(255)
)
RETURNS TABLE(
    id UUID,
    email VARCHAR(255),
    name VARCHAR(255),
    avatar_url VARCHAR(500),
    created_at TIMESTAMP WITH TIME ZONE
) AS $$
DECLARE
    v_user_id UUID;
BEGIN
    INSERT INTO users (email, password_hash, name)
    VALUES (p_email, p_password_hash, p_name)
    RETURNING users.id INTO v_user_id;
    
    RETURN QUERY
    SELECT u.id, u.email, u.name, u.avatar_url, u.created_at
    FROM users u WHERE u.id = v_user_id;
END;
$$ LANGUAGE plpgsql;

-- Get user by email (for authentication)
CREATE OR REPLACE FUNCTION get_user_by_email(p_email VARCHAR(255))
RETURNS TABLE(
    id UUID,
    email VARCHAR(255),
    password_hash VARCHAR(255),
    name VARCHAR(255),
    avatar_url VARCHAR(500),
    created_at TIMESTAMP WITH TIME ZONE
) AS $$
BEGIN
    RETURN QUERY
    SELECT u.id, u.email, u.password_hash, u.name, u.avatar_url, u.created_at
    FROM users u WHERE u.email = p_email;
END;
$$ LANGUAGE plpgsql;

-- Get user by ID
CREATE OR REPLACE FUNCTION get_user_by_id(p_user_id UUID)
RETURNS TABLE(
    id UUID,
    email VARCHAR(255),
    name VARCHAR(255),
    avatar_url VARCHAR(500),
    created_at TIMESTAMP WITH TIME ZONE
) AS $$
BEGIN
    RETURN QUERY
    SELECT u.id, u.email, u.name, u.avatar_url, u.created_at
    FROM users u WHERE u.id = p_user_id;
END;
$$ LANGUAGE plpgsql;

-- ============================================
-- PROJECT FUNCTIONS
-- ============================================

-- Create a new project with default columns
CREATE OR REPLACE FUNCTION create_project(
    p_name VARCHAR(255),
    p_description TEXT,
    p_icon VARCHAR(50),
    p_owner_id UUID
)
RETURNS UUID AS $$
DECLARE
    v_project_id UUID;
BEGIN
    -- Create project
    INSERT INTO projects (name, description, icon, owner_id)
    VALUES (p_name, p_description, COALESCE(p_icon, '📋'), p_owner_id)
    RETURNING id INTO v_project_id;
    
    -- Add owner as project member
    INSERT INTO project_members (project_id, user_id, role)
    VALUES (v_project_id, p_owner_id, 'owner');
    
    -- Create default columns
    INSERT INTO columns (project_id, name, position, color) VALUES
        (v_project_id, 'To Do', 0, '#6366f1'),
        (v_project_id, 'Done', 1, '#22c55e');
    
    -- Log activity
    INSERT INTO activity_log (project_id, user_id, action, entity_type, entity_id, details)
    VALUES (v_project_id, p_owner_id, 'created', 'project', v_project_id, 
            jsonb_build_object('name', p_name));
    
    RETURN v_project_id;
END;
$$ LANGUAGE plpgsql;

-- Get all projects for a user
CREATE OR REPLACE FUNCTION get_user_projects(p_user_id UUID)
RETURNS TABLE(
    id UUID,
    name VARCHAR(255),
    description TEXT,
    icon VARCHAR(50),
    owner_id UUID,
    created_at TIMESTAMP WITH TIME ZONE,
    task_count BIGINT,
    member_count BIGINT
) AS $$
BEGIN
    RETURN QUERY
    SELECT 
        p.id,
        p.name,
        p.description,
        p.icon,
        p.owner_id,
        p.created_at,
        (SELECT COUNT(*) FROM tasks t 
         JOIN columns c ON t.column_id = c.id 
         WHERE c.project_id = p.id) as task_count,
        (SELECT COUNT(*) FROM project_members pm WHERE pm.project_id = p.id) as member_count
    FROM projects p
    JOIN project_members pm ON p.id = pm.project_id
    WHERE pm.user_id = p_user_id
    ORDER BY p.created_at DESC;
END;
$$ LANGUAGE plpgsql;

-- Get project by ID with full details
CREATE OR REPLACE FUNCTION get_project_details(p_project_id UUID)
RETURNS TABLE(
    id UUID,
    name VARCHAR(255),
    description TEXT,
    icon VARCHAR(50),
    owner_id UUID,
    created_at TIMESTAMP WITH TIME ZONE
) AS $$
BEGIN
    RETURN QUERY
    SELECT p.id, p.name, p.description, p.icon, p.owner_id, p.created_at
    FROM projects p WHERE p.id = p_project_id;
END;
$$ LANGUAGE plpgsql;

-- Delete project
CREATE OR REPLACE FUNCTION delete_project(p_project_id UUID, p_user_id UUID)
RETURNS BOOLEAN AS $$
DECLARE
    v_owner_id UUID;
BEGIN
    SELECT owner_id INTO v_owner_id FROM projects WHERE id = p_project_id;
    
    IF v_owner_id != p_user_id THEN
        RAISE EXCEPTION 'Only the project owner can delete the project';
    END IF;
    
    DELETE FROM projects WHERE id = p_project_id;
    RETURN TRUE;
END;
$$ LANGUAGE plpgsql;

-- ============================================
-- COLUMN FUNCTIONS
-- ============================================

-- Get columns for a project
CREATE OR REPLACE FUNCTION get_project_columns(p_project_id UUID)
RETURNS TABLE(
    id UUID,
    project_id UUID,
    name VARCHAR(255),
    col_position INTEGER,
    color VARCHAR(50),
    task_count BIGINT
) AS $$
BEGIN
    RETURN QUERY
    SELECT 
        c.id,
        c.project_id,
        c.name,
        c.position as col_position,
        c.color,
        (SELECT COUNT(*) FROM tasks t WHERE t.column_id = c.id) as task_count
    FROM columns c
    WHERE c.project_id = p_project_id
    ORDER BY c.position ASC;
END;
$$ LANGUAGE plpgsql;

-- Create a new column
CREATE OR REPLACE FUNCTION create_column(
    p_project_id UUID,
    p_name VARCHAR(255),
    p_color VARCHAR(50)
)
RETURNS UUID AS $$
DECLARE
    v_column_id UUID;
    v_max_position INTEGER;
BEGIN
    SELECT COALESCE(MAX(position), -1) + 1 INTO v_max_position
    FROM columns WHERE project_id = p_project_id;
    
    INSERT INTO columns (project_id, name, position, color)
    VALUES (p_project_id, p_name, v_max_position, COALESCE(p_color, '#6366f1'))
    RETURNING id INTO v_column_id;
    
    RETURN v_column_id;
END;
$$ LANGUAGE plpgsql;

-- Update column
CREATE OR REPLACE FUNCTION update_column(
    p_column_id UUID,
    p_name VARCHAR(255),
    p_color VARCHAR(50)
)
RETURNS BOOLEAN AS $$
BEGIN
    UPDATE columns 
    SET name = COALESCE(p_name, name),
        color = COALESCE(p_color, color)
    WHERE id = p_column_id;
    
    RETURN FOUND;
END;
$$ LANGUAGE plpgsql;

-- Delete column and reassign tasks to first column
CREATE OR REPLACE FUNCTION delete_column(p_column_id UUID)
RETURNS BOOLEAN AS $$
DECLARE
    v_project_id UUID;
    v_first_column_id UUID;
BEGIN
    SELECT project_id INTO v_project_id FROM columns WHERE id = p_column_id;
    
    -- Find the first column that isn't this one
    SELECT id INTO v_first_column_id 
    FROM columns 
    WHERE project_id = v_project_id AND id != p_column_id
    ORDER BY position ASC
    LIMIT 1;
    
    -- Move tasks to first column if one exists
    IF v_first_column_id IS NOT NULL THEN
        UPDATE tasks SET column_id = v_first_column_id WHERE column_id = p_column_id;
    END IF;
    
    DELETE FROM columns WHERE id = p_column_id;
    
    -- Reorder remaining columns
    WITH ordered AS (
        SELECT id, ROW_NUMBER() OVER (ORDER BY position) - 1 as new_pos
        FROM columns WHERE project_id = v_project_id
    )
    UPDATE columns c SET position = o.new_pos
    FROM ordered o WHERE c.id = o.id;
    
    RETURN TRUE;
END;
$$ LANGUAGE plpgsql;

-- ============================================
-- TASK FUNCTIONS
-- ============================================

-- Get all tasks for a project (grouped by column)
CREATE OR REPLACE FUNCTION get_project_tasks(p_project_id UUID)
RETURNS TABLE(
    id UUID,
    column_id UUID,
    title VARCHAR(500),
    description TEXT,
    priority VARCHAR(20),
    task_position INTEGER,
    assignee_id UUID,
    assignee_name VARCHAR(255),
    assignee_avatar VARCHAR(500),
    due_date TIMESTAMP WITH TIME ZONE,
    tags TEXT[],
    created_by UUID,
    created_at TIMESTAMP WITH TIME ZONE
) AS $$
BEGIN
    RETURN QUERY
    SELECT 
        t.id,
        t.column_id,
        t.title,
        t.description,
        t.priority,
        t.position as task_position,
        t.assignee_id,
        u.name as assignee_name,
        u.avatar_url as assignee_avatar,
        t.due_date,
        t.tags,
        t.created_by,
        t.created_at
    FROM tasks t
    JOIN columns c ON t.column_id = c.id
    LEFT JOIN users u ON t.assignee_id = u.id
    WHERE c.project_id = p_project_id
    ORDER BY c.position ASC, t.position ASC;
END;
$$ LANGUAGE plpgsql;

-- Create a new task
CREATE OR REPLACE FUNCTION create_task(
    p_column_id UUID,
    p_title VARCHAR(500),
    p_description TEXT,
    p_priority VARCHAR(20),
    p_assignee_id UUID,
    p_due_date TIMESTAMP WITH TIME ZONE,
    p_tags TEXT[],
    p_created_by UUID
)
RETURNS UUID AS $$
DECLARE
    v_task_id UUID;
    v_max_position INTEGER;
    v_project_id UUID;
BEGIN
    -- Get max position
    SELECT COALESCE(MAX(position), -1) + 1 INTO v_max_position
    FROM tasks WHERE column_id = p_column_id;
    
    -- Get project ID for activity log
    SELECT project_id INTO v_project_id FROM columns WHERE id = p_column_id;
    
    INSERT INTO tasks (column_id, title, description, priority, position, assignee_id, due_date, tags, created_by)
    VALUES (p_column_id, p_title, p_description, COALESCE(p_priority, 'medium'), v_max_position, 
            p_assignee_id, p_due_date, p_tags, p_created_by)
    RETURNING id INTO v_task_id;
    
    -- Log activity
    INSERT INTO activity_log (project_id, user_id, action, entity_type, entity_id, details)
    VALUES (v_project_id, p_created_by, 'created', 'task', v_task_id, 
            jsonb_build_object('title', p_title));
    
    RETURN v_task_id;
END;
$$ LANGUAGE plpgsql;

-- Update task
CREATE OR REPLACE FUNCTION update_task(
    p_task_id UUID,
    p_title VARCHAR(500),
    p_description TEXT,
    p_priority VARCHAR(20),
    p_assignee_id UUID,
    p_due_date TIMESTAMP WITH TIME ZONE,
    p_tags TEXT[],
    p_user_id UUID
)
RETURNS BOOLEAN AS $$
DECLARE
    v_project_id UUID;
BEGIN
    -- Get project ID
    SELECT c.project_id INTO v_project_id 
    FROM tasks t JOIN columns c ON t.column_id = c.id 
    WHERE t.id = p_task_id;
    
    UPDATE tasks 
    SET title = COALESCE(p_title, title),
        description = COALESCE(p_description, description),
        priority = COALESCE(p_priority, priority),
        assignee_id = p_assignee_id,
        due_date = p_due_date,
        tags = COALESCE(p_tags, tags)
    WHERE id = p_task_id;
    
    -- Log activity
    INSERT INTO activity_log (project_id, user_id, action, entity_type, entity_id, details)
    VALUES (v_project_id, p_user_id, 'updated', 'task', p_task_id, 
            jsonb_build_object('title', p_title));
    
    RETURN FOUND;
END;
$$ LANGUAGE plpgsql;

-- Move task to different column
CREATE OR REPLACE FUNCTION move_task(
    p_task_id UUID,
    p_new_column_id UUID,
    p_new_position INTEGER,
    p_user_id UUID
)
RETURNS BOOLEAN AS $$
DECLARE
    v_old_column_id UUID;
    v_project_id UUID;
    v_column_name VARCHAR(255);
BEGIN
    SELECT column_id INTO v_old_column_id FROM tasks WHERE id = p_task_id;
    SELECT project_id INTO v_project_id FROM columns WHERE id = p_new_column_id;
    SELECT name INTO v_column_name FROM columns WHERE id = p_new_column_id;
    
    -- Shift tasks in new column to make room
    UPDATE tasks 
    SET position = position + 1 
    WHERE column_id = p_new_column_id AND position >= p_new_position;
    
    -- Move the task
    UPDATE tasks 
    SET column_id = p_new_column_id, position = p_new_position
    WHERE id = p_task_id;
    
    -- Reorder old column
    WITH ordered AS (
        SELECT id, ROW_NUMBER() OVER (ORDER BY position) - 1 as new_pos
        FROM tasks WHERE column_id = v_old_column_id
    )
    UPDATE tasks t SET position = o.new_pos
    FROM ordered o WHERE t.id = o.id;
    
    -- Log activity
    INSERT INTO activity_log (project_id, user_id, action, entity_type, entity_id, details)
    VALUES (v_project_id, p_user_id, 'moved', 'task', p_task_id, 
            jsonb_build_object('column', v_column_name));
    
    RETURN TRUE;
END;
$$ LANGUAGE plpgsql;

-- Delete task
CREATE OR REPLACE FUNCTION delete_task(p_task_id UUID, p_user_id UUID)
RETURNS BOOLEAN AS $$
DECLARE
    v_column_id UUID;
    v_project_id UUID;
    v_title VARCHAR(500);
BEGIN
    SELECT column_id, title INTO v_column_id, v_title FROM tasks WHERE id = p_task_id;
    SELECT project_id INTO v_project_id FROM columns WHERE id = v_column_id;
    
    DELETE FROM tasks WHERE id = p_task_id;
    
    -- Reorder remaining tasks
    WITH ordered AS (
        SELECT id, ROW_NUMBER() OVER (ORDER BY position) - 1 as new_pos
        FROM tasks WHERE column_id = v_column_id
    )
    UPDATE tasks t SET position = o.new_pos
    FROM ordered o WHERE t.id = o.id;
    
    -- Log activity
    INSERT INTO activity_log (project_id, user_id, action, entity_type, entity_id, details)
    VALUES (v_project_id, p_user_id, 'deleted', 'task', p_task_id, 
            jsonb_build_object('title', v_title));
    
    RETURN TRUE;
END;
$$ LANGUAGE plpgsql;

-- ============================================
-- PROJECT MEMBER FUNCTIONS
-- ============================================

-- Get project members
CREATE OR REPLACE FUNCTION get_project_members(p_project_id UUID)
RETURNS TABLE(
    id UUID,
    user_id UUID,
    name VARCHAR(255),
    email VARCHAR(255),
    avatar_url VARCHAR(500),
    role VARCHAR(50),
    joined_at TIMESTAMP WITH TIME ZONE
) AS $$
BEGIN
    RETURN QUERY
    SELECT 
        pm.id,
        pm.user_id,
        u.name,
        u.email,
        u.avatar_url,
        pm.role,
        pm.joined_at
    FROM project_members pm
    JOIN users u ON pm.user_id = u.id
    WHERE pm.project_id = p_project_id
    ORDER BY pm.joined_at ASC;
END;
$$ LANGUAGE plpgsql;

-- Add project member
CREATE OR REPLACE FUNCTION add_project_member(
    p_project_id UUID,
    p_user_email VARCHAR(255),
    p_role VARCHAR(50)
)
RETURNS BOOLEAN AS $$
DECLARE
    v_user_id UUID;
BEGIN
    SELECT id INTO v_user_id FROM users WHERE email = p_user_email;
    
    IF v_user_id IS NULL THEN
        RAISE EXCEPTION 'User not found';
    END IF;
    
    INSERT INTO project_members (project_id, user_id, role)
    VALUES (p_project_id, v_user_id, COALESCE(p_role, 'member'))
    ON CONFLICT (project_id, user_id) DO UPDATE SET role = EXCLUDED.role;
    
    RETURN TRUE;
END;
$$ LANGUAGE plpgsql;

