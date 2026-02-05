import { describe, it, expect, beforeEach } from 'vitest';
import { POST as createColumn, PUT as updateColumn, DELETE as deleteColumn } from '@/app/api/columns/route';
import { truncateAll } from '../helpers/testDb';
import { clearCookiesForTest } from '../helpers/mockCookies';
import { clearAllSessions } from '@/lib/session';
import { createAuthenticatedUser, createTestProject, getProjectColumns } from '../helpers/auth';

function jsonReq(url: string, body: Record<string, unknown>, method = 'POST') {
  return new Request(url, {
    method,
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  });
}

describe('Columns API', () => {
  let projectId: string;

  beforeEach(async () => {
    await truncateAll();
    clearCookiesForTest();
    clearAllSessions();
    await createAuthenticatedUser();
    projectId = await createTestProject();
  });

  describe('POST /columns', () => {
    it('creates a column in project', async () => {
      const res = await createColumn(
        jsonReq('http://localhost/api/columns', {
          project_id: projectId,
          name: 'In Progress',
          color: '#f59e0b',
        })
      );
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.id).toBeDefined();
      expect(data.success).toBe(true);

      // Verify column exists in project
      const columns = await getProjectColumns(projectId);
      const colNames = columns.map((c: { name: string }) => c.name);
      expect(colNames).toContain('In Progress');
    });

    it('returns 400 without project_id or name', async () => {
      const res = await createColumn(
        jsonReq('http://localhost/api/columns', { project_id: projectId })
      );
      expect(res.status).toBe(400);
    });

    it('returns 401 when not authenticated', async () => {
      clearCookiesForTest();
      clearAllSessions();
      const res = await createColumn(
        jsonReq('http://localhost/api/columns', {
          project_id: projectId,
          name: 'Blocked',
        })
      );
      expect(res.status).toBe(401);
    });
  });

  describe('PUT /columns', () => {
    it('updates column name and color', async () => {
      const columns = await getProjectColumns(projectId);
      const columnId = columns[0].id;

      const res = await updateColumn(
        jsonReq('http://localhost/api/columns', {
          id: columnId,
          name: 'Renamed',
          color: '#ef4444',
        }, 'PUT')
      );
      expect(res.status).toBe(200);

      // Verify the update
      const updatedColumns = await getProjectColumns(projectId);
      const updated = updatedColumns.find((c: { id: string }) => c.id === columnId);
      expect(updated.name).toBe('Renamed');
      expect(updated.color).toBe('#ef4444');
    });

    it('returns 400 without id', async () => {
      const res = await updateColumn(
        jsonReq('http://localhost/api/columns', { name: 'No ID' }, 'PUT')
      );
      expect(res.status).toBe(400);
    });

    it('returns 401 when not authenticated', async () => {
      clearCookiesForTest();
      clearAllSessions();
      const res = await updateColumn(
        jsonReq('http://localhost/api/columns', { id: 'fake', name: 'X' }, 'PUT')
      );
      expect(res.status).toBe(401);
    });
  });

  describe('DELETE /columns', () => {
    it('deletes a column', async () => {
      // Create an extra column first (project starts with To Do and Done)
      const createRes = await createColumn(
        jsonReq('http://localhost/api/columns', {
          project_id: projectId,
          name: 'To Delete',
        })
      );
      const { id: newColId } = await createRes.json();

      const res = await deleteColumn(
        new Request(`http://localhost/api/columns?id=${newColId}`, { method: 'DELETE' })
      );
      expect(res.status).toBe(200);

      const columns = await getProjectColumns(projectId);
      const ids = columns.map((c: { id: string }) => c.id);
      expect(ids).not.toContain(newColId);
    });

    it('returns 400 without id query param', async () => {
      const res = await deleteColumn(
        new Request('http://localhost/api/columns', { method: 'DELETE' })
      );
      expect(res.status).toBe(400);
    });

    it('returns 401 when not authenticated', async () => {
      clearCookiesForTest();
      clearAllSessions();
      const res = await deleteColumn(
        new Request('http://localhost/api/columns?id=fake', { method: 'DELETE' })
      );
      expect(res.status).toBe(401);
    });
  });
});
