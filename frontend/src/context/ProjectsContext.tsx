import { createContext, useContext, useState, useCallback, useEffect, ReactNode } from 'react';
import { Project } from '@/types';
import { useAuth } from './AuthContext';
import { api } from '@/api/client';

interface ProjectsContextType {
  projects: Project[];
  loading: boolean;
  fetchProjects: () => Promise<void>;
}

const ProjectsContext = createContext<ProjectsContextType | undefined>(undefined);

export function ProjectsProvider({ children }: { children: ReactNode }) {
  const { user } = useAuth();
  const [projects, setProjects] = useState<Project[]>([]);
  const [loading, setLoading] = useState(true);

  const fetchProjects = useCallback(async () => {
    try {
      const data = await api.get<{ projects: Project[] }>('/projects');
      setProjects(data.projects || []);
    } catch (error) {
      console.error('Failed to fetch projects:', error);
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    if (user) {
      fetchProjects();
    } else {
      setProjects([]);
      setLoading(false);
    }
  }, [user, fetchProjects]);

  return (
    <ProjectsContext.Provider value={{ projects, loading, fetchProjects }}>
      {children}
    </ProjectsContext.Provider>
  );
}

export function useProjects() {
  const context = useContext(ProjectsContext);
  if (context === undefined) {
    throw new Error('useProjects must be used within a ProjectsProvider');
  }
  return context;
}
