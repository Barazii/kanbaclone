'use client';

import { useEffect, useState, useCallback, use } from 'react';
import { useRouter } from 'next/navigation';
import { useAuth } from '@/context/AuthContext';
import Sidebar from '@/components/Sidebar';
import ColumnBoard from '@/components/ColumnBoard';
import TaskModal from '@/components/TaskModal';
import ProjectModal from '@/components/ProjectModal';
import { Project, Column, Task, ProjectMember } from '@/types';
import { DragDropContext, DropResult } from '@hello-pangea/dnd';
import { Plus, FolderOpen, ArrowLeft, Users, LayoutGrid, Mail } from 'lucide-react';
import Link from 'next/link';

interface ProjectPageProps {
  params: Promise<{ id: string }>;
}

export default function ProjectPage({ params }: ProjectPageProps) {
  const { id: projectId } = use(params);
  const { user, loading: authLoading } = useAuth();
  const router = useRouter();
  
  const [project, setProject] = useState<Project | null>(null);
  const [columns, setColumns] = useState<Column[]>([]);
  const [members, setMembers] = useState<ProjectMember[]>([]);
  const [projects, setProjects] = useState<Project[]>([]);
  const [loading, setLoading] = useState(true);
  const [activeTab, setActiveTab] = useState<'board' | 'members'>('board');
  
  const [showTaskModal, setShowTaskModal] = useState(false);
  const [showProjectModal, setShowProjectModal] = useState(false);
  const [selectedTask, setSelectedTask] = useState<Task | null>(null);
  const [selectedColumnId, setSelectedColumnId] = useState<string>('');
  
  const [inviteEmail, setInviteEmail] = useState('');
  const [inviteLoading, setInviteLoading] = useState(false);
  const [inviteMessage, setInviteMessage] = useState<{ type: 'success' | 'error'; text: string } | null>(null);

  useEffect(() => {
    if (!authLoading && !user) {
      router.push('/login');
    }
  }, [user, authLoading, router]);

  const fetchProjectData = useCallback(async () => {
    try {
      const res = await fetch(`/api/projects/${projectId}`);
      if (res.ok) {
        const data = await res.json();
        setProject(data.project);
        setColumns(data.columns || []);
        setMembers(data.members || []);
      } else {
        router.push('/dashboard');
      }
    } catch (error) {
      console.error('Failed to fetch project:', error);
    } finally {
      setLoading(false);
    }
  }, [projectId, router]);

  const fetchProjects = useCallback(async () => {
    try {
      const res = await fetch('/api/projects');
      if (res.ok) {
        const data = await res.json();
        setProjects(data.projects || []);
      }
    } catch (error) {
      console.error('Failed to fetch projects:', error);
    }
  }, []);

  useEffect(() => {
    if (user) {
      fetchProjectData();
      fetchProjects();
    }
  }, [user, fetchProjectData, fetchProjects]);

  const handleDragEnd = async (result: DropResult) => {
    const { destination, source, draggableId } = result;

    if (!destination) return;
    if (destination.droppableId === source.droppableId && destination.index === source.index) return;

    // Optimistically update UI
    const newColumns = [...columns];
    const sourceCol = newColumns.find(c => c.id === source.droppableId);
    const destCol = newColumns.find(c => c.id === destination.droppableId);

    if (!sourceCol || !destCol) return;

    const [movedTask] = sourceCol.tasks!.splice(source.index, 1);
    movedTask.column_id = destination.droppableId;
    destCol.tasks!.splice(destination.index, 0, movedTask);

    setColumns(newColumns);

    // Persist to backend
    try {
      await fetch('/api/tasks/move', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          task_id: draggableId,
          column_id: destination.droppableId,
          position: destination.index,
        }),
      });
    } catch (error) {
      console.error('Failed to move task:', error);
      fetchProjectData(); // Revert on error
    }
  };

  const handleAddTask = (columnId: string) => {
    setSelectedColumnId(columnId);
    setSelectedTask(null);
    setShowTaskModal(true);
  };

  const handleEditTask = (task: Task) => {
    setSelectedTask(task);
    setSelectedColumnId(task.column_id);
    setShowTaskModal(true);
  };

  const handleSaveTask = async (taskData: Partial<Task>) => {
    try {
      if (taskData.id) {
        // Update existing task
        await fetch('/api/tasks', {
          method: 'PUT',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(taskData),
        });
      } else {
        // Create new task
        await fetch('/api/tasks', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            ...taskData,
            column_id: selectedColumnId,
          }),
        });
      }
      setShowTaskModal(false);
      fetchProjectData();
    } catch (error) {
      console.error('Failed to save task:', error);
    }
  };

  const handleDeleteTask = async (taskId: string) => {
    try {
      await fetch(`/api/tasks?id=${taskId}`, { method: 'DELETE' });
      setShowTaskModal(false);
      fetchProjectData();
    } catch (error) {
      console.error('Failed to delete task:', error);
    }
  };

  const handleAddColumn = async () => {
    const name = prompt('Enter column name:');
    if (!name) return;

    try {
      await fetch('/api/columns', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          project_id: projectId,
          name,
        }),
      });
      fetchProjectData();
    } catch (error) {
      console.error('Failed to add column:', error);
    }
  };

  const handleDeleteColumn = async (columnId: string) => {
    try {
      await fetch(`/api/columns?id=${columnId}`, { method: 'DELETE' });
      fetchProjectData();
    } catch (error) {
      console.error('Failed to delete column:', error);
    }
  };

  const handleInviteMember = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!inviteEmail.trim()) return;
    
    setInviteLoading(true);
    setInviteMessage(null);
    
    try {
      const res = await fetch(`/api/projects/${projectId}/invite`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email: inviteEmail.trim() }),
      });
      
      const data = await res.json();
      
      if (res.ok) {
        setInviteMessage({ type: 'success', text: 'Member added successfully!' });
        setInviteEmail('');
        fetchProjectData();
      } else {
        setInviteMessage({ type: 'error', text: data.error || 'Failed to invite member' });
      }
    } catch (error) {
      setInviteMessage({ type: 'error', text: 'Failed to invite member' });
    } finally {
      setInviteLoading(false);
    }
  };

  const handleCreateProject = async (data: { name: string; description: string; icon: string }) => {
    try {
      const res = await fetch('/api/projects', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data),
      });

      if (res.ok) {
        const result = await res.json();
        setShowProjectModal(false);
        router.push(`/projects/${result.id}`);
      }
    } catch (error) {
      console.error('Failed to create project:', error);
    }
  };

  if (authLoading || loading) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-white">
        <div className="animate-spin rounded-full h-6 w-6 border border-gray-200 border-t-black"></div>
      </div>
    );
  }

  if (!user || !project) return null;

  return (
    <div className="flex h-screen bg-white">
      <Sidebar projects={projects} onNewProject={() => setShowProjectModal(true)} />
      
      <main className="flex-1 flex flex-col overflow-hidden p-4">
        <div className="flex-1 flex flex-col bg-gray-50 rounded-xl overflow-hidden">
          {/* Project Header */}
          <header className="px-6 py-4 border-b border-gray-100">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-4">
                <Link 
                  href="/dashboard"
                  className="p-2 hover:bg-white rounded-lg transition-colors"
                >
                  <ArrowLeft className="w-4 h-4 text-gray-400" />
                </Link>
                <div className="w-9 h-9 bg-white border border-gray-100 rounded-lg flex items-center justify-center">
                  <FolderOpen className="w-4 h-4 text-gray-500" />
                </div>
                <div>
                  <h1 className="text-sm font-normal text-black">{project.name}</h1>
                  {project.description && (
                    <p className="text-xs font-light text-gray-400 mt-0.5">{project.description}</p>
                  )}
                </div>
              </div>
              
              {/* Tabs */}
              <div className="flex items-center gap-1 bg-white border border-gray-100 rounded-lg p-1">
                <button
                  onClick={() => setActiveTab('board')}
                  className={`flex items-center gap-2 px-3 py-1.5 rounded text-xs font-light transition-colors ${
                    activeTab === 'board'
                      ? 'bg-black text-white'
                      : 'text-gray-500 hover:text-black'
                  }`}
                >
                  <LayoutGrid className="w-3.5 h-3.5" />
                  Board
                </button>
                <button
                  onClick={() => setActiveTab('members')}
                  className={`flex items-center gap-2 px-3 py-1.5 rounded text-xs font-light transition-colors ${
                    activeTab === 'members'
                      ? 'bg-black text-white'
                      : 'text-gray-500 hover:text-black'
                  }`}
                >
                  <Users className="w-3.5 h-3.5" />
                  Members
                </button>
              </div>
            </div>
          </header>

          {/* Board Tab */}
          {activeTab === 'board' && (
            <div className="flex-1 overflow-x-auto p-6">
              <DragDropContext onDragEnd={handleDragEnd}>
                <div className="flex gap-4 h-full">
                  {columns.map((column) => (
                    <ColumnBoard
                      key={column.id}
                      column={column}
                      tasks={column.tasks || []}
                      onAddTask={handleAddTask}
                      onEditTask={handleEditTask}
                      onDeleteTask={handleDeleteTask}
                      onDeleteColumn={handleDeleteColumn}
                    />
                  ))}

                  {/* Add Column Button */}
                  <div className="flex-shrink-0 w-64">
                    <button
                      onClick={handleAddColumn}
                      className="w-full h-full min-h-[120px] bg-white border border-dashed border-gray-200 rounded-lg flex flex-col items-center justify-center text-gray-400 hover:border-gray-300 hover:text-gray-500 transition-all"
                    >
                      <Plus className="w-5 h-5 mb-1" />
                      <span className="text-xs font-light">Add Column</span>
                    </button>
                  </div>
                </div>
              </DragDropContext>
            </div>
          )}

          {/* Members Tab */}
          {activeTab === 'members' && (
            <div className="flex-1 overflow-y-auto p-6">
              <div className="max-w-xl">
                {/* Invite Form */}
                <div className="mb-8">
                  <h2 className="text-sm font-normal text-black mb-4">Invite Member</h2>
                  <form onSubmit={handleInviteMember} className="flex gap-3">
                    <div className="flex-1 relative">
                      <Mail className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-gray-400" />
                      <input
                        type="email"
                        value={inviteEmail}
                        onChange={(e) => setInviteEmail(e.target.value)}
                        placeholder="Enter email address"
                        className="w-full pl-10 pr-4 py-2.5 text-sm font-light bg-white border border-gray-200 rounded-lg focus:outline-none focus:border-black transition-colors"
                        required
                      />
                    </div>
                    <button
                      type="submit"
                      disabled={inviteLoading}
                      className="px-5 py-2.5 bg-black text-white text-sm font-light hover:bg-gray-900 transition-colors disabled:opacity-50"
                    >
                      {inviteLoading ? 'Inviting...' : 'Invite'}
                    </button>
                  </form>
                  {inviteMessage && (
                    <p className={`mt-2 text-xs font-light ${inviteMessage.type === 'success' ? 'text-green-600' : 'text-red-600'}`}>
                      {inviteMessage.text}
                    </p>
                  )}
                </div>

                {/* Members List */}
                <h2 className="text-sm font-normal text-black mb-4">Team Members</h2>
                <div className="space-y-2">
                  {members.map((member) => (
                    <div key={member.id} className="bg-white border border-gray-100 rounded-lg p-4 flex items-center justify-between">
                      <div className="flex items-center gap-3">
                        <div className="w-8 h-8 bg-gray-100 rounded-full flex items-center justify-center text-xs font-light text-gray-600">
                          {member.name.charAt(0).toUpperCase()}
                        </div>
                        <div>
                          <p className="text-sm font-normal text-black">{member.name}</p>
                          <p className="text-xs font-light text-gray-400">{member.email}</p>
                        </div>
                      </div>
                      <span className={`text-xs font-light px-2 py-1 rounded ${
                        member.role === 'owner' ? 'bg-black text-white' : 'bg-gray-100 text-gray-600'
                      }`}>
                        {member.role}
                      </span>
                    </div>
                  ))}
                </div>
              </div>
            </div>
          )}
        </div>
      </main>

      {showTaskModal && (
        <TaskModal
          task={selectedTask || undefined}
          columnId={selectedColumnId}
          members={members}
          onClose={() => setShowTaskModal(false)}
          onSave={handleSaveTask}
          onDelete={selectedTask ? handleDeleteTask : undefined}
        />
      )}

      {showProjectModal && (
        <ProjectModal
          onClose={() => setShowProjectModal(false)}
          onSave={handleCreateProject}
        />
      )}
    </div>
  );
}
