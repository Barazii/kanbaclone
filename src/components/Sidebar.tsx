'use client';

import Link from 'next/link';
import Image from 'next/image';
import { usePathname } from 'next/navigation';
import { useAuth } from '@/context/AuthContext';
import { 
  FolderOpen, 
  Settings, 
  Plus,
  ChevronDown,
  LogOut,
  PanelLeftClose,
  PanelLeft,
  FileText
} from 'lucide-react';
import { useState } from 'react';
import { Project } from '@/types';

interface SidebarProps {
  projects: Project[];
  onNewProject: () => void;
}

export default function Sidebar({ projects, onNewProject }: SidebarProps) {
  const pathname = usePathname();
  const { user, logout } = useAuth();
  const [showUserMenu, setShowUserMenu] = useState(false);
  const [collapsed, setCollapsed] = useState(false);

  const navItems = [
    { href: '/dashboard', icon: FolderOpen, label: 'Projects', expandable: true },
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

      {/* New Project Button */}
      <div className={`pb-4 ${collapsed ? 'flex justify-center' : 'px-3'}`}>
        <button
          onClick={onNewProject}
          className={`flex items-center justify-center bg-gray-900 text-white rounded-lg hover:bg-gray-800 transition-all duration-300 ${collapsed ? 'w-9 h-9' : 'w-full px-4 py-2.5 gap-2'}`}
        >
          <Plus className={`flex-shrink-0 ${collapsed ? 'w-5 h-5' : 'w-4 h-4'}`} />
          {!collapsed && (
            <span className="whitespace-nowrap">
              New Project
            </span>
          )}
        </button>
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
                      <FileText className="w-4 h-4 flex-shrink-0" />
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
        <div className="relative">
          <button 
            onClick={() => setShowUserMenu(!showUserMenu)}
            className={`flex items-center rounded-lg hover:bg-gray-50 transition-all duration-300 ${collapsed ? 'w-9 h-9 justify-center' : 'w-full p-2 gap-3'}`}
          >
            <div className={`bg-gray-200 rounded-full flex items-center justify-center text-sm font-medium flex-shrink-0 ${collapsed ? 'w-7 h-7' : 'w-8 h-8'}`}>
              {user?.name?.charAt(0).toUpperCase() || 'U'}
            </div>
            {!collapsed && (
              <>
                <div className="text-left flex-1">
                  <p className="text-sm font-medium text-gray-900 whitespace-nowrap">{user?.name || 'User'}</p>
                  <p className="text-xs text-gray-500 truncate">{user?.email || 'user@email.com'}</p>
                </div>
                <ChevronDown className="w-4 h-4 text-gray-400 flex-shrink-0" />
              </>
            )}
          </button>
          
          {showUserMenu && (
            <div className={`absolute bottom-full mb-2 bg-white rounded-lg shadow-lg py-1 z-50 ${collapsed ? 'left-0 w-40' : 'left-0 w-full'}`}>
              <button
                onClick={logout}
                className="w-full flex items-center gap-2 px-3 py-2 text-sm text-red-600 hover:bg-red-50"
              >
                <LogOut className="w-4 h-4" />
                <span>Sign out</span>
              </button>
            </div>
          )}
        </div>
      </div>
    </aside>
  );
}
