import { describe, it, expect, vi } from 'vitest';
import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import TaskCard from '@/components/TaskCard';
import { DragDropContext, Droppable } from '@hello-pangea/dnd';
import { createMockTask } from '../test-utils';

// TaskCard uses Draggable, which requires DragDropContext and Droppable
function renderTaskCard(
  taskOverrides = {},
  props: { onEdit?: ReturnType<typeof vi.fn>; onDelete?: ReturnType<typeof vi.fn> } = {}
) {
  const task = createMockTask(taskOverrides);
  const onEdit = props.onEdit ?? vi.fn();
  const onDelete = props.onDelete ?? vi.fn();

  const result = render(
    <DragDropContext onDragEnd={() => {}}>
      <Droppable droppableId="test-column">
        {(provided) => (
          <div ref={provided.innerRef} {...provided.droppableProps}>
            <TaskCard task={task} index={0} onEdit={onEdit} onDelete={onDelete} />
            {provided.placeholder}
          </div>
        )}
      </Droppable>
    </DragDropContext>
  );

  return { ...result, task, onEdit, onDelete };
}

describe('TaskCard', () => {
  describe('rendering', () => {
    it('should display the task title', () => {
      renderTaskCard({ title: 'Important Task' });
      expect(screen.getByText('Important Task')).toBeInTheDocument();
    });

    it('should display the task description when present', () => {
      renderTaskCard({ description: 'This is a detailed description' });
      expect(screen.getByText('This is a detailed description')).toBeInTheDocument();
    });

    it('should display assignee name initial and first name', () => {
      renderTaskCard({ assignee_name: 'John Smith' });
      expect(screen.getByText('J')).toBeInTheDocument();
      expect(screen.getByText('John')).toBeInTheDocument();
    });

    it('should not render assignee section when no assignee', () => {
      renderTaskCard({ assignee_name: undefined, due_date: undefined });
      expect(screen.queryByText('J')).not.toBeInTheDocument();
    });

    it('should display formatted due date', () => {
      renderTaskCard({ due_date: '2024-06-15T00:00:00Z' });
      // Month short, day numeric: "Jun 15"
      expect(screen.getByText('Jun 15')).toBeInTheDocument();
    });

    it('should not render due date section when no due date', () => {
      renderTaskCard({ due_date: undefined, assignee_name: undefined });
      expect(screen.queryByText(/Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec/)).not.toBeInTheDocument();
    });

    it('should display both assignee and due date when both present', () => {
      renderTaskCard({
        assignee_name: 'Alice Wonder',
        due_date: '2024-12-25T00:00:00Z',
      });
      expect(screen.getByText('A')).toBeInTheDocument();
      expect(screen.getByText('Alice')).toBeInTheDocument();
      expect(screen.getByText('Dec 25')).toBeInTheDocument();
    });
  });

  describe('interactions', () => {
    it('should call onEdit with the task when card is clicked', async () => {
      const user = userEvent.setup();
      const onEdit = vi.fn();
      const { task } = renderTaskCard({}, { onEdit });

      await user.click(screen.getByText(task.title));
      expect(onEdit).toHaveBeenCalledWith(task);
    });

    it('should call onDelete with task id when delete button is clicked and confirmed', async () => {
      const user = userEvent.setup();
      const onDelete = vi.fn();
      vi.mocked(window.confirm).mockReturnValue(true);

      const { task, container } = renderTaskCard({}, { onDelete });

      // Find the actual <button> element (not the div with role="button" from Draggable)
      const deleteBtn = container.querySelector('button');
      expect(deleteBtn).toBeTruthy();
      await user.click(deleteBtn!);
      expect(onDelete).toHaveBeenCalledWith(task.id);
    });

    it('should not call onDelete when confirm is cancelled', async () => {
      const user = userEvent.setup();
      const onDelete = vi.fn();
      vi.mocked(window.confirm).mockReturnValue(false);

      const { container } = renderTaskCard({}, { onDelete });

      const deleteBtn = container.querySelector('button');
      expect(deleteBtn).toBeTruthy();
      await user.click(deleteBtn!);
      expect(onDelete).not.toHaveBeenCalled();
    });

    it('should stop propagation on delete click (not trigger onEdit)', async () => {
      const user = userEvent.setup();
      const onEdit = vi.fn();
      const onDelete = vi.fn();
      vi.mocked(window.confirm).mockReturnValue(true);

      const { container } = renderTaskCard({}, { onEdit, onDelete });

      const deleteBtn = container.querySelector('button');
      expect(deleteBtn).toBeTruthy();
      await user.click(deleteBtn!);
      // onEdit should NOT have been called because stopPropagation was used
      expect(onEdit).not.toHaveBeenCalled();
      expect(onDelete).toHaveBeenCalled();
    });
  });
});
