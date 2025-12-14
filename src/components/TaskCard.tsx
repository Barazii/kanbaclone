'use client';

import { Task } from '@/types';
import { Trash2 } from 'lucide-react';
import { Draggable } from '@hello-pangea/dnd';

interface TaskCardProps {
  task: Task;
  index: number;
  onEdit: (task: Task) => void;
  onDelete: (taskId: string) => void;
}

export default function TaskCard({ task, index, onEdit, onDelete }: TaskCardProps) {
  const handleDelete = (e: React.MouseEvent) => {
    e.stopPropagation();
    if (confirm('Are you sure you want to delete this task?')) {
      onDelete(task.id);
    }
  };

  return (
    <Draggable draggableId={task.id} index={index}>
      {(provided, snapshot) => (
        <div
          ref={provided.innerRef}
          {...provided.draggableProps}
          {...provided.dragHandleProps}
          className={`bg-gray-50 border border-gray-100 rounded p-3 cursor-pointer hover:border-gray-200 transition-all group ${
            snapshot.isDragging ? 'shadow-md border-gray-300' : ''
          }`}
          onClick={() => onEdit(task)}
        >
          <div className="flex items-start justify-between gap-2">
            <h4 className="text-xs font-normal text-black flex-1 line-clamp-2">
              {task.title}
            </h4>
            <button
              onClick={handleDelete}
              className="p-1 opacity-0 group-hover:opacity-100 hover:bg-gray-200 rounded transition-all"
            >
              <Trash2 className="w-3 h-3 text-gray-400 hover:text-black" />
            </button>
          </div>
          
          {task.description && (
            <p className="text-[11px] font-light text-gray-400 mt-1 line-clamp-2">{task.description}</p>
          )}
          
          {/* Footer */}
          {(task.assignee_name || task.due_date) && (
            <div className="flex items-center justify-between mt-2 pt-2 border-t border-gray-100">
              {task.assignee_name ? (
                <div className="flex items-center gap-1">
                  <div className="w-4 h-4 bg-gray-200 rounded-full flex items-center justify-center text-[9px] font-light text-gray-600">
                    {task.assignee_name.charAt(0).toUpperCase()}
                  </div>
                  <span className="text-[10px] font-light text-gray-400">{task.assignee_name.split(' ')[0]}</span>
                </div>
              ) : (
                <div />
              )}
              
              {task.due_date && (
                <span className="text-[10px] font-light text-gray-400">
                  {new Date(task.due_date).toLocaleDateString('en-US', { month: 'short', day: 'numeric' })}
                </span>
              )}
            </div>
          )}
        </div>
      )}
    </Draggable>
  );
}
