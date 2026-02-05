import { useEffect, useState, useCallback } from 'react';
import { useNavigate, useParams, Link } from 'react-router-dom';
import { useAuth } from '@/context/AuthContext';
import { useProjects } from '@/context/ProjectsContext';
import Sidebar from '@/components/Sidebar';
import ColumnBoard from '@/components/ColumnBoard';
import TaskModal from '@/components/TaskModal';
import { Project, Column, Task, ProjectMember } from '@/types';
import { DragDropContext, DropResult } from '@hello-pangea/dnd';
import { Plus, FolderOpen, ArrowLeft, Users, LayoutGrid, Mail, Trash2, MoreVertical } from 'lucide-react';
import { api } from '@/api/client';

export default function ProjectBoard() {
  const { id: projectId } = useParams<{ id: string }>();
  const { user, loading: authLoading } = useAuth();
  const navigate = useNavigate();

  const [project, setProject] = useState<Project | null>(null);
  const [columns, setColumns] = useState<Column[]>([]);
  const [members, setMembers] = useState<ProjectMember[]>([]);
  const { projects } = useProjects();
  const [loading, setLoading] = useState(true);
  const [activeTab, setActiveTab] = useState<'board' | 'members'>('board');

  const [showTaskModal, setShowTaskModal] = useState(false);
  const [selectedTask, setSelectedTask] = useState<Task | null>(null);
  const [selectedColumnId, setSelectedColumnId] = useState<string>('');

  const [inviteEmail, setInviteEmail] = useState('');
  const [inviteLoading, setInviteLoading] = useState(false);
  const [inviteMessage, setInviteMessage] = useState<{ type: 'success' | 'error'; text: string } | null>(
    null
  );
  const [showProjectMenu, setShowProjectMenu] = useState(false);

  useEffect(() => {
    if (!authLoading && !user) {
      navigate('/login');
    }
  }, [user, authLoading, navigate]);

  const fetchProjectData = useCallback(async () => {
    if (!projectId) return;
    try {
      const data = await api.get<{
        project: Project;
        columns: Column[];
        members: ProjectMember[];
      }>(`/projects/${projectId}`);
      setProject(data.project);
      setColumns(data.columns || []);
      setMembers(data.members || []);
    } catch {
      navigate('/dashboard');
    } finally {
      setLoading(false);
    }
  }, [projectId, navigate]);

  useEffect(() => {
    if (user) {
      fetchProjectData();
    }
  }, [user, fetchProjectData]);

  const handleDragEnd = async (result: DropResult) => {
    const { destination, source, draggableId } = result;

    if (!destination) return;
    if (destination.droppableId === source.droppableId && destination.index === source.index) return;

    const newColumns = columns.map((col) => {
      if (col.id === source.droppableId || col.id === destination.droppableId) {
        return { ...col, tasks: [...(col.tasks || [])] };
      }
      return col;
    });
    const sourceCol = newColumns.find((c) => c.id === source.droppableId);
    const destCol = newColumns.find((c) => c.id === destination.droppableId);

    if (!sourceCol || !destCol) return;

    const [movedTask] = sourceCol.tasks!.splice(source.index, 1);
    const updatedTask = { ...movedTask, column_id: destination.droppableId };
    destCol.tasks!.splice(destination.index, 0, updatedTask);

    setColumns(newColumns);

    try {
      await api.post('/tasks/move', {
        task_id: draggableId,
        column_id: destination.droppableId,
        position: destination.index,
      });
    } catch (error) {
      console.error('Failed to move task:', error);
      fetchProjectData();
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
        await api.put('/tasks', taskData);
      } else {
        await api.post('/tasks', {
          ...taskData,
          column_id: selectedColumnId,
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
      await api.delete(`/tasks?id=${taskId}`);
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
      await api.post('/columns', {
        project_id: projectId,
        name,
      });
      fetchProjectData();
    } catch (error) {
      console.error('Failed to add column:', error);
    }
  };

  const handleDeleteColumn = async (columnId: string) => {
    try {
      await api.delete(`/columns?id=${columnId}`);
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
      await api.post(`/projects/${projectId}/invite`, { email: inviteEmail.trim() });
      setInviteMessage({ type: 'success', text: 'Member added successfully!' });
      setInviteEmail('');
      fetchProjectData();
    } catch (err) {
      const message = err instanceof Error ? err.message : 'Failed to invite member';
      setInviteMessage({ type: 'error', text: message });
    } finally {
      setInviteLoading(false);
    }
  };

  const handleDeleteProject = async () => {
    if (!confirm('Are you sure you want to delete this project? This action cannot be undone.')) {
      return;
    }

    try {
      await api.delete(`/projects/${projectId}`);
      navigate('/dashboard');
    } catch (err) {
      const message = err instanceof Error ? err.message : 'Failed to delete project';
      alert(message);
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
      <Sidebar projects={projects} />

      <main className="flex-1 flex flex-col overflow-hidden p-4">
        <div className="flex-1 flex flex-col bg-white border border-black rounded-xl overflow-hidden">
          <header className="px-6 py-4 border-b border-black">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-4">
                <Link to="/dashboard" className="p-2 hover:bg-black/[0.02] rounded-lg transition-colors">
                  <ArrowLeft className="w-4 h-4 text-black" />
                </Link>
                <div className="w-9 h-9 bg-white border border-black rounded-lg flex items-center justify-center">
                  <FolderOpen className="w-4 h-4 text-black" />
                </div>
                <div>
                  <h1 className="text-sm font-normal text-black">{project.name}</h1>
                  {project.description && (
                    <p className="text-xs font-light text-black/40 mt-0.5">{project.description}</p>
                  )}
                </div>
              </div>

              <div className="flex items-center gap-3">
                <div className="flex items-center gap-1 bg-white border border-black/10 rounded-lg p-1">
                  <button
                    onClick={() => setActiveTab('board')}
                    className={`flex items-center gap-2 px-3 py-1.5 rounded text-xs font-light transition-colors ${
                      activeTab === 'board' ? 'bg-black text-white' : 'text-black/50 hover:text-black'
                    }`}
                  >
                    <LayoutGrid className="w-3.5 h-3.5" />
                    Board
                  </button>
                  <button
                    onClick={() => setActiveTab('members')}
                    className={`flex items-center gap-2 px-3 py-1.5 rounded text-xs font-light transition-colors ${
                      activeTab === 'members' ? 'bg-black text-white' : 'text-black/50 hover:text-black'
                    }`}
                  >
                    <Users className="w-3.5 h-3.5" />
                    Members
                  </button>
                </div>

                <div className="relative">
                  <button
                    onClick={() => setShowProjectMenu(!showProjectMenu)}
                    className="p-2 hover:bg-black/[0.02] rounded-lg transition-colors"
                  >
                    <MoreVertical className="w-4 h-4 text-black/40" />
                  </button>
                  {showProjectMenu && (
                    <>
                      <div className="fixed inset-0 z-40" onClick={() => setShowProjectMenu(false)} />
                      <div className="absolute right-0 top-full mt-1 bg-white rounded-lg shadow-lg border border-black/10 py-1 z-50 min-w-[160px]">
                        <button
                          onClick={() => {
                            setShowProjectMenu(false);
                            handleDeleteProject();
                          }}
                          className="w-full flex items-center gap-2 px-3 py-2 text-sm text-red-600 hover:bg-red-50 transition-colors"
                        >
                          <Trash2 className="w-4 h-4" />
                          Delete Project
                        </button>
                      </div>
                    </>
                  )}
                </div>
              </div>
            </div>
          </header>

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

                  <div className="flex-shrink-0 w-64">
                    <button
                      onClick={handleAddColumn}
                      className="w-full h-full min-h-[120px] rounded-lg flex flex-col items-center justify-center text-black hover:bg-black/5 transition-all"
                    >
                      <Plus className="w-5 h-5 mb-1" />
                      <span className="text-xs font-semibold">Add Column</span>
                    </button>
                  </div>
                </div>
              </DragDropContext>
            </div>
          )}

          {activeTab === 'members' && (
            <div className="flex-1 overflow-y-auto p-6">
              <div className="max-w-xl">
                <div className="mb-8">
                  <h2 className="text-sm font-normal text-black mb-4">Invite Member</h2>
                  <form onSubmit={handleInviteMember} className="flex gap-3">
                    <div className="flex-1 relative">
                      <Mail className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-black/40" />
                      <input
                        type="email"
                        value={inviteEmail}
                        onChange={(e) => setInviteEmail(e.target.value)}
                        placeholder="Enter email address"
                        className="w-full pl-10 pr-4 py-2.5 text-sm font-light bg-white border border-black/10 rounded-lg focus:outline-none focus:border-black transition-colors"
                        required
                      />
                    </div>
                    <button
                      type="submit"
                      disabled={inviteLoading}
                      className="px-5 py-2.5 bg-black text-white text-sm font-light hover:bg-black/90 transition-colors disabled:opacity-50"
                    >
                      {inviteLoading ? 'Inviting...' : 'Invite'}
                    </button>
                  </form>
                  {inviteMessage && (
                    <p
                      className={`mt-2 text-xs font-light ${inviteMessage.type === 'success' ? 'text-black' : 'text-red-600'}`}
                    >
                      {inviteMessage.text}
                    </p>
                  )}
                </div>

                <h2 className="text-sm font-normal text-black mb-4">Team Members</h2>
                <div className="space-y-2">
                  {members.map((member) => (
                    <div
                      key={member.id}
                      className="bg-white border border-black/10 rounded-lg p-4 flex items-center justify-between"
                    >
                      <div className="flex items-center gap-3">
                        <div className="w-8 h-8 bg-black/5 rounded-full flex items-center justify-center text-xs font-light text-black/60">
                          {member.name.charAt(0).toUpperCase()}
                        </div>
                        <div>
                          <p className="text-sm font-normal text-black">{member.name}</p>
                          <p className="text-xs font-light text-black/40">{member.email}</p>
                        </div>
                      </div>
                      <span
                        className={`text-xs font-light px-2 py-1 rounded ${
                          member.role === 'owner' ? 'bg-black text-white' : 'bg-black/5 text-black/60'
                        }`}
                      >
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
    </div>
  );
}
