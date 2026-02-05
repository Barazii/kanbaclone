import { describe, it, expect, beforeEach, vi } from 'vitest';

// Mock the db module
vi.mock('@/lib/db', () => ({
  query: vi.fn(),
}));

import { query } from '@/lib/db';
import {
  setSession,
  getSession,
  deleteSession,
  hasSession,
  clearAllSessions,
} from '@/lib/session';

const mockQuery = vi.mocked(query);

describe('Session Store', () => {
  beforeEach(() => {
    mockQuery.mockReset();
  });

  it('stores a session by inserting into DB', async () => {
    mockQuery.mockResolvedValueOnce({ rows: [], rowCount: 1, command: 'INSERT', oid: 0, fields: [] });
    await setSession('sess-1', 'user-1');
    expect(mockQuery).toHaveBeenCalledWith(
      expect.stringContaining('INSERT INTO sessions'),
      ['sess-1', 'user-1']
    );
  });

  it('getSession returns user_id for valid session', async () => {
    mockQuery.mockResolvedValueOnce({ rows: [{ user_id: 'user-1' }], rowCount: 1, command: 'SELECT', oid: 0, fields: [] });
    const result = await getSession('sess-1');
    expect(result).toBe('user-1');
  });

  it('hasSession returns true for existing session', async () => {
    mockQuery.mockResolvedValueOnce({ rows: [{ exists: true }], rowCount: 1, command: 'SELECT', oid: 0, fields: [] });
    const result = await hasSession('sess-1');
    expect(result).toBe(true);
  });

  it('hasSession returns false for non-existing session', async () => {
    mockQuery.mockResolvedValueOnce({ rows: [{ exists: false }], rowCount: 1, command: 'SELECT', oid: 0, fields: [] });
    const result = await hasSession('nonexistent');
    expect(result).toBe(false);
  });

  it('deleteSession removes the session', async () => {
    mockQuery.mockResolvedValueOnce({ rows: [], rowCount: 1, command: 'DELETE', oid: 0, fields: [] });
    await deleteSession('sess-1');
    expect(mockQuery).toHaveBeenCalledWith(
      'DELETE FROM sessions WHERE id = $1',
      ['sess-1']
    );
  });

  it('clearAllSessions deletes all sessions', async () => {
    mockQuery.mockResolvedValueOnce({ rows: [], rowCount: 0, command: 'DELETE', oid: 0, fields: [] });
    await clearAllSessions();
    expect(mockQuery).toHaveBeenCalledWith('DELETE FROM sessions');
  });
});
