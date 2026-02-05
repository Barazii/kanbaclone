import { Column, Task } from '@/types';
import { Plus, Trash2 } from 'lucide-react';
import TaskCard from './TaskCard';
import StrictModeDroppable from './StrictModeDroppable';

interface ColumnBoardProps {
  column: Column;
  tasks: Task[];
  onAddTask: (columnId: string) => void;
  onEditTask: (task: Task) => void;
  onDeleteTask: (taskId: string) => void;
  onDeleteColumn: (columnId: string) => void;
}

const protectedColumns = ['to do', 'todo', 'done', 'complete', 'completed'];

export default function ColumnBoard({
  column,
  tasks,
  onAddTask,
  onEditTask,
  onDeleteTask,
  onDeleteColumn,
}: ColumnBoardProps) {
  const isProtected = protectedColumns.includes(column.name.toLowerCase());

  const handleDeleteColumn = () => {
    if (isProtected) return;
    if (confirm(`Are you sure you want to delete "${column.name}"? Tasks will be moved to the first column.`)) {
      onDeleteColumn(column.id);
    }
  };

  return (
    <div className="flex-shrink-0 w-64 bg-white border border-black rounded-lg flex flex-col max-h-full group">
      <div className="p-3 border-b border-black">
        <div className="flex items-center justify-between">
          <h3 className="text-xs font-semibold text-black uppercase tracking-wide">{column.name}</h3>
          {!isProtected && (
            <button
              onClick={handleDeleteColumn}
              className="p-1 opacity-0 group-hover:opacity-100 hover:bg-black/10 rounded transition-all"
              title="Delete column"
            >
              <Trash2 className="w-3 h-3 text-black" />
            </button>
          )}
        </div>
      </div>

      <StrictModeDroppable droppableId={column.id}>
        {(provided, snapshot) => (
          <div
            ref={provided.innerRef}
            {...provided.droppableProps}
            className={`flex-1 overflow-y-auto p-2 transition-colors ${
              snapshot.isDraggingOver ? 'bg-black/5' : ''
            }`}
            style={{ minHeight: '80px' }}
          >
            <div className="space-y-2">
              {tasks.map((task, index) => (
                <TaskCard
                  key={task.id}
                  task={task}
                  index={index}
                  onEdit={onEditTask}
                  onDelete={onDeleteTask}
                />
              ))}
            </div>
            {provided.placeholder}

            <button
              onClick={() => onAddTask(column.id)}
              className="w-full mt-2 p-2 text-black hover:bg-black/5 rounded transition-colors flex items-center justify-center gap-1 text-xs font-semibold"
            >
              <Plus className="w-3.5 h-3.5" />
              <span>Add task</span>
            </button>
          </div>
        )}
      </StrictModeDroppable>
    </div>
  );
}
