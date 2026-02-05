import { useEffect, useState, useRef } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '@/context/AuthContext';
import Sidebar from '@/components/Sidebar';
import { Project } from '@/types';
import { Send, Key, AlertCircle } from 'lucide-react';
import { api } from '@/api/client';

interface Message {
  role: 'user' | 'assistant';
  content: string;
}

export default function AIAssistant() {
  const { user, loading: authLoading } = useAuth();
  const navigate = useNavigate();
  const [projects, setProjects] = useState<Project[]>([]);
  const [loading, setLoading] = useState(true);

  const [apiKey, setApiKey] = useState('');
  const [apiKeyInput, setApiKeyInput] = useState('');
  const [messages, setMessages] = useState<Message[]>([]);
  const [input, setInput] = useState('');
  const [sending, setSending] = useState(false);
  const messagesEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!authLoading && !user) {
      navigate('/login');
    }
  }, [user, authLoading, navigate]);

  useEffect(() => {
    if (user) {
      fetchProjects();
      const saved = localStorage.getItem('openai_api_key');
      if (saved) setApiKey(saved);
    }
  }, [user]);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

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

  const saveApiKey = () => {
    const trimmed = apiKeyInput.trim();
    if (trimmed) {
      localStorage.setItem('openai_api_key', trimmed);
      setApiKey(trimmed);
      setApiKeyInput('');
    }
  };

  const clearApiKey = () => {
    localStorage.removeItem('openai_api_key');
    setApiKey('');
    setMessages([]);
  };

  const sendMessage = async () => {
    if (!input.trim() || sending) return;

    const userMessage: Message = { role: 'user', content: input.trim() };
    const newMessages = [...messages, userMessage];
    setMessages(newMessages);
    setInput('');
    setSending(true);

    try {
      const data = await api.post<{ message: string }>('/ai-chat', { messages: newMessages, apiKey });
      setMessages([...newMessages, { role: 'assistant', content: data.message }]);
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Unknown error';
      setMessages([...newMessages, { role: 'assistant', content: `Error: ${message}` }]);
    } finally {
      setSending(false);
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

  if (!apiKey) {
    return (
      <div className="flex h-screen bg-white overflow-hidden">
        <Sidebar projects={projects} />
        <main className="flex-1 overflow-y-auto p-4">
          <div className="h-full bg-white border border-black rounded-xl p-8">
            <div className="max-w-xl">
              <h1 className="text-lg font-normal text-black mb-8">AI Assistant</h1>

              <div className="mb-8">
                <h2 className="text-sm font-normal text-black uppercase tracking-wide mb-4">Setup</h2>
                <div className="bg-white border border-black rounded-lg p-5">
                  <div className="flex items-start gap-3 mb-5 p-3 bg-amber-50 border border-amber-200 rounded-lg">
                    <AlertCircle className="w-4 h-4 text-amber-600 mt-0.5 flex-shrink-0" />
                    <p className="text-xs font-light text-amber-800">
                      Your API key is stored locally in your browser and sent directly to OpenAI. It is
                      never stored on our servers.
                    </p>
                  </div>

                  <label className="block text-xs font-light text-gray-500 mb-1.5">OpenAI API Key</label>
                  <input
                    type="password"
                    value={apiKeyInput}
                    onChange={(e) => setApiKeyInput(e.target.value)}
                    onKeyDown={(e) => e.key === 'Enter' && saveApiKey()}
                    placeholder="sk-..."
                    className="w-full px-3 py-2.5 text-sm font-light bg-white border border-black rounded-lg focus:outline-none focus:border-black transition-colors text-black mb-3"
                  />
                  <p className="text-xs font-light text-gray-400 mb-4">
                    Get your API key from{' '}
                    <a
                      href="https://platform.openai.com/api-keys"
                      target="_blank"
                      rel="noopener noreferrer"
                      className="underline hover:text-gray-600"
                    >
                      platform.openai.com/api-keys
                    </a>
                  </p>
                  <button
                    onClick={saveApiKey}
                    disabled={!apiKeyInput.trim()}
                    className="px-5 py-2 bg-black text-white text-sm font-light hover:bg-gray-900 transition-colors disabled:opacity-50"
                  >
                    Save API Key
                  </button>
                </div>
              </div>
            </div>
          </div>
        </main>
      </div>
    );
  }

  return (
    <div className="flex h-screen bg-white overflow-hidden">
      <Sidebar projects={projects} />
      <main className="flex-1 flex flex-col p-4 overflow-hidden">
        <div className="flex-1 flex flex-col bg-white border border-black rounded-xl overflow-hidden">
          <div className="flex items-center justify-between px-6 py-4 border-b border-black">
            <h1 className="text-lg font-normal text-black">AI Assistant</h1>
            <button
              onClick={clearApiKey}
              className="flex items-center gap-1.5 text-xs font-light text-black hover:text-red-500 transition-colors"
              title="Remove API key"
            >
              <Key className="w-3.5 h-3.5" />
              Remove API Key
            </button>
          </div>

          <div className="flex-1 overflow-y-auto px-6 py-4 space-y-4">
            <div className="max-w-3xl mx-auto w-full h-full space-y-4">
              {messages.length === 0 && (
                <div className="flex items-center justify-center h-full">
                  <p className="text-sm font-light text-gray-400">Discuss your tasks with ChatGPT</p>
                </div>
              )}
              {messages.map((msg, i) => (
                <div key={i} className={`flex ${msg.role === 'user' ? 'justify-end' : 'justify-start'}`}>
                  <div
                    className={`max-w-[75%] px-4 py-2.5 rounded-lg text-sm font-light whitespace-pre-wrap ${
                      msg.role === 'user'
                        ? 'bg-black text-white'
                        : 'bg-white border border-black text-black'
                    }`}
                  >
                    {msg.content}
                  </div>
                </div>
              ))}
              {sending && (
                <div className="flex justify-start">
                  <div className="bg-white border border-black px-4 py-2.5 rounded-lg">
                    <div className="flex gap-1">
                      <span
                        className="w-1.5 h-1.5 bg-gray-400 rounded-full animate-bounce"
                        style={{ animationDelay: '0ms' }}
                      />
                      <span
                        className="w-1.5 h-1.5 bg-gray-400 rounded-full animate-bounce"
                        style={{ animationDelay: '150ms' }}
                      />
                      <span
                        className="w-1.5 h-1.5 bg-gray-400 rounded-full animate-bounce"
                        style={{ animationDelay: '300ms' }}
                      />
                    </div>
                  </div>
                </div>
              )}
              <div ref={messagesEndRef} />
            </div>
          </div>

          <div className="px-6 py-4 border-t border-black">
            <div className="max-w-3xl mx-auto w-full relative">
              <input
                type="text"
                value={input}
                onChange={(e) => setInput(e.target.value)}
                onKeyDown={(e) => e.key === 'Enter' && !e.shiftKey && sendMessage()}
                placeholder="Type a message..."
                disabled={sending}
                className="w-full px-4 py-2.5 pr-12 text-sm font-light bg-white border border-black rounded-lg focus:outline-none transition-colors text-black disabled:opacity-50"
              />
              <button
                onClick={sendMessage}
                disabled={!input.trim() || sending}
                className="absolute right-2 top-1/2 -translate-y-1/2 p-1.5 bg-black text-white rounded-md hover:bg-gray-900 transition-colors"
              >
                <Send className="w-4 h-4" />
              </button>
            </div>
          </div>
        </div>
      </main>
    </div>
  );
}
