import { query } from './db.js';

const SESSION_TTL_SECONDS = 60 * 60 * 24 * 7; // 7 days

export async function setSession(sessionId: string, userId: string) {
  await query(
    `INSERT INTO sessions (id, user_id, expires_at)
     VALUES ($1, $2, NOW() + INTERVAL '${SESSION_TTL_SECONDS} seconds')
     ON CONFLICT (id) DO UPDATE SET user_id = $2, expires_at = NOW() + INTERVAL '${SESSION_TTL_SECONDS} seconds'`,
    [sessionId, userId]
  );
}

export async function getSession(sessionId: string): Promise<string | undefined> {
  const result = await query<{ user_id: string }>(
    'SELECT user_id FROM sessions WHERE id = $1 AND expires_at > NOW()',
    [sessionId]
  );
  return result.rows[0]?.user_id;
}

export async function deleteSession(sessionId: string) {
  await query('DELETE FROM sessions WHERE id = $1', [sessionId]);
}

export async function hasSession(sessionId: string): Promise<boolean> {
  const result = await query<{ exists: boolean }>(
    'SELECT EXISTS(SELECT 1 FROM sessions WHERE id = $1 AND expires_at > NOW()) AS exists',
    [sessionId]
  );
  return result.rows[0]?.exists ?? false;
}

export async function clearAllSessions() {
  await query('DELETE FROM sessions');
}

export async function cleanExpiredSessions() {
  await query('DELETE FROM sessions WHERE expires_at <= NOW()');
}
