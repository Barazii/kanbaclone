// User types
export interface User {
  id: string;
  email: string;
  name: string;
  avatar_url?: string;
  created_at: string;
}

// Project types
export interface Project {
  id: string;
  name: string;
  description?: string;
  icon: string;
  owner_id: string;
  created_at: string;
  task_count?: number;
  member_count?: number;
}

// Column types
export interface Column {
  id: string;
  project_id: string;
  name: string;
  position: number;
  color: string;
  task_count?: number;
  tasks?: Task[];
}

// Task types
export interface Task {
  id: string;
  column_id: string;
  title: string;
  description?: string;
  priority: 'low' | 'medium' | 'high';
  position: number;
  assignee_id?: string;
  assignee_name?: string;
  assignee_avatar?: string;
  due_date?: string;
  tags?: string[];
  created_by?: string;
  created_at: string;
}

// Project member types
export interface ProjectMember {
  id: string;
  user_id: string;
  name: string;
  email: string;
  avatar_url?: string;
  role: 'owner' | 'admin' | 'member';
  joined_at: string;
}
