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

// POST create new task
export async function POST(request: Request) {
  try {
    const userId = await getUserId();
    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { column_id, title, description, priority, assignee_id, due_date, tags } = await request.json();

    if (!column_id || !title) {
      return NextResponse.json({ error: 'Column ID and title are required' }, { status: 400 });
    }

    const result = await query(
      'SELECT create_task($1, $2, $3, $4, $5, $6, $7, $8) as id',
      [
        column_id,
        title,
        description || null,
        priority || 'medium',
        assignee_id || null,
        due_date || null,
        tags || null,
        userId
      ]
    );

    const taskId = result.rows[0].id;

    return NextResponse.json({ id: taskId, success: true });
  } catch (error) {
    console.error('Create task error:', error);
    return NextResponse.json({ error: 'Failed to create task' }, { status: 500 });
  }
}

// PUT update task
export async function PUT(request: Request) {
  try {
    const userId = await getUserId();
    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { id, title, description, priority, assignee_id, due_date, tags } = await request.json();

    if (!id) {
      return NextResponse.json({ error: 'Task ID is required' }, { status: 400 });
    }

    await query(
      'SELECT update_task($1, $2, $3, $4, $5, $6, $7, $8)',
      [id, title, description, priority, assignee_id || null, due_date || null, tags || null, userId]
    );

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Update task error:', error);
    return NextResponse.json({ error: 'Failed to update task' }, { status: 500 });
  }
}

// DELETE task
export async function DELETE(request: Request) {
  try {
    const userId = await getUserId();
    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { searchParams } = new URL(request.url);
    const taskId = searchParams.get('id');

    if (!taskId) {
      return NextResponse.json({ error: 'Task ID is required' }, { status: 400 });
    }

    await query('SELECT delete_task($1, $2)', [taskId, userId]);

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Delete task error:', error);
    return NextResponse.json({ error: 'Failed to delete task' }, { status: 500 });
  }
}
