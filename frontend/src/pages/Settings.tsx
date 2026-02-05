import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '@/context/AuthContext';
import Sidebar from '@/components/Sidebar';
import { Project } from '@/types';
import { User } from 'lucide-react';
import { api } from '@/api/client';

export default function Settings() {
  const { user, loading: authLoading } = useAuth();
  const navigate = useNavigate();
  const [projects, setProjects] = useState<Project[]>([]);
  const [loading, setLoading] = useState(true);

  const [name, setName] = useState('');
  const [saving, setSaving] = useState(false);
  const [message, setMessage] = useState<{ type: 'success' | 'error'; text: string } | null>(null);

  useEffect(() => {
    if (!authLoading && !user) {
      navigate('/login');
    }
  }, [user, authLoading, navigate]);

  useEffect(() => {
    if (user) {
      setName(user.name || '');
      fetchProjects();
    }
  }, [user]);

  const fetchProjects = async () => {
    try {
      const data = await api.get<{ projects: Project[] }>('/projects');
      setProjects(data.projects || []);
    } catch (error) {
      console.error('Failed to fetch projects:', error);
    } finally {
      setLoading(false);
    }
  };

  const handleSave = async () => {
    setSaving(true);
    setMessage(null);

    try {
      await api.put('/auth/update', { name });
      setMessage({ type: 'success', text: 'Settings saved successfully' });
    } catch {
      setMessage({ type: 'error', text: 'Failed to save settings' });
    } finally {
      setSaving(false);
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
          <div className="max-w-xl">
            <h1 className="text-lg font-normal text-black mb-8">Settings</h1>

            <div className="mb-8">
              <h2 className="text-sm font-normal text-black uppercase tracking-wide mb-4">Profile</h2>
              <div className="bg-white border border-black rounded-lg p-5">
                <div className="flex items-center gap-4 mb-6">
                  <div className="w-14 h-14 bg-gray-100 rounded-full flex items-center justify-center">
                    <User className="w-6 h-6 text-gray-500" />
                  </div>
                  <div>
                    <p className="text-sm font-normal text-black">{user.name}</p>
                    <p className="text-xs font-light text-gray-500">{user.email}</p>
                  </div>
                </div>

                <div className="space-y-4">
                  <div>
                    <label className="block text-xs font-light text-gray-500 mb-1.5">Display Name</label>
                    <input
                      type="text"
                      value={name}
                      onChange={(e) => setName(e.target.value)}
                      className="w-full px-3 py-2.5 text-sm font-light bg-white border border-black rounded-lg focus:outline-none focus:border-black transition-colors text-black"
                    />
                  </div>

                  <button
                    onClick={handleSave}
                    disabled={saving}
                    className="px-5 py-2 bg-black text-white text-sm font-light hover:bg-gray-900 transition-colors disabled:opacity-50"
                  >
                    {saving ? 'Saving...' : 'Save Changes'}
                  </button>

                  {message && (
                    <p
                      className={`text-xs font-light ${message.type === 'success' ? 'text-green-600' : 'text-red-600'}`}
                    >
                      {message.text}
                    </p>
                  )}
                </div>
              </div>
            </div>
          </div>
        </div>
      </main>
    </div>
  );
}
