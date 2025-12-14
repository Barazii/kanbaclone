'use client';

import { useEffect, useState } from 'react';
import { useRouter } from 'next/navigation';
import { useAuth } from '@/context/AuthContext';
import Sidebar from '@/components/Sidebar';
import ProjectModal from '@/components/ProjectModal';
import { Project } from '@/types';
import { FolderOpen, Plus } from 'lucide-react';
import Link from 'next/link';

export default function DashboardPage() {
  const { user, loading: authLoading } = useAuth();
  const router = useRouter();
  const [projects, setProjects] = useState<Project[]>([]);
  const [loading, setLoading] = useState(true);
  const [showProjectModal, setShowProjectModal] = useState(false);

  useEffect(() => {
    if (!authLoading && !user) {
      router.push('/login');
    }
  }, [user, authLoading, router]);

  useEffect(() => {
    if (user) {
      fetchProjects();
    }
  }, [user]);

  const fetchProjects = async () => {
    try {
      const res = await fetch('/api/projects');
      if (res.ok) {
        const data = await res.json();
        setProjects(data.projects || []);
      }
    } catch (error) {
      console.error('Failed to fetch projects:', error);
    } finally {
      setLoading(false);
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

  if (!user) return null;

  return (
    <div className="flex h-screen bg-white">
      <Sidebar projects={projects} onNewProject={() => setShowProjectModal(true)} />
      
      <main className="flex-1 overflow-y-auto p-4">
        <div className="h-full bg-gray-50 rounded-xl p-8">
          <div className="max-w-5xl mx-auto">
            {/* Projects Section */}
            <div>
              <div className="flex justify-between items-center mb-6">
                <h2 className="text-base font-normal text-black tracking-wide">Your Projects</h2>
              </div>

            {projects.length === 0 ? (
              <div className="bg-white border border-gray-100 rounded-xl p-16 text-center">
                <FolderOpen className="w-12 h-12 text-gray-300 mx-auto mb-4" />
                <h3 className="text-base font-normal text-black mb-2">No projects yet</h3>
                <p className="text-sm font-light text-gray-500 mb-6">Create your first project to get started</p>
                <button
                  onClick={() => setShowProjectModal(true)}
                  className="inline-flex items-center gap-2 bg-black text-white px-5 py-2.5 text-sm font-light tracking-wide hover:bg-gray-900 transition-colors"
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
                    href={`/projects/${project.id}`}
                    className="bg-white border border-gray-100 rounded-xl p-5 hover:border-gray-300 hover:shadow-sm transition-all group"
                  >
                    <div className="flex items-center gap-3 mb-3">
                      <div className="w-10 h-10 bg-gray-50 border border-gray-100 rounded-lg flex items-center justify-center group-hover:bg-gray-100 transition-colors">
                        <FolderOpen className="w-5 h-5 text-gray-500" />
                      </div>
                      <div className="flex-1">
                        <h3 className="text-sm font-normal text-black">{project.name}</h3>
                        <p className="text-xs font-light text-gray-400">Created {new Date(project.created_at).toLocaleDateString()}</p>
                      </div>
                    </div>
                    <p className="text-xs font-light text-gray-500 line-clamp-2">
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
        <ProjectModal
          onClose={() => setShowProjectModal(false)}
          onSave={handleCreateProject}
        />
      )}
    </div>
  );
}
