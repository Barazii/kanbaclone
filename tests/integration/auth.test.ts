import { describe, it, expect, beforeEach } from 'vitest';
import { POST as register } from '@/app/api/auth/register/route';
import { POST as login } from '@/app/api/auth/login/route';
import { GET as getMe } from '@/app/api/auth/me/route';
import { POST as logout } from '@/app/api/auth/logout/route';
import { PUT as updateUser } from '@/app/api/auth/update/route';
import { truncateAll } from '../helpers/testDb';
import { clearCookiesForTest } from '../helpers/mockCookies';
import { clearAllSessions } from '@/lib/session';

function jsonReq(url: string, body: Record<string, unknown>, method = 'POST') {
  return new Request(url, {
    method,
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  });
}

describe('Auth API', () => {
  beforeEach(async () => {
    await truncateAll();
    clearCookiesForTest();
    clearAllSessions();
  });

  describe('POST /register', () => {
    it('creates user and returns user object', async () => {
      const res = await register(
        jsonReq('http://localhost/api/auth/register', {
          email: 'test@test.com',
          password: 'password123',
          name: 'Test User',
        })
      );
      const data = await res.json();

      expect(res.status).toBe(200);
      expect(data.user).toBeDefined();
      expect(data.user.email).toBe('test@test.com');
      expect(data.user.name).toBe('Test User');
      expect(data.user.id).toBeDefined();
    });

    it('returns 400 when fields are missing', async () => {
      const res = await register(
        jsonReq('http://localhost/api/auth/register', { email: 'test@test.com' })
      );
      expect(res.status).toBe(400);
    });

    it('returns 400 for duplicate email', async () => {
      await register(
        jsonReq('http://localhost/api/auth/register', {
          email: 'dup@test.com',
          password: 'password123',
          name: 'User 1',
        })
      );

      const res = await register(
        jsonReq('http://localhost/api/auth/register', {
          email: 'dup@test.com',
          password: 'password123',
          name: 'User 2',
        })
      );

      expect(res.status).toBe(400);
      const data = await res.json();
      expect(data.error).toContain('already exists');
    });

    it('sets session cookie after registration', async () => {
      const res = await register(
        jsonReq('http://localhost/api/auth/register', {
          email: 'cookie@test.com',
          password: 'password123',
          name: 'Cookie User',
        })
      );
      expect(res.status).toBe(200);

      // After registration, the session should be set so /me works
      const meRes = await getMe();
      expect(meRes.status).toBe(200);
      const meData = await meRes.json();
      expect(meData.user.email).toBe('cookie@test.com');
    });
  });

  describe('POST /login', () => {
    beforeEach(async () => {
      // Create a user first
      await register(
        jsonReq('http://localhost/api/auth/register', {
          email: 'login@test.com',
          password: 'password123',
          name: 'Login User',
        })
      );
      // Clear session so we can test login fresh
      clearCookiesForTest();
      clearAllSessions();
    });

    it('authenticates with valid credentials', async () => {
      const res = await login(
        jsonReq('http://localhost/api/auth/login', {
          email: 'login@test.com',
          password: 'password123',
        })
      );

      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.user.email).toBe('login@test.com');
    });

    it('returns 401 for wrong password', async () => {
      const res = await login(
        jsonReq('http://localhost/api/auth/login', {
          email: 'login@test.com',
          password: 'wrongpassword',
        })
      );
      expect(res.status).toBe(401);
    });

    it('returns 401 for non-existent email', async () => {
      const res = await login(
        jsonReq('http://localhost/api/auth/login', {
          email: 'nobody@test.com',
          password: 'password123',
        })
      );
      expect(res.status).toBe(401);
    });

    it('returns 400 when fields are missing', async () => {
      const res = await login(
        jsonReq('http://localhost/api/auth/login', { email: 'login@test.com' })
      );
      expect(res.status).toBe(400);
    });
  });

  describe('GET /me', () => {
    it('returns user when authenticated', async () => {
      await register(
        jsonReq('http://localhost/api/auth/register', {
          email: 'me@test.com',
          password: 'password123',
          name: 'Me User',
        })
      );

      const res = await getMe();
      expect(res.status).toBe(200);
      const data = await res.json();
      expect(data.user.email).toBe('me@test.com');
      expect(data.user.name).toBe('Me User');
    });

    it('returns 401 when no session', async () => {
      const res = await getMe();
      expect(res.status).toBe(401);
    });
  });

  describe('POST /logout', () => {
    it('clears session and subsequent /me returns 401', async () => {
      await register(
        jsonReq('http://localhost/api/auth/register', {
          email: 'logout@test.com',
          password: 'password123',
          name: 'Logout User',
        })
      );

      const logoutRes = await logout();
      expect(logoutRes.status).toBe(200);

      const meRes = await getMe();
      expect(meRes.status).toBe(401);
    });
  });

  describe('PUT /update', () => {
    it('updates user name', async () => {
      await register(
        jsonReq('http://localhost/api/auth/register', {
          email: 'update@test.com',
          password: 'password123',
          name: 'Old Name',
        })
      );

      const res = await updateUser(
        jsonReq('http://localhost/api/auth/update', { name: 'New Name' }, 'PUT')
      );
      expect(res.status).toBe(200);

      const meRes = await getMe();
      const meData = await meRes.json();
      expect(meData.user.name).toBe('New Name');
    });

    it('returns 400 for empty name', async () => {
      await register(
        jsonReq('http://localhost/api/auth/register', {
          email: 'update2@test.com',
          password: 'password123',
          name: 'User',
        })
      );

      const res = await updateUser(
        jsonReq('http://localhost/api/auth/update', { name: '' }, 'PUT')
      );
      expect(res.status).toBe(400);
    });

    it('returns 401 when not authenticated', async () => {
      const res = await updateUser(
        jsonReq('http://localhost/api/auth/update', { name: 'New Name' }, 'PUT')
      );
      expect(res.status).toBe(401);
    });
  });
});
