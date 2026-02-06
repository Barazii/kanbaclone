import { describe, it, expect, vi } from 'vitest';
import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import ColumnBoard from '@/components/ColumnBoard';
import { DragDropContext } from '@hello-pangea/dnd';
import { createMockColumn, createMockTask } from '../test-utils';

function renderColumnBoard(
  columnOverrides = {},
  tasks = [createMockTask()],
  props: Partial<{
    onAddTask: ReturnType<typeof vi.fn>;
    onEditTask: ReturnType<typeof vi.fn>;
    onDeleteTask: ReturnType<typeof vi.fn>;
    onDeleteColumn: ReturnType<typeof vi.fn>;
  }> = {}
) {
  const column = createMockColumn(columnOverrides);

  const defaultHandlers = {
    onAddTask: vi.fn(),
    onEditTask: vi.fn(),
    onDeleteTask: vi.fn(),
    onDeleteColumn: vi.fn(),
    ...props,
  };

  const result = render(
    <DragDropContext onDragEnd={() => {}}>
      <ColumnBoard column={column} tasks={tasks} {...defaultHandlers} />
    </DragDropContext>
  );

  return { ...result, column, ...defaultHandlers };
}

describe('ColumnBoard', () => {
  describe('rendering', () => {
    it('should display the column name uppercased', () => {
      renderColumnBoard({ name: 'In Progress' });
      expect(screen.getByText('In Progress')).toBeInTheDocument();
    });

    it('should render task cards for each task', () => {
      const tasks = [
        createMockTask({ id: 't1', title: 'Task One' }),
        createMockTask({ id: 't2', title: 'Task Two' }),
        createMockTask({ id: 't3', title: 'Task Three' }),
      ];
      renderColumnBoard({}, tasks);

      expect(screen.getByText('Task One')).toBeInTheDocument();
      expect(screen.getByText('Task Two')).toBeInTheDocument();
      expect(screen.getByText('Task Three')).toBeInTheDocument();
    });

    it('should render empty column with just Add task button', () => {
      renderColumnBoard({}, []);
      expect(screen.getByText('Add task')).toBeInTheDocument();
    });
  });

  describe('protected columns', () => {
    it('should not show delete button for "To Do" column', () => {
      renderColumnBoard({ name: 'To Do' });
      const deleteButtons = screen.queryAllByTitle('Delete column');
      expect(deleteButtons).toHaveLength(0);
    });

    it('should not show delete button for "Done" column', () => {
      renderColumnBoard({ name: 'Done' });
      const deleteButtons = screen.queryAllByTitle('Delete column');
      expect(deleteButtons).toHaveLength(0);
    });

    it('should not show delete button for "todo" column (case insensitive)', () => {
      renderColumnBoard({ name: 'todo' });
      const deleteButtons = screen.queryAllByTitle('Delete column');
      expect(deleteButtons).toHaveLength(0);
    });

    it('should not show delete button for "Complete" column', () => {
      renderColumnBoard({ name: 'Complete' });
      const deleteButtons = screen.queryAllByTitle('Delete column');
      expect(deleteButtons).toHaveLength(0);
    });

    it('should not show delete button for "Completed" column', () => {
      renderColumnBoard({ name: 'Completed' });
      const deleteButtons = screen.queryAllByTitle('Delete column');
      expect(deleteButtons).toHaveLength(0);
    });

    it('should show delete button for non-protected columns', () => {
      renderColumnBoard({ name: 'In Progress' });
      expect(screen.getByTitle('Delete column')).toBeInTheDocument();
    });

    it('should show delete button for custom columns', () => {
      renderColumnBoard({ name: 'Review' });
      expect(screen.getByTitle('Delete column')).toBeInTheDocument();
    });
  });

  describe('interactions', () => {
    it('should call onAddTask with columnId when Add task is clicked', async () => {
      const user = userEvent.setup();
      const onAddTask = vi.fn();
      const { column } = renderColumnBoard({}, [], { onAddTask });

      await user.click(screen.getByText('Add task'));
      expect(onAddTask).toHaveBeenCalledWith(column.id);
    });

    it('should call onDeleteColumn when delete button is clicked and confirmed', async () => {
      const user = userEvent.setup();
      const onDeleteColumn = vi.fn();
      vi.mocked(window.confirm).mockReturnValue(true);

      const { column } = renderColumnBoard({ name: 'In Progress' }, [], { onDeleteColumn });

      await user.click(screen.getByTitle('Delete column'));
      expect(window.confirm).toHaveBeenCalled();
      expect(onDeleteColumn).toHaveBeenCalledWith(column.id);
    });

    it('should not call onDeleteColumn when confirm is cancelled', async () => {
      const user = userEvent.setup();
      const onDeleteColumn = vi.fn();
      vi.mocked(window.confirm).mockReturnValue(false);

      renderColumnBoard({ name: 'In Progress' }, [], { onDeleteColumn });

      await user.click(screen.getByTitle('Delete column'));
      expect(onDeleteColumn).not.toHaveBeenCalled();
    });

  });
});
