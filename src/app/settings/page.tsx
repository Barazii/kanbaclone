'use client';

import { useEffect, useState } from 'react';
import { useRouter } from 'next/navigation';
import { useAuth } from '@/context/AuthContext';
import Sidebar from '@/components/Sidebar';
import { Project } from '@/types';
import { User } from 'lucide-react';

export default function SettingsPage() {
  const { user, loading: authLoading } = useAuth();
  const router = useRouter();
  const [projects, setProjects] = useState<Project[]>([]);
  const [loading, setLoading] = useState(true);
  
  const [name, setName] = useState('');
  const [saving, setSaving] = useState(false);
  const [message, setMessage] = useState<{ type: 'success' | 'error'; text: string } | null>(null);

  useEffect(() => {
    if (!authLoading && !user) {
      router.push('/login');
    }
  }, [user, authLoading, router]);

  useEffect(() => {
    if (user) {
      setName(user.name || '');
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

  const handleSave = async () => {
    setSaving(true);
    setMessage(null);
    
    try {
      const res = await fetch('/api/auth/update', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name }),
      });
      
      if (res.ok) {
        setMessage({ type: 'success', text: 'Settings saved successfully' });
      } else {
        setMessage({ type: 'error', text: 'Failed to save settings' });
      }
    } catch (error) {
      setMessage({ type: 'error', text: 'Failed to save settings' });
    } finally {
      setSaving(false);
    }
  };

  if (authLoading || loading) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-white dark:bg-gray-900">
        <div className="animate-spin rounded-full h-6 w-6 border border-gray-200 border-t-black dark:border-gray-700 dark:border-t-white"></div>
      </div>
    );
  }

  if (!user) return null;

  return (
    <div className="flex h-screen bg-white dark:bg-gray-900">
      <Sidebar projects={projects} onNewProject={() => {}} />
      
      <main className="flex-1 overflow-y-auto p-4">
        <div className="h-full bg-gray-50 dark:bg-gray-800 rounded-xl p-8">
          <div className="max-w-xl">
            <h1 className="text-lg font-normal text-black dark:text-white mb-8">Settings</h1>
            
            {/* Profile Section */}
            <div className="mb-8">
              <h2 className="text-sm font-normal text-black dark:text-white uppercase tracking-wide mb-4">Profile</h2>
              <div className="bg-white dark:bg-gray-900 border border-gray-100 dark:border-gray-700 rounded-lg p-5">
                <div className="flex items-center gap-4 mb-6">
                  <div className="w-14 h-14 bg-gray-100 dark:bg-gray-700 rounded-full flex items-center justify-center">
                    <User className="w-6 h-6 text-gray-500 dark:text-gray-400" />
                  </div>
                  <div>
                    <p className="text-sm font-normal text-black dark:text-white">{user.name}</p>
                    <p className="text-xs font-light text-gray-500 dark:text-gray-400">{user.email}</p>
                  </div>
                </div>
                
                <div className="space-y-4">
                  <div>
                    <label className="block text-xs font-light text-gray-500 dark:text-gray-400 mb-1.5">Display Name</label>
                    <input
                      type="text"
                      value={name}
                      onChange={(e) => setName(e.target.value)}
                      className="w-full px-3 py-2.5 text-sm font-light bg-gray-50 dark:bg-gray-800 border border-gray-200 dark:border-gray-700 rounded-lg focus:outline-none focus:border-black dark:focus:border-white transition-colors text-black dark:text-white"
                    />
                  </div>
                  
                  <button
                    onClick={handleSave}
                    disabled={saving}
                    className="px-5 py-2 bg-black dark:bg-white text-white dark:text-black text-sm font-light hover:bg-gray-900 dark:hover:bg-gray-100 transition-colors disabled:opacity-50"
                  >
                    {saving ? 'Saving...' : 'Save Changes'}
                  </button>
                  
                  {message && (
                    <p className={`text-xs font-light ${message.type === 'success' ? 'text-green-600' : 'text-red-600'}`}>
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
