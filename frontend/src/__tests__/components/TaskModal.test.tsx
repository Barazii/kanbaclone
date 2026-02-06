import { describe, it, expect, vi } from 'vitest';
import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import TaskModal from '@/components/TaskModal';
import { createMockTask, createMockMember } from '../test-utils';

describe('TaskModal', () => {
  const defaultProps = {
    columnId: 'col-1',
    members: [
      createMockMember({ user_id: 'u1', name: 'Alice', email: 'alice@example.com' }),
      createMockMember({ user_id: 'u2', name: 'Bob', email: 'bob@example.com', role: 'member' as const }),
    ],
    onClose: vi.fn(),
    onSave: vi.fn(),
  };

  describe('create mode (no task prop)', () => {
    it('should default to medium priority', () => {
      render(<TaskModal {...defaultProps} />);
      // The medium button should have the selected class
      const mediumBtn = screen.getByRole('button', { name: /medium/i });
      expect(mediumBtn).toHaveClass('bg-yellow-100');
    });

    it('should call onSave with form data on submit', async () => {
      const user = userEvent.setup();
      const onSave = vi.fn();

      render(<TaskModal {...defaultProps} onSave={onSave} />);

      await user.type(screen.getByPlaceholderText('Enter task title'), 'New Task Title');
      await user.type(screen.getByPlaceholderText('Add a description'), 'Task desc');
      await user.click(screen.getByRole('button', { name: 'Create Task' }));

      expect(onSave).toHaveBeenCalledWith(
        expect.objectContaining({
          id: undefined,
          column_id: 'col-1',
          title: 'New Task Title',
          description: 'Task desc',
          priority: 'medium',
          assignee_id: undefined,
          due_date: undefined,
        })
      );
    });

    it('should not render the Delete button in create mode', () => {
      render(<TaskModal {...defaultProps} />);
      expect(screen.queryByRole('button', { name: 'Delete' })).not.toBeInTheDocument();
    });

  });

  describe('edit mode (with task prop)', () => {
    const existingTask = createMockTask({
      id: 'task-99',
      title: 'Existing Task',
      description: 'Old description',
      priority: 'high',
      assignee_id: 'u1',
      due_date: '2024-06-15T00:00:00Z',
    });

    it('should render the Delete button when task and onDelete are provided', () => {
      render(<TaskModal {...defaultProps} task={existingTask} onDelete={vi.fn()} />);
      expect(screen.getByRole('button', { name: 'Delete' })).toBeInTheDocument();
    });

    it('should call onDelete with task id when Delete is clicked', async () => {
      const user = userEvent.setup();
      const onDelete = vi.fn();

      render(<TaskModal {...defaultProps} task={existingTask} onDelete={onDelete} />);
      await user.click(screen.getByRole('button', { name: 'Delete' }));

      expect(onDelete).toHaveBeenCalledWith('task-99');
    });

    it('should call onSave with updated data on submit', async () => {
      const user = userEvent.setup();
      const onSave = vi.fn();

      render(<TaskModal {...defaultProps} onSave={onSave} task={existingTask} />);

      const titleInput = screen.getByPlaceholderText('Enter task title');
      await user.clear(titleInput);
      await user.type(titleInput, 'Updated Task');
      await user.click(screen.getByRole('button', { name: 'Save Changes' }));

      expect(onSave).toHaveBeenCalledWith(
        expect.objectContaining({
          id: 'task-99',
          title: 'Updated Task',
        })
      );
    });
  });

  describe('priority selection', () => {
    it('should allow switching to low priority', async () => {
      const user = userEvent.setup();

      render(<TaskModal {...defaultProps} />);

      await user.click(screen.getByRole('button', { name: /low/i }));

      // low button should be selected
      const lowBtn = screen.getByRole('button', { name: /low/i });
      expect(lowBtn).toHaveClass('bg-blue-100');
    });

    it('should allow switching to high priority', async () => {
      const user = userEvent.setup();

      render(<TaskModal {...defaultProps} />);

      await user.click(screen.getByRole('button', { name: /high/i }));

      const highBtn = screen.getByRole('button', { name: /high/i });
      expect(highBtn).toHaveClass('bg-red-100');
    });

    it('should include the priority in save data', async () => {
      const user = userEvent.setup();
      const onSave = vi.fn();

      render(<TaskModal {...defaultProps} onSave={onSave} />);

      await user.type(screen.getByPlaceholderText('Enter task title'), 'Test');
      await user.click(screen.getByRole('button', { name: /high/i }));
      await user.click(screen.getByRole('button', { name: 'Create Task' }));

      expect(onSave).toHaveBeenCalledWith(
        expect.objectContaining({ priority: 'high' })
      );
    });
  });

  describe('assignee selection', () => {
    it('should render members as options', () => {
      render(<TaskModal {...defaultProps} />);
      const select = screen.getByRole('combobox');

      expect(select).toBeInTheDocument();
      // Check for unassigned option plus member names
      expect(screen.getByText('Unassigned')).toBeInTheDocument();
      expect(screen.getByText('Alice')).toBeInTheDocument();
      expect(screen.getByText('Bob')).toBeInTheDocument();
    });

    it('should render with no members', () => {
      render(<TaskModal {...defaultProps} members={[]} />);
      const select = screen.getByRole('combobox');
      expect(select).toBeInTheDocument();
      expect(screen.getByText('Unassigned')).toBeInTheDocument();
    });

    it('should include assignee_id in save data when selected', async () => {
      const user = userEvent.setup();
      const onSave = vi.fn();

      render(<TaskModal {...defaultProps} onSave={onSave} />);

      await user.type(screen.getByPlaceholderText('Enter task title'), 'Test');
      await user.selectOptions(screen.getByRole('combobox'), 'u2');
      await user.click(screen.getByRole('button', { name: 'Create Task' }));

      expect(onSave).toHaveBeenCalledWith(
        expect.objectContaining({ assignee_id: 'u2' })
      );
    });

    it('should set assignee_id to undefined when Unassigned is selected', async () => {
      const user = userEvent.setup();
      const onSave = vi.fn();

      render(<TaskModal {...defaultProps} onSave={onSave} />);

      await user.type(screen.getByPlaceholderText('Enter task title'), 'Test');
      // Select Bob then switch back to unassigned
      await user.selectOptions(screen.getByRole('combobox'), 'u2');
      await user.selectOptions(screen.getByRole('combobox'), '');
      await user.click(screen.getByRole('button', { name: 'Create Task' }));

      expect(onSave).toHaveBeenCalledWith(
        expect.objectContaining({ assignee_id: undefined })
      );
    });
  });

  describe('close button and cancel', () => {
    it('should call onClose when Cancel is clicked', async () => {
      const user = userEvent.setup();
      const onClose = vi.fn();

      render(<TaskModal {...defaultProps} onClose={onClose} />);
      await user.click(screen.getByRole('button', { name: 'Cancel' }));

      expect(onClose).toHaveBeenCalledTimes(1);
    });

    it('should call onClose when X button is clicked', async () => {
      const user = userEvent.setup();
      const onClose = vi.fn();

      render(<TaskModal {...defaultProps} onClose={onClose} />);
      // Get all buttons and find the close X button
      const buttons = screen.getAllByRole('button');
      const closeBtn = buttons.find(
        (btn) =>
          btn.textContent !== 'Cancel' &&
          btn.textContent !== 'Create Task' &&
          btn.textContent !== 'Save Changes' &&
          btn.textContent !== 'Delete' &&
          !btn.textContent?.match(/low|medium|high/i)
      );
      await user.click(closeBtn!);

      expect(onClose).toHaveBeenCalledTimes(1);
    });
  });

});
