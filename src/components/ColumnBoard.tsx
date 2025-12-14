'use client';

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

// Protected columns that cannot be deleted
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
    <div className="flex-shrink-0 w-64 bg-white border border-gray-100 rounded-lg flex flex-col max-h-full group">
      {/* Column Header */}
      <div className="p-3 border-b border-gray-50">
        <div className="flex items-center justify-between">
          <h3 className="text-xs font-normal text-black uppercase tracking-wide">{column.name}</h3>
          {!isProtected && (
            <button
              onClick={handleDeleteColumn}
              className="p-1 opacity-0 group-hover:opacity-100 hover:bg-gray-100 rounded transition-all"
              title="Delete column"
            >
              <Trash2 className="w-3 h-3 text-gray-400 hover:text-black" />
            </button>
          )}
        </div>
      </div>

      {/* Tasks List */}
      <StrictModeDroppable droppableId={column.id}>
        {(provided, snapshot) => (
          <div
            ref={provided.innerRef}
            {...provided.droppableProps}
            className={`flex-1 overflow-y-auto p-2 transition-colors ${
              snapshot.isDraggingOver ? 'bg-gray-50' : ''
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
            
            {/* Inline Add Task */}
            <button
              onClick={() => onAddTask(column.id)}
              className="w-full mt-2 p-2 text-gray-400 hover:text-black hover:bg-gray-50 rounded transition-colors flex items-center justify-center gap-1 text-xs font-light"
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
