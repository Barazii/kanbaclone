'use client';

import Link from 'next/link';
import Image from 'next/image';
import { usePathname } from 'next/navigation';
import { useAuth } from '@/context/AuthContext';
import { 
  Folder,
  FolderOpen, 
  Settings, 
  LogOut,
  PanelLeftClose,
  PanelLeft
} from 'lucide-react';
import { useState } from 'react';
import { Project } from '@/types';

interface SidebarProps {
  projects: Project[];
}

export default function Sidebar({ projects }: SidebarProps) {
  const pathname = usePathname();
  const { user, logout } = useAuth();
  const [collapsed, setCollapsed] = useState(false);

  const navItems = [
    { href: '/dashboard', icon: Folder, label: 'Projects', expandable: true },
    { href: '/settings', icon: Settings, label: 'Settings' },
  ];

  return (
    <aside className={`${collapsed ? 'w-16' : 'w-64'} bg-white/80 backdrop-blur-sm flex flex-col h-screen transition-all duration-300 ease-in-out`}>
      {/* Logo */}
      <div className={`flex items-center min-h-[56px] ${collapsed ? 'justify-center px-0' : 'justify-between p-3'}`}>
        <button
          onClick={() => collapsed && setCollapsed(false)}
          className={`flex items-center gap-3 hover:bg-gray-100 rounded-lg p-1 transition-colors select-none ${collapsed ? 'justify-center' : ''}`}
        >
          <Image src="/logo.png" alt="Kanba" width={28} height={28} className="rounded-lg flex-shrink-0 pointer-events-none" draggable={false} unoptimized />
          {!collapsed && (
            <span className="font-semibold text-lg whitespace-nowrap select-none">
              Kanba
            </span>
          )}
        </button>
        {!collapsed && (
          <button
            onClick={() => setCollapsed(true)}
            className="p-1.5 hover:bg-gray-100 rounded-lg transition-colors flex-shrink-0"
          >
            <PanelLeftClose className="w-4 h-4 text-gray-500" />
          </button>
        )}
      </div>

      {/* Navigation */}
      <nav className={`flex-1 py-2 overflow-y-auto overflow-x-hidden ${collapsed ? 'flex flex-col items-center' : 'px-2'}`}>
        {navItems.map((item) => {
          const isActive = pathname === item.href || pathname.startsWith(item.href + '/');
          const Icon = item.icon;
          
          return (
            <div key={item.label} className={collapsed ? 'flex justify-center w-full' : 'w-full'}>
              <Link
                href={item.href}
                className={`flex items-center rounded-lg mb-1 transition-all duration-300 ${
                  collapsed ? 'justify-center w-9 h-9' : 'gap-3 px-3 py-2'
                } ${
                  isActive 
                    ? 'bg-gray-100 text-black' 
                    : 'text-gray-500 hover:bg-gray-50 hover:text-black'
                }`}
              >
                <Icon className="w-5 h-5 flex-shrink-0" />
                {!collapsed && (
                  <span className="text-sm font-light whitespace-nowrap flex-1">
                    {item.label}
                  </span>
                )}
              </Link>
              
              {/* Project sub-items */}
              {item.label === 'Projects' && !collapsed && (
                <div className={`ml-4 pl-3 py-1 overflow-hidden transition-all duration-300 ${collapsed ? 'h-0 opacity-0' : 'h-auto opacity-100'}`}>
                  {projects.map((project) => (
                    <Link
                      key={project.id}
                      href={`/projects/${project.id}`}
                      className={`flex items-center gap-2 px-2 py-1.5 rounded text-sm transition-colors ${
                        pathname === `/projects/${project.id}`
                          ? 'bg-gray-100 text-gray-900'
                          : 'text-gray-500 hover:text-gray-700 hover:bg-gray-50'
                      }`}
                    >
                      <FolderOpen className="w-4 h-4 flex-shrink-0" />
                      <span className="truncate">{project.name}</span>
                    </Link>
                  ))}
                </div>
              )}
            </div>
          );
        })}
      </nav>

      {/* User Profile */}
      <div className={`${collapsed ? 'py-3 flex justify-center' : 'p-3'}`}>
        <div className={`flex items-center ${collapsed ? '' : 'gap-2'}`}>
          <div className={`bg-black/10 rounded-full flex items-center justify-center text-sm font-medium flex-shrink-0 ${collapsed ? 'w-7 h-7' : 'w-8 h-8'}`}>
            {user?.name?.charAt(0).toUpperCase() || 'U'}
          </div>
          {!collapsed && (
            <>
              <div className="text-left flex-1 min-w-0">
                <p className="text-sm font-medium text-black whitespace-nowrap">{user?.name || 'User'}</p>
                <p className="text-xs text-black/50 truncate">{user?.email || 'user@email.com'}</p>
              </div>
              <button
                onClick={logout}
                className="p-1.5 hover:bg-black/5 rounded-lg transition-colors flex-shrink-0"
                title="Sign out"
              >
                <LogOut className="w-4 h-4 text-black/40 hover:text-black" />
              </button>
            </>
          )}
        </div>
      </div>
    </aside>
  );
}
