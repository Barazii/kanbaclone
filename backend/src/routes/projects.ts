import { Router, Response } from 'express';
import { query } from '../lib/db.js';
import { authenticate, AuthRequest } from '../middleware/auth.js';

const router = Router();

// All routes require authentication
router.use(authenticate);

// GET /api/projects
router.get('/', async (req: AuthRequest, res: Response) => {
  try {
    const result = await query('SELECT * FROM get_user_projects($1)', [req.userId]);
    return res.json({ projects: result.rows });
  } catch (error) {
    console.error('Get projects error:', error);
    return res.status(500).json({ error: 'Failed to get projects' });
  }
});

// POST /api/projects
router.post('/', async (req: AuthRequest, res: Response) => {
  try {
    const { name, description, icon } = req.body;

    if (!name) {
      return res.status(400).json({ error: 'Project name is required' });
    }

    const result = await query('SELECT create_project($1, $2, $3, $4) as id', [
      name,
      description || null,
      icon || '📋',
      req.userId,
    ]);

    const projectId = result.rows[0].id;

    return res.json({ id: projectId, success: true });
  } catch (error) {
    console.error('Create project error:', error);
    return res.status(500).json({ error: 'Failed to create project' });
  }
});

// GET /api/projects/:id
router.get('/:id', async (req: AuthRequest, res: Response) => {
  try {
    const projectId = req.params.id;

    const projectResult = await query('SELECT * FROM get_project_details($1)', [projectId]);
    if (projectResult.rows.length === 0) {
      return res.status(404).json({ error: 'Project not found' });
    }

    const [columnsResult, tasksResult, membersResult] = await Promise.all([
      query('SELECT * FROM get_project_columns($1)', [projectId]),
      query('SELECT * FROM get_project_tasks($1)', [projectId]),
      query('SELECT * FROM get_project_members($1)', [projectId]),
    ]);

    const tasksByColumn = new Map<string, typeof tasksResult.rows>();
    for (const task of tasksResult.rows) {
      const list = tasksByColumn.get(task.column_id);
      const mapped = { ...task, position: task.task_position };
      if (list) {
        list.push(mapped);
      } else {
        tasksByColumn.set(task.column_id, [mapped]);
      }
    }

    const columns = columnsResult.rows.map((col) => ({
      ...col,
      position: col.col_position,
      tasks: tasksByColumn.get(col.id) || [],
    }));

    return res.json({
      project: projectResult.rows[0],
      columns,
      members: membersResult.rows,
    });
  } catch (error) {
    console.error('Get project error:', error);
    return res.status(500).json({ error: 'Failed to get project' });
  }
});

// DELETE /api/projects/:id
router.delete('/:id', async (req: AuthRequest, res: Response) => {
  try {
    const projectId = req.params.id;

    await query('SELECT delete_project($1, $2)', [projectId, req.userId]);

    return res.json({ success: true });
  } catch (error) {
    console.error('Delete project error:', error);
    return res.status(500).json({ error: 'Failed to delete project' });
  }
});

// POST /api/projects/:id/invite
router.post('/:id/invite', async (req: AuthRequest, res: Response) => {
  try {
    const projectId = req.params.id;
    const { email } = req.body;

    if (!email) {
      return res.status(400).json({ error: 'Email is required' });
    }

    const memberCheck = await query(
      'SELECT role FROM project_members WHERE project_id = $1 AND user_id = $2',
      [projectId, req.userId]
    );

    if (memberCheck.rows.length === 0) {
      return res.status(403).json({ error: 'Not authorized to invite members' });
    }

    const userResult = await query('SELECT id, email FROM users WHERE email = $1', [
      email.toLowerCase(),
    ]);

    if (userResult.rows.length === 0) {
      return res.status(404).json({ error: 'User not found. They need to register first.' });
    }

    const invitedUser = userResult.rows[0] as { id: string; email: string };

    const existingMember = await query(
      'SELECT id FROM project_members WHERE project_id = $1 AND user_id = $2',
      [projectId, invitedUser.id]
    );

    if (existingMember.rows.length > 0) {
      return res.status(400).json({ error: 'User is already a member of this project' });
    }

    await query('SELECT add_project_member($1, $2, $3)', [
      projectId,
      invitedUser.email,
      'member',
    ]);

    return res.json({ success: true, message: 'Member added successfully' });
  } catch (error) {
    console.error('Invite member error:', error);
    return res.status(500).json({ error: 'Failed to invite member' });
  }
});

export default router;
