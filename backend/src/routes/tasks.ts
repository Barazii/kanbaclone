import { Router, Response } from 'express';
import { query } from '../lib/db.js';
import { authenticate, AuthRequest } from '../middleware/auth.js';

const router = Router();

router.use(authenticate);

// POST /api/tasks
router.post('/', async (req: AuthRequest, res: Response) => {
  try {
    const { column_id, title, description, priority, assignee_id, due_date, tags } = req.body;

    if (!column_id || !title) {
      return res.status(400).json({ error: 'Column ID and title are required' });
    }

    const result = await query('SELECT create_task($1, $2, $3, $4, $5, $6, $7, $8) as id', [
      column_id,
      title,
      description || null,
      priority || 'medium',
      assignee_id || null,
      due_date || null,
      tags || null,
      req.userId,
    ]);

    const taskId = result.rows[0].id;

    return res.json({ id: taskId, success: true });
  } catch (error) {
    console.error('Create task error:', error);
    return res.status(500).json({ error: 'Failed to create task' });
  }
});

// PUT /api/tasks
router.put('/', async (req: AuthRequest, res: Response) => {
  try {
    const { id, title, description, priority, assignee_id, due_date, tags } = req.body;

    if (!id) {
      return res.status(400).json({ error: 'Task ID is required' });
    }

    await query('SELECT update_task($1, $2, $3, $4, $5, $6, $7, $8)', [
      id,
      title,
      description,
      priority,
      assignee_id || null,
      due_date || null,
      tags || null,
      req.userId,
    ]);

    return res.json({ success: true });
  } catch (error) {
    console.error('Update task error:', error);
    return res.status(500).json({ error: 'Failed to update task' });
  }
});

// DELETE /api/tasks
router.delete('/', async (req: AuthRequest, res: Response) => {
  try {
    const taskId = req.query.id as string;

    if (!taskId) {
      return res.status(400).json({ error: 'Task ID is required' });
    }

    await query('SELECT delete_task($1, $2)', [taskId, req.userId]);

    return res.json({ success: true });
  } catch (error) {
    console.error('Delete task error:', error);
    return res.status(500).json({ error: 'Failed to delete task' });
  }
});

// POST /api/tasks/move
router.post('/move', async (req: AuthRequest, res: Response) => {
  try {
    const { task_id, column_id, position } = req.body;

    if (!task_id || !column_id || position === undefined) {
      return res.status(400).json({ error: 'Task ID, column ID, and position are required' });
    }

    await query('SELECT move_task($1, $2, $3, $4)', [task_id, column_id, position, req.userId]);

    return res.json({ success: true });
  } catch (error) {
    console.error('Move task error:', error);
    return res.status(500).json({ error: 'Failed to move task' });
  }
});

export default router;
