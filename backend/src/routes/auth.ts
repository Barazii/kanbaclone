import { Router, Request, Response } from 'express';
import bcrypt from 'bcryptjs';
import { v4 as uuidv4 } from 'uuid';
import { query } from '../lib/db.js';
import { setSession, deleteSession, getSession, hasSession } from '../lib/session.js';
import { authenticate, AuthRequest } from '../middleware/auth.js';

const router = Router();

// POST /api/auth/login
router.post('/login', async (req: Request, res: Response) => {
  try {
    const { email, password } = req.body;

    if (!email || !password) {
      return res.status(400).json({ error: 'Missing required fields' });
    }

    const result = await query('SELECT * FROM get_user_by_email($1)', [email]);

    if (result.rows.length === 0) {
      return res.status(401).json({ error: 'Invalid credentials' });
    }

    const user = result.rows[0] as {
      id: string;
      email: string;
      password_hash: string;
      name: string;
      avatar_url: string;
      created_at: string;
    };

    const validPassword = await bcrypt.compare(password, user.password_hash);
    if (!validPassword) {
      return res.status(401).json({ error: 'Invalid credentials' });
    }

    const sessionId = uuidv4();
    await setSession(sessionId, user.id);

    res.cookie('session', sessionId, {
      httpOnly: true,
      secure: process.env.NODE_ENV === 'production',
      sameSite: process.env.NODE_ENV === 'production' ? 'none' : 'lax',
      maxAge: 60 * 60 * 24 * 7 * 1000, // 7 days in ms
    });

    return res.json({
      user: {
        id: user.id,
        email: user.email,
        name: user.name,
        avatar_url: user.avatar_url,
        created_at: user.created_at,
      },
    });
  } catch (error) {
    console.error('Login error:', error);
    return res.status(500).json({ error: 'Login failed' });
  }
});

// POST /api/auth/register
router.post('/register', async (req: Request, res: Response) => {
  try {
    const { email, password, name } = req.body;

    if (!email || !password || !name) {
      return res.status(400).json({ error: 'Missing required fields' });
    }

    const existingUser = await query('SELECT * FROM get_user_by_email($1)', [email]);
    if (existingUser.rows.length > 0) {
      return res.status(400).json({ error: 'User already exists' });
    }

    const passwordHash = await bcrypt.hash(password, 10);

    const result = await query('SELECT * FROM create_user($1, $2, $3)', [
      email,
      passwordHash,
      name,
    ]);

    const user = result.rows[0];

    const sessionId = uuidv4();
    await setSession(sessionId, user.id);

    res.cookie('session', sessionId, {
      httpOnly: true,
      secure: process.env.NODE_ENV === 'production',
      sameSite: process.env.NODE_ENV === 'production' ? 'none' : 'lax',
      maxAge: 60 * 60 * 24 * 7 * 1000,
    });

    return res.json({
      user: {
        id: user.id,
        email: user.email,
        name: user.name,
        avatar_url: user.avatar_url,
        created_at: user.created_at,
      },
    });
  } catch (error) {
    console.error('Registration error:', error);
    return res.status(500).json({ error: 'Registration failed' });
  }
});

// POST /api/auth/logout
router.post('/logout', async (req: Request, res: Response) => {
  try {
    const sessionId = req.cookies.session;

    if (sessionId) {
      await deleteSession(sessionId);
      res.clearCookie('session');
    }

    return res.json({ success: true });
  } catch (error) {
    console.error('Logout error:', error);
    return res.status(500).json({ error: 'Logout failed' });
  }
});

// GET /api/auth/me
router.get('/me', async (req: Request, res: Response) => {
  try {
    const sessionId = req.cookies.session;

    if (!sessionId || !(await hasSession(sessionId))) {
      return res.status(401).json({ error: 'Not authenticated' });
    }

    const userId = await getSession(sessionId);

    const result = await query('SELECT * FROM get_user_by_id($1)', [userId]);

    if (result.rows.length === 0) {
      return res.status(404).json({ error: 'User not found' });
    }

    const user = result.rows[0] as {
      id: string;
      email: string;
      name: string;
      avatar_url: string;
      created_at: string;
    };

    return res.json({
      user: {
        id: user.id,
        email: user.email,
        name: user.name,
        avatar_url: user.avatar_url,
        created_at: user.created_at,
      },
    });
  } catch (error) {
    console.error('Auth check error:', error);
    return res.status(500).json({ error: 'Auth check failed' });
  }
});

// PUT /api/auth/update
router.put('/update', authenticate, async (req: AuthRequest, res: Response) => {
  try {
    const { name } = req.body;

    if (!name || name.trim().length === 0) {
      return res.status(400).json({ error: 'Name is required' });
    }

    await query('UPDATE users SET name = $1, updated_at = NOW() WHERE id = $2', [
      name.trim(),
      req.userId,
    ]);

    return res.json({ success: true });
  } catch (error) {
    console.error('Update user error:', error);
    return res.status(500).json({ error: 'Failed to update user' });
  }
});

export default router;
