import { useEffect } from 'react';
import { useNavigate, Link } from 'react-router-dom';
import { useAuth } from '@/context/AuthContext';

export default function Landing() {
  const { user, loading } = useAuth();
  const navigate = useNavigate();

  useEffect(() => {
    if (!loading && user) {
      navigate('/dashboard');
    }
  }, [user, loading, navigate]);

  if (loading) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-white">
        <div className="animate-spin rounded-full h-8 w-8 border-t border-b border-black"></div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-white relative overflow-hidden">
      <div className="absolute inset-0 pointer-events-none">
        <div className="absolute top-0 left-1/4 w-px h-full bg-gray-200"></div>
        <div className="absolute top-0 left-2/4 w-px h-full bg-gray-200"></div>
        <div className="absolute top-0 left-3/4 w-px h-full bg-gray-200"></div>
        <div className="absolute top-1/4 left-0 w-full h-px bg-gray-200"></div>
        <div className="absolute top-2/4 left-0 w-full h-px bg-gray-200"></div>
        <div className="absolute top-3/4 left-0 w-full h-px bg-gray-200"></div>

        <div className="absolute top-20 right-20 w-32 h-32 border border-gray-300 rotate-45 opacity-50"></div>
        <div className="absolute top-40 right-32 w-16 h-16 border border-gray-300 rotate-12 opacity-40"></div>
        <div className="absolute bottom-32 left-16 w-24 h-24 border border-gray-300 opacity-40"></div>
        <div className="absolute bottom-48 left-32 w-8 h-8 bg-gray-200 rotate-45 opacity-60"></div>
        <div className="absolute top-1/3 left-12 w-20 h-px bg-gray-300 rotate-45 opacity-70"></div>
        <div className="absolute top-1/2 right-16 w-28 h-px bg-gray-300 -rotate-12 opacity-60"></div>
        <div className="absolute bottom-1/4 right-1/4 w-12 h-12 border border-gray-300 rounded-full opacity-40"></div>
      </div>

      <header className="fixed top-0 w-full bg-white/90 backdrop-blur-sm border-b border-gray-100 z-50">
        <div className="max-w-6xl mx-auto px-6 lg:px-8">
          <div className="flex justify-between items-center h-16">
            <div className="flex items-center gap-2.5">
              <img src="/logo.png" alt="Kanba" width={28} height={28} className="rounded" />
              <span className="font-normal text-lg tracking-tight">Kanba</span>
            </div>
            <div className="flex items-center gap-6">
              <Link
                to="/login"
                className="bg-black text-white px-4 py-2 text-sm font-light tracking-wide hover:bg-gray-900 transition-colors"
              >
                Log In
              </Link>
            </div>
          </div>
        </div>
      </header>

      <main className="pt-32 pb-24 relative z-10">
        <div className="max-w-6xl mx-auto px-6 lg:px-8">
          <div className="text-center max-w-2xl mx-auto">
            <h1 className="text-4xl sm:text-5xl font-extralight text-black tracking-tight leading-tight">
              Simple Platform to Help You
              <span className="font-normal"> Stay Organized and Productive </span>
              <span className="font-normal">in Building Your Projects</span>
            </h1>
          </div>

          <div id="features" className="mt-32 grid md:grid-cols-3 gap-16">
            <div className="text-center">
              <h3 className="text-sm font-normal text-black tracking-wide uppercase mb-3">
                Visual Management
              </h3>
              <p className="text-sm font-light text-black leading-relaxed">
                See your work clearly with intuitive boards. Drag and drop tasks to update status.
              </p>
            </div>
            <div className="text-center">
              <h3 className="text-sm font-normal text-black tracking-wide uppercase mb-3">Collaboration</h3>
              <p className="text-sm font-light text-black leading-relaxed">
                Work together seamlessly. Assign tasks, add comments, and keep everyone in sync.
              </p>
            </div>
            <div className="text-center">
              <h3 className="text-sm font-normal text-black tracking-wide uppercase mb-3">AI Assistant</h3>
              <p className="text-sm font-light text-black leading-relaxed">
                AI chat assistant is integrated so you don&apos;t switch between multiple chrome tabs.
              </p>
            </div>
          </div>
        </div>
      </main>

      <footer className="border-t border-gray-100 py-12 relative z-10">
        <div className="max-w-6xl mx-auto px-6 lg:px-8 text-center"></div>
      </footer>
    </div>
  );
}
