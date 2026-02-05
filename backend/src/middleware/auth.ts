import { Request, Response, NextFunction } from 'express';
import { getSession, hasSession } from '../lib/session.js';

export interface AuthRequest extends Request {
  userId?: string;
}

export async function authenticate(
  req: AuthRequest,
  res: Response,
  next: NextFunction
) {
  try {
    const sessionId = req.cookies.session;

    if (!sessionId || !(await hasSession(sessionId))) {
      return res.status(401).json({ error: 'Unauthorized' });
    }

    const userId = await getSession(sessionId);
    if (!userId) {
      return res.status(401).json({ error: 'Unauthorized' });
    }

    req.userId = userId;
    next();
  } catch (error) {
    console.error('Auth middleware error:', error);
    return res.status(500).json({ error: 'Authentication failed' });
  }
}
