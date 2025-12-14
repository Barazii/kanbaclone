import { NextResponse } from 'next/server';
import { cookies } from 'next/headers';
import { query } from '@/lib/db';
import { getSession, hasSession } from '@/lib/session';

// Helper to get user ID from session
async function getUserId(): Promise<string | null> {
  const cookieStore = await cookies();
  const sessionId = cookieStore.get('session')?.value;
  if (!sessionId || !hasSession(sessionId)) return null;
  return getSession(sessionId) || null;
}

// GET project details with columns and tasks
export async function GET(
  request: Request,
  { params }: { params: Promise<{ id: string }> }
) {
  try {
    const userId = await getUserId();
    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { id: projectId } = await params;

    // Get project details
    const projectResult = await query('SELECT * FROM get_project_details($1)', [projectId]);
    if (projectResult.rows.length === 0) {
      return NextResponse.json({ error: 'Project not found' }, { status: 404 });
    }

    // Get columns
    const columnsResult = await query('SELECT * FROM get_project_columns($1)', [projectId]);
    
    // Get tasks
    const tasksResult = await query('SELECT * FROM get_project_tasks($1)', [projectId]);
    
    // Get members
    const membersResult = await query('SELECT * FROM get_project_members($1)', [projectId]);

    // Group tasks by column - map task_position back to position for frontend
    const columns = columnsResult.rows.map(col => ({
      ...col,
      position: col.col_position,
      tasks: tasksResult.rows
        .filter(task => task.column_id === col.id)
        .map(task => ({ ...task, position: task.task_position }))
    }));

    return NextResponse.json({
      project: projectResult.rows[0],
      columns,
      members: membersResult.rows,
    });
  } catch (error) {
    console.error('Get project error:', error);
    return NextResponse.json({ error: 'Failed to get project' }, { status: 500 });
  }
}

// DELETE project
export async function DELETE(
  request: Request,
  { params }: { params: Promise<{ id: string }> }
) {
  try {
    const userId = await getUserId();
    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { id: projectId } = await params;

    await query('SELECT delete_project($1, $2)', [projectId, userId]);

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Delete project error:', error);
    return NextResponse.json({ error: 'Failed to delete project' }, { status: 500 });
  }
}
