'use client';

import { useEffect } from 'react';
import { useRouter } from 'next/navigation';
import { useAuth } from '@/context/AuthContext';
import Link from 'next/link';
import Image from 'next/image';


export default function Home() {
  const { user, loading } = useAuth();
  const router = useRouter();

  useEffect(() => {
    if (!loading && user) {
      router.push('/dashboard');
    }
  }, [user, loading, router]);

  if (loading) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-white">
        <div className="animate-spin rounded-full h-8 w-8 border-t border-b border-black"></div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-white relative overflow-hidden">
      {/* Background geometric pattern */}
      <div className="absolute inset-0 pointer-events-none">
        {/* Grid lines */}
        <div className="absolute top-0 left-1/4 w-px h-full bg-gray-100"></div>
        <div className="absolute top-0 left-2/4 w-px h-full bg-gray-100"></div>
        <div className="absolute top-0 left-3/4 w-px h-full bg-gray-100"></div>
        <div className="absolute top-1/4 left-0 w-full h-px bg-gray-100"></div>
        <div className="absolute top-2/4 left-0 w-full h-px bg-gray-100"></div>
        <div className="absolute top-3/4 left-0 w-full h-px bg-gray-100"></div>
        
        {/* Decorative shapes */}
        <div className="absolute top-20 right-20 w-32 h-32 border border-gray-200 rotate-45 opacity-40"></div>
        <div className="absolute top-40 right-32 w-16 h-16 border border-gray-200 rotate-12 opacity-30"></div>
        <div className="absolute bottom-32 left-16 w-24 h-24 border border-gray-200 opacity-30"></div>
        <div className="absolute bottom-48 left-32 w-8 h-8 bg-gray-100 rotate-45 opacity-50"></div>
        <div className="absolute top-1/3 left-12 w-20 h-px bg-gray-200 rotate-45 opacity-60"></div>
        <div className="absolute top-1/2 right-16 w-28 h-px bg-gray-200 -rotate-12 opacity-50"></div>
        <div className="absolute bottom-1/4 right-1/4 w-12 h-12 border border-gray-200 rounded-full opacity-30"></div>
      </div>

      {/* Header */}
      <header className="fixed top-0 w-full bg-white/90 backdrop-blur-sm border-b border-gray-100 z-50">
        <div className="max-w-6xl mx-auto px-6 lg:px-8">
          <div className="flex justify-between items-center h-16">
            <div className="flex items-center gap-2.5">
              <Image src="/logo.png" alt="Kanba" width={28} height={28} className="rounded" unoptimized />
              <span className="font-normal text-lg tracking-tight">Kanba</span>
            </div>
            <div className="flex items-center gap-6">
              <Link
                href="/login"
                className="text-gray-500 hover:text-black text-sm font-light transition-colors"
              >
                Log in
              </Link>
              <Link
                href="/register"
                className="bg-black text-white px-4 py-2 text-sm font-light tracking-wide hover:bg-gray-900 transition-colors"
              >
                Get Started
              </Link>
            </div>
          </div>
        </div>
      </header>

      {/* Hero Section */}
      <main className="pt-32 pb-24 relative z-10">
        <div className="max-w-6xl mx-auto px-6 lg:px-8">
          <div className="text-center max-w-2xl mx-auto">
            <h1 className="text-4xl sm:text-5xl font-extralight text-black tracking-tight leading-tight">
              Simple Platform to Help You
              <span className="font-normal"> Stay Organized and Productive </span>
              <span className="font-normal">in Building Your Projects</span>
            </h1>
            <p className="mt-8 text-base font-light text-gray-500 leading-relaxed">
            </p>

          </div>

          {/* Features */}
          <div id="features" className="mt-32 grid md:grid-cols-3 gap-16">
            <div className="text-center">
              <h3 className="text-sm font-normal text-black tracking-wide uppercase mb-3">Visual Management</h3>
              <p className="text-sm font-light text-gray-500 leading-relaxed">
                See your work clearly with intuitive Kanban boards. Drag and drop tasks between columns.
              </p>
            </div>
            <div className="text-center">
              <h3 className="text-sm font-normal text-black tracking-wide uppercase mb-3">Collaboration</h3>
              <p className="text-sm font-light text-gray-500 leading-relaxed">
                Work together seamlessly. Assign tasks, add comments, and keep everyone in sync.
              </p>
            </div>
            <div className="text-center">
              <h3 className="text-sm font-normal text-black tracking-wide uppercase mb-3">Open Source</h3>
              <p className="text-sm font-light text-gray-500 leading-relaxed">
                Totally free and open source. Self-host for complete control and privacy.
              </p>
            </div>
          </div>
        </div>
      </main>

      {/* Footer */}
      <footer className="border-t border-gray-100 py-12 relative z-10">
        <div className="max-w-6xl mx-auto px-6 lg:px-8 text-center">
          <p className="text-xs font-light text-gray-400 tracking-wide">© 2024 Kanba</p>
        </div>
      </footer>
    </div>
  );
}
