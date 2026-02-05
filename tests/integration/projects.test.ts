import { describe, it, expect, beforeEach } from 'vitest';
import { GET as getProjects, POST as createProject } from '@/app/api/projects/route';
import { GET as getProjectById, DELETE as deleteProject } from '@/app/api/projects/[id]/route';
import { POST as inviteMember } from '@/app/api/projects/[id]/invite/route';
import { truncateAll } from '../helpers/testDb';
import { clearCookiesForTest } from '../helpers/mockCookies';
import { clearAllSessions } from '@/lib/session';
import { createAuthenticatedUser } from '../helpers/auth';

function jsonReq(url: string, body: Record<string, unknown>, method = 'POST') {
  return new Request(url, {
    method,
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  });
}

describe('Projects API', () => {
  beforeEach(async () => {
    await truncateAll();
    clearCookiesForTest();
    clearAllSessions();
  });

  describe('GET /projects', () => {
    it('returns empty array for new user', async () => {
      await createAuthenticatedUser();
      const res = await getProjects();
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.projects).toEqual([]);
    });

    it('returns 401 when not authenticated', async () => {
      const res = await getProjects();
      expect(res.status).toBe(401);
    });

    it('returns created projects', async () => {
      await createAuthenticatedUser();
      await createProject(
        jsonReq('http://localhost/api/projects', { name: 'Project A' })
      );
      await createProject(
        jsonReq('http://localhost/api/projects', { name: 'Project B' })
      );
      const res = await getProjects();
      const data = await res.json();
      expect(data.projects.length).toBe(2);
      const names = data.projects.map((p: { name: string }) => p.name);
      expect(names).toContain('Project A');
      expect(names).toContain('Project B');
    });
  });

  describe('POST /projects', () => {
    it('creates project and returns id', async () => {
      await createAuthenticatedUser();
      const res = await createProject(
        jsonReq('http://localhost/api/projects', { name: 'My Project' })
      );
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.id).toBeDefined();
      expect(data.success).toBe(true);
    });

    it('returns 401 when not authenticated', async () => {
      const res = await createProject(
        jsonReq('http://localhost/api/projects', { name: 'No Auth' })
      );
      expect(res.status).toBe(401);
    });

    it('returns 400 without name', async () => {
      await createAuthenticatedUser();
      const res = await createProject(
        jsonReq('http://localhost/api/projects', {})
      );
      expect(res.status).toBe(400);
    });

    it('creates project with default columns (To Do, Done)', async () => {
      await createAuthenticatedUser();
      const createRes = await createProject(
        jsonReq('http://localhost/api/projects', { name: 'Col Project' })
      );
      const { id: projectId } = await createRes.json();

      const detailRes = await getProjectById(
        new Request(`http://localhost/api/projects/${projectId}`),
        { params: Promise.resolve({ id: projectId }) }
      );
      const data = await detailRes.json();
      const colNames = data.columns.map((c: { name: string }) => c.name);
      expect(colNames).toContain('To Do');
      expect(colNames).toContain('Done');
    });
  });

  describe('GET /projects/[id]', () => {
    it('returns project with columns and members', async () => {
      await createAuthenticatedUser();
      const createRes = await createProject(
        jsonReq('http://localhost/api/projects', { name: 'Detail Project' })
      );
      const { id: projectId } = await createRes.json();

      const res = await getProjectById(
        new Request(`http://localhost/api/projects/${projectId}`),
        { params: Promise.resolve({ id: projectId }) }
      );
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.project).toBeDefined();
      expect(data.columns).toBeDefined();
      expect(data.members).toBeDefined();
      expect(data.members.length).toBeGreaterThanOrEqual(1);
    });

    it('returns 401 when not authenticated', async () => {
      const fakeId = '00000000-0000-0000-0000-000000000000';
      const res = await getProjectById(
        new Request(`http://localhost/api/projects/${fakeId}`),
        { params: Promise.resolve({ id: fakeId }) }
      );
      expect(res.status).toBe(401);
    });

    it('returns 404 for non-existent project', async () => {
      await createAuthenticatedUser();
      const fakeId = '00000000-0000-0000-0000-000000000000';
      const res = await getProjectById(
        new Request(`http://localhost/api/projects/${fakeId}`),
        { params: Promise.resolve({ id: fakeId }) }
      );
      expect(res.status).toBe(404);
    });
  });

  describe('DELETE /projects/[id]', () => {
    it('deletes project for owner', async () => {
      await createAuthenticatedUser();
      const createRes = await createProject(
        jsonReq('http://localhost/api/projects', { name: 'Delete Me' })
      );
      const { id: projectId } = await createRes.json();

      const res = await deleteProject(
        new Request(`http://localhost/api/projects/${projectId}`, { method: 'DELETE' }),
        { params: Promise.resolve({ id: projectId }) }
      );
      expect(res.status).toBe(200);

      // Verify it's gone
      const getRes = await getProjectById(
        new Request(`http://localhost/api/projects/${projectId}`),
        { params: Promise.resolve({ id: projectId }) }
      );
      expect(getRes.status).toBe(404);
    });

    it('returns 401 when not authenticated', async () => {
      const fakeId = '00000000-0000-0000-0000-000000000000';
      const res = await deleteProject(
        new Request(`http://localhost/api/projects/${fakeId}`, { method: 'DELETE' }),
        { params: Promise.resolve({ id: fakeId }) }
      );
      expect(res.status).toBe(401);
    });
  });

  describe('POST /projects/[id]/invite', () => {
    it('adds member by email', async () => {
      await createAuthenticatedUser('owner@test.com', 'pass123', 'Owner');

      // Create a second user (need to clear cookies, register, then restore owner session)
      // We'll insert the second user directly via register then re-auth as owner
      clearCookiesForTest();
      clearAllSessions();
      await createAuthenticatedUser('invitee@test.com', 'pass123', 'Invitee');

      // Re-authenticate as owner
      clearCookiesForTest();
      clearAllSessions();
      const { POST: loginPost } = await import('@/app/api/auth/login/route');
      await loginPost(
        jsonReq('http://localhost/api/auth/login', {
          email: 'owner@test.com',
          password: 'pass123',
        })
      );

      // Create project as owner
      const createRes = await createProject(
        jsonReq('http://localhost/api/projects', { name: 'Team Project' })
      );
      const { id: projectId } = await createRes.json();

      // Invite the second user
      const res = await inviteMember(
        jsonReq(`http://localhost/api/projects/${projectId}/invite`, {
          email: 'invitee@test.com',
        }),
        { params: Promise.resolve({ id: projectId }) }
      );
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.success).toBe(true);
    });

    it('returns 404 for unregistered email', async () => {
      await createAuthenticatedUser();
      const createRes = await createProject(
        jsonReq('http://localhost/api/projects', { name: 'Invite Project' })
      );
      const { id: projectId } = await createRes.json();

      const res = await inviteMember(
        jsonReq(`http://localhost/api/projects/${projectId}/invite`, {
          email: 'nobody@nowhere.com',
        }),
        { params: Promise.resolve({ id: projectId }) }
      );
      expect(res.status).toBe(404);
    });

    it('returns 400 when email is missing', async () => {
      await createAuthenticatedUser();
      const createRes = await createProject(
        jsonReq('http://localhost/api/projects', { name: 'No Email Project' })
      );
      const { id: projectId } = await createRes.json();

      const res = await inviteMember(
        jsonReq(`http://localhost/api/projects/${projectId}/invite`, {}),
        { params: Promise.resolve({ id: projectId }) }
      );
      expect(res.status).toBe(400);
    });

    it('returns 401 when not authenticated', async () => {
      const fakeId = '00000000-0000-0000-0000-000000000000';
      clearCookiesForTest();
      clearAllSessions();
      const res = await inviteMember(
        jsonReq(`http://localhost/api/projects/${fakeId}/invite`, {
          email: 'someone@test.com',
        }),
        { params: Promise.resolve({ id: fakeId }) }
      );
      expect(res.status).toBe(401);
    });

    it('returns 403 when non-member tries to invite', async () => {
      // Create owner and project
      await createAuthenticatedUser('projowner@test.com', 'pass123', 'ProjOwner');
      const createRes = await createProject(
        jsonReq('http://localhost/api/projects', { name: 'Private Project' })
      );
      const { id: projectId } = await createRes.json();

      // Create a third user to be invited
      clearCookiesForTest();
      clearAllSessions();
      await createAuthenticatedUser('third@test.com', 'pass123', 'Third');

      // Create outsider and try to invite from their session
      clearCookiesForTest();
      clearAllSessions();
      await createAuthenticatedUser('outsider@test.com', 'pass123', 'Outsider');

      const res = await inviteMember(
        jsonReq(`http://localhost/api/projects/${projectId}/invite`, {
          email: 'third@test.com',
        }),
        { params: Promise.resolve({ id: projectId }) }
      );
      expect(res.status).toBe(403);
    });

    it('returns 400 for already-member', async () => {
      await createAuthenticatedUser('owner2@test.com', 'pass123', 'Owner2');

      // Create second user
      clearCookiesForTest();
      clearAllSessions();
      await createAuthenticatedUser('member@test.com', 'pass123', 'Member');

      // Re-auth as owner
      clearCookiesForTest();
      clearAllSessions();
      const { POST: loginPost } = await import('@/app/api/auth/login/route');
      await loginPost(
        jsonReq('http://localhost/api/auth/login', {
          email: 'owner2@test.com',
          password: 'pass123',
        })
      );

      const createRes = await createProject(
        jsonReq('http://localhost/api/projects', { name: 'Dup Invite' })
      );
      const { id: projectId } = await createRes.json();

      // Invite once
      await inviteMember(
        jsonReq(`http://localhost/api/projects/${projectId}/invite`, {
          email: 'member@test.com',
        }),
        { params: Promise.resolve({ id: projectId }) }
      );

      // Invite again — should be 400
      const res = await inviteMember(
        jsonReq(`http://localhost/api/projects/${projectId}/invite`, {
          email: 'member@test.com',
        }),
        { params: Promise.resolve({ id: projectId }) }
      );
      expect(res.status).toBe(400);
      const data = await res.json();
      expect(data.error).toContain('already a member');
    });
  });
});
