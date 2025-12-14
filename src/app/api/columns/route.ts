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

// POST create column
export async function POST(request: Request) {
  try {
    const userId = await getUserId();
    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { project_id, name, color } = await request.json();

    if (!project_id || !name) {
      return NextResponse.json({ error: 'Project ID and name are required' }, { status: 400 });
    }

    const result = await query(
      'SELECT create_column($1, $2, $3) as id',
      [project_id, name, color || '#6366f1']
    );

    return NextResponse.json({ id: result.rows[0].id, success: true });
  } catch (error) {
    console.error('Create column error:', error);
    return NextResponse.json({ error: 'Failed to create column' }, { status: 500 });
  }
}

// PUT update column
export async function PUT(request: Request) {
  try {
    const userId = await getUserId();
    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { id, name, color } = await request.json();

    if (!id) {
      return NextResponse.json({ error: 'Column ID is required' }, { status: 400 });
    }

    await query('SELECT update_column($1, $2, $3)', [id, name, color]);

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Update column error:', error);
    return NextResponse.json({ error: 'Failed to update column' }, { status: 500 });
  }
}

// DELETE column
export async function DELETE(request: Request) {
  try {
    const userId = await getUserId();
    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { searchParams } = new URL(request.url);
    const columnId = searchParams.get('id');

    if (!columnId) {
      return NextResponse.json({ error: 'Column ID is required' }, { status: 400 });
    }

    await query('SELECT delete_column($1)', [columnId]);

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error('Delete column error:', error);
    return NextResponse.json({ error: 'Failed to delete column' }, { status: 500 });
  }
}
