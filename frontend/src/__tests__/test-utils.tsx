import { ReactElement } from 'react';
import { render, RenderOptions } from '@testing-library/react';
import { MemoryRouter } from 'react-router-dom';
import { User, Project, Column, Task, ProjectMember } from '@/types';

// -------------------------------------------------------
// Mock data factories
// -------------------------------------------------------

export function createMockUser(overrides: Partial<User> = {}): User {
  return {
    id: 'user-1',
    email: 'test@example.com',
    name: 'Test User',
    created_at: '2024-01-01T00:00:00Z',
    ...overrides,
  };
}

export function createMockProject(overrides: Partial<Project> = {}): Project {
  return {
    id: 'project-1',
    name: 'Test Project',
    description: 'A test project description',
    icon: 'folder',
    owner_id: 'user-1',
    created_at: '2024-01-01T00:00:00Z',
    task_count: 5,
    member_count: 2,
    ...overrides,
  };
}

export function createMockColumn(overrides: Partial<Column> = {}): Column {
  return {
    id: 'col-1',
    project_id: 'project-1',
    name: 'To Do',
    position: 0,
    color: '#000000',
    task_count: 2,
    tasks: [],
    ...overrides,
  };
}

export function createMockTask(overrides: Partial<Task> = {}): Task {
  return {
    id: 'task-1',
    column_id: 'col-1',
    title: 'Test Task',
    description: 'A test task description',
    priority: 'medium',
    position: 0,
    created_at: '2024-01-01T00:00:00Z',
    ...overrides,
  };
}

export function createMockMember(overrides: Partial<ProjectMember> = {}): ProjectMember {
  return {
    id: 'member-1',
    user_id: 'user-1',
    name: 'Test User',
    email: 'test@example.com',
    role: 'owner',
    joined_at: '2024-01-01T00:00:00Z',
    ...overrides,
  };
}

// -------------------------------------------------------
// Custom render with router
// -------------------------------------------------------

interface CustomRenderOptions extends Omit<RenderOptions, 'wrapper'> {
  initialEntries?: string[];
}

export function renderWithRouter(
  ui: ReactElement,
  { initialEntries = ['/'], ...options }: CustomRenderOptions = {}
) {
  function Wrapper({ children }: { children: React.ReactNode }) {
    return <MemoryRouter initialEntries={initialEntries}>{children}</MemoryRouter>;
  }

  return render(ui, { wrapper: Wrapper, ...options });
}

// -------------------------------------------------------
// Global fetch mock helper
// -------------------------------------------------------

export function mockFetch(responses: Record<string, unknown>) {
  return (input: RequestInfo | URL, _init?: RequestInit) => {
    const url = typeof input === 'string' ? input : input.toString();
    for (const [pattern, data] of Object.entries(responses)) {
      if (url.includes(pattern)) {
        return Promise.resolve({
          ok: true,
          json: () => Promise.resolve(data),
        });
      }
    }
    return Promise.resolve({
      ok: false,
      status: 404,
      json: () => Promise.resolve({ error: 'Not found' }),
    });
  };
}

export function mockFetchError(status = 500, message = 'Server error') {
  return () =>
    Promise.resolve({
      ok: false,
      status,
      json: () => Promise.resolve({ error: message }),
    });
}

export function mockFetchNetworkError() {
  return () => Promise.reject(new Error('Network error'));
}
