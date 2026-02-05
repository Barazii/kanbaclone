import { useEffect, useState } from 'react';
import { useNavigate, Link } from 'react-router-dom';
import { useAuth } from '@/context/AuthContext';
import { useProjects } from '@/context/ProjectsContext';
import Sidebar from '@/components/Sidebar';
import ProjectModal from '@/components/ProjectModal';
import { FolderOpen, Plus } from 'lucide-react';
import { api } from '@/api/client';

export default function Dashboard() {
  const { user, loading: authLoading } = useAuth();
  const { projects, loading: projectsLoading, fetchProjects } = useProjects();
  const navigate = useNavigate();
  const [showProjectModal, setShowProjectModal] = useState(false);

  useEffect(() => {
    if (!authLoading && !user) {
      navigate('/login');
    }
  }, [user, authLoading, navigate]);

  const loading = projectsLoading;

  const handleCreateProject = async (data: { name: string; description: string; icon: string }) => {
    try {
      const result = await api.post<{ id: string }>('/projects', data);
      setShowProjectModal(false);
      fetchProjects();
      navigate(`/projects/${result.id}`);
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

  if (!user) return null;

  return (
    <div className="flex h-screen bg-white">
      <Sidebar projects={projects} />

      <main className="flex-1 overflow-y-auto p-4">
        <div className="h-full bg-white border border-black rounded-xl p-8">
          <div className="max-w-5xl mx-auto">
            <div>
              <div className="flex justify-between items-center mb-6">
                <h2 className="text-base font-normal text-black tracking-wide">Your Projects</h2>
                {projects.length > 0 && (
                  <button
                    onClick={() => setShowProjectModal(true)}
                    className="flex items-center gap-2 bg-black text-white px-4 py-2 text-sm font-light tracking-wide hover:bg-black/90 transition-colors rounded-lg"
                  >
                    <Plus className="w-4 h-4" />
                    New Project
                  </button>
                )}
              </div>

              {projects.length === 0 ? (
                <div className="bg-white border border-black rounded-xl p-16 text-center">
                  <FolderOpen className="w-12 h-12 text-black mx-auto mb-4" />
                  <h3 className="text-base font-semibold text-black mb-2">No projects yet</h3>
                  <p className="text-sm font-medium text-black mb-6">
                    Create your first project to get started
                  </p>
                  <button
                    onClick={() => setShowProjectModal(true)}
                    className="inline-flex items-center gap-2 bg-black text-white px-5 py-2.5 text-sm font-light tracking-wide hover:bg-black/90 transition-colors"
                  >
                    <Plus className="w-4 h-4" />
                    Create Project
                  </button>
                </div>
              ) : (
                <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
                  {projects.map((project) => (
                    <Link
                      key={project.id}
                      to={`/projects/${project.id}`}
                      className="bg-white border border-black rounded-xl p-5 hover:shadow-md transition-all group"
                    >
                      <div className="flex items-center gap-3 mb-3">
                        <div className="w-10 h-10 bg-white border border-black rounded-lg flex items-center justify-center group-hover:bg-black/5 transition-colors">
                          <FolderOpen className="w-5 h-5 text-black" />
                        </div>
                        <div className="flex-1">
                          <h3 className="text-sm font-semibold text-black">{project.name}</h3>
                          <p className="text-xs font-medium text-black">
                            Created {new Date(project.created_at).toLocaleDateString()}
                          </p>
                        </div>
                      </div>
                      <p className="text-xs font-normal text-black line-clamp-2">
                        {project.description || 'No description'}
                      </p>
                    </Link>
                  ))}
                </div>
              )}
            </div>
          </div>
        </div>
      </main>

      {showProjectModal && (
        <ProjectModal onClose={() => setShowProjectModal(false)} onSave={handleCreateProject} />
      )}
    </div>
  );
}
