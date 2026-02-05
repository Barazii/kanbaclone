import { Router, Response } from 'express';
import { query } from '../lib/db.js';
import { authenticate, AuthRequest } from '../middleware/auth.js';

const router = Router();

router.use(authenticate);

// POST /api/columns
router.post('/', async (req: AuthRequest, res: Response) => {
  try {
    const { project_id, name, color } = req.body;

    if (!project_id || !name) {
      return res.status(400).json({ error: 'Project ID and name are required' });
    }

    const result = await query('SELECT create_column($1, $2, $3) as id', [
      project_id,
      name,
      color || '#6366f1',
    ]);

    return res.json({ id: result.rows[0].id, success: true });
  } catch (error) {
    console.error('Create column error:', error);
    return res.status(500).json({ error: 'Failed to create column' });
  }
});

// PUT /api/columns
router.put('/', async (req: AuthRequest, res: Response) => {
  try {
    const { id, name, color } = req.body;

    if (!id) {
      return res.status(400).json({ error: 'Column ID is required' });
    }

    await query('SELECT update_column($1, $2, $3)', [id, name, color]);

    return res.json({ success: true });
  } catch (error) {
    console.error('Update column error:', error);
    return res.status(500).json({ error: 'Failed to update column' });
  }
});

// DELETE /api/columns
router.delete('/', async (req: AuthRequest, res: Response) => {
  try {
    const columnId = req.query.id as string;

    if (!columnId) {
      return res.status(400).json({ error: 'Column ID is required' });
    }

    await query('SELECT delete_column($1)', [columnId]);

    return res.json({ success: true });
  } catch (error) {
    console.error('Delete column error:', error);
    return res.status(500).json({ error: 'Failed to delete column' });
  }
});

export default router;
