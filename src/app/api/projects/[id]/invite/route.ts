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

// POST invite member by email
export async function POST(
  request: Request,
  { params }: { params: Promise<{ id: string }> }
) {
  try {
    const userId = await getUserId();
    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { id: projectId } = await params;
    const { email } = await request.json();

    if (!email) {
      return NextResponse.json({ error: 'Email is required' }, { status: 400 });
    }

    // Check if user is owner/member of the project
    const memberCheck = await query(
      'SELECT role FROM project_members WHERE project_id = $1 AND user_id = $2',
      [projectId, userId]
    );

    if (memberCheck.rows.length === 0) {
      return NextResponse.json({ error: 'Not authorized to invite members' }, { status: 403 });
    }

    // Find user by email
    const userResult = await query(
      'SELECT id, email FROM users WHERE email = $1',
      [email.toLowerCase()]
    );

    if (userResult.rows.length === 0) {
      return NextResponse.json({ error: 'User not found. They need to register first.' }, { status: 404 });
    }

    const invitedUser = userResult.rows[0] as { id: string; email: string };

    // Check if already a member
    const existingMember = await query(
      'SELECT id FROM project_members WHERE project_id = $1 AND user_id = $2',
      [projectId, invitedUser.id]
    );

    if (existingMember.rows.length > 0) {
      return NextResponse.json({ error: 'User is already a member of this project' }, { status: 400 });
    }

    // Add member to project
    await query(
      'SELECT add_project_member($1, $2, $3)',
      [projectId, invitedUser.id, 'member']
    );

    return NextResponse.json({ success: true, message: 'Member added successfully' });
  } catch (error) {
    console.error('Invite member error:', error);
    return NextResponse.json({ error: 'Failed to invite member' }, { status: 500 });
  }
}
