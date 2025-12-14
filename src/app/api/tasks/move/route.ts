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

// POST move task to different column/position
export async function POST(request: Request) {
  try {
    const userId = await getUserId();
    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { task_id, column_id, position } = await request.json();

    if (!task_id || !column_id || position === undefined) {
      return NextResponse.json(
        { error: 'Task ID, column ID, and position are required' },
        { status: 400 }
      );
    }

    await query(
      'SELECT move_task($1, $2, $3, $4)',
      [task_id, column_id, position, userId]
    );

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Move task error:', error);
    return NextResponse.json({ error: 'Failed to move task' }, { status: 500 });
  }
}
