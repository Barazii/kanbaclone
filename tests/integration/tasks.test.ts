import { describe, it, expect, beforeEach } from 'vitest';
import { POST as createTask, PUT as updateTask, DELETE as deleteTask } from '@/app/api/tasks/route';
import { POST as moveTask } from '@/app/api/tasks/move/route';
import { GET as getProjectById } from '@/app/api/projects/[id]/route';
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

describe('Tasks API', () => {
  let projectId: string;
  let todoColumnId: string;
  let doneColumnId: string;

  beforeEach(async () => {
    await truncateAll();
    clearCookiesForTest();
    clearAllSessions();
    await createAuthenticatedUser();
    projectId = await createTestProject();

    const columns = await getProjectColumns(projectId);
    const todoCol = columns.find((c: { name: string }) => c.name === 'To Do');
    const doneCol = columns.find((c: { name: string }) => c.name === 'Done');
    todoColumnId = todoCol.id;
    doneColumnId = doneCol.id;
  });

  describe('POST /tasks', () => {
    it('creates a task in a column', async () => {
      const res = await createTask(
        jsonReq('http://localhost/api/tasks', {
          column_id: todoColumnId,
          title: 'My Task',
        })
      );
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.id).toBeDefined();
      expect(data.success).toBe(true);
    });

    it('returns 400 without column_id or title', async () => {
      const res = await createTask(
        jsonReq('http://localhost/api/tasks', { column_id: todoColumnId })
      );
      expect(res.status).toBe(400);
    });

    it('returns 401 when not authenticated', async () => {
      clearCookiesForTest();
      clearAllSessions();
      const res = await createTask(
        jsonReq('http://localhost/api/tasks', {
          column_id: todoColumnId,
          title: 'Unauth Task',
        })
      );
      expect(res.status).toBe(401);
    });

    it('creates task with all optional fields', async () => {
      const res = await createTask(
        jsonReq('http://localhost/api/tasks', {
          column_id: todoColumnId,
          title: 'Full Task',
          description: 'A detailed description',
          priority: 'high',
          tags: ['bug', 'urgent'],
          due_date: '2026-12-31T00:00:00Z',
        })
      );
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.id).toBeDefined();
    });

    it('task appears in project after creation', async () => {
      await createTask(
        jsonReq('http://localhost/api/tasks', {
          column_id: todoColumnId,
          title: 'Visible Task',
        })
      );

      const res = await getProjectById(
        new Request(`http://localhost/api/projects/${projectId}`),
        { params: Promise.resolve({ id: projectId }) }
      );
      const data = await res.json();
      const todoCol = data.columns.find((c: { name: string }) => c.name === 'To Do');
      const taskTitles = todoCol.tasks.map((t: { title: string }) => t.title);
      expect(taskTitles).toContain('Visible Task');
    });
  });

  describe('PUT /tasks', () => {
    let taskId: string;

    beforeEach(async () => {
      const res = await createTask(
        jsonReq('http://localhost/api/tasks', {
          column_id: todoColumnId,
          title: 'Original Title',
          priority: 'low',
        })
      );
      const data = await res.json();
      taskId = data.id;
    });

    it('updates task title and priority', async () => {
      const res = await updateTask(
        jsonReq('http://localhost/api/tasks', {
          id: taskId,
          title: 'Updated Title',
          priority: 'high',
        }, 'PUT')
      );
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.success).toBe(true);
    });

    it('returns 400 without id', async () => {
      const res = await updateTask(
        jsonReq('http://localhost/api/tasks', { title: 'No ID' }, 'PUT')
      );
      expect(res.status).toBe(400);
    });

    it('returns 401 when not authenticated', async () => {
      clearCookiesForTest();
      clearAllSessions();
      const res = await updateTask(
        jsonReq('http://localhost/api/tasks', { id: taskId, title: 'Nope' }, 'PUT')
      );
      expect(res.status).toBe(401);
    });
  });

  describe('DELETE /tasks', () => {
    let taskId: string;

    beforeEach(async () => {
      const res = await createTask(
        jsonReq('http://localhost/api/tasks', {
          column_id: todoColumnId,
          title: 'To Delete',
        })
      );
      const data = await res.json();
      taskId = data.id;
    });

    it('deletes task by id', async () => {
      const res = await deleteTask(
        new Request(`http://localhost/api/tasks?id=${taskId}`, { method: 'DELETE' })
      );
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.success).toBe(true);
    });

    it('returns 400 without id query param', async () => {
      const res = await deleteTask(
        new Request('http://localhost/api/tasks', { method: 'DELETE' })
      );
      expect(res.status).toBe(400);
    });

    it('returns 401 when not authenticated', async () => {
      clearCookiesForTest();
      clearAllSessions();
      const res = await deleteTask(
        new Request(`http://localhost/api/tasks?id=${taskId}`, { method: 'DELETE' })
      );
      expect(res.status).toBe(401);
    });
  });

  describe('POST /tasks/move', () => {
    let taskId: string;

    beforeEach(async () => {
      const res = await createTask(
        jsonReq('http://localhost/api/tasks', {
          column_id: todoColumnId,
          title: 'Movable Task',
        })
      );
      const data = await res.json();
      taskId = data.id;
    });

    it('moves task to a different column', async () => {
      const res = await moveTask(
        jsonReq('http://localhost/api/tasks/move', {
          task_id: taskId,
          column_id: doneColumnId,
          position: 0,
        })
      );
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.success).toBe(true);
    });

    it('returns 400 when fields are missing', async () => {
      const res = await moveTask(
        jsonReq('http://localhost/api/tasks/move', {
          task_id: taskId,
        })
      );
      expect(res.status).toBe(400);
    });

    it('returns 401 when not authenticated', async () => {
      clearCookiesForTest();
      clearAllSessions();
      const res = await moveTask(
        jsonReq('http://localhost/api/tasks/move', {
          task_id: taskId,
          column_id: doneColumnId,
          position: 0,
        })
      );
      expect(res.status).toBe(401);
    });

    it('task appears in new column after move', async () => {
      await moveTask(
        jsonReq('http://localhost/api/tasks/move', {
          task_id: taskId,
          column_id: doneColumnId,
          position: 0,
        })
      );

      const res = await getProjectById(
        new Request(`http://localhost/api/projects/${projectId}`),
        { params: Promise.resolve({ id: projectId }) }
      );
      const data = await res.json();
      const doneCol = data.columns.find((c: { name: string }) => c.name === 'Done');
      const taskIds = doneCol.tasks.map((t: { id: string }) => t.id);
      expect(taskIds).toContain(taskId);
    });
  });
});
