import { query } from '@/lib/db';

export async function truncateAll() {
  await query(
    'TRUNCATE users, projects, project_members, columns, tasks, task_comments, activity_log CASCADE'
  );
}
