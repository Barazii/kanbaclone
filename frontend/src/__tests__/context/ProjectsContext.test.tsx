import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor, act } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { ProjectsProvider, useProjects } from '@/context/ProjectsContext';
import { AuthProvider } from '@/context/AuthContext';
import { MemoryRouter } from 'react-router-dom';
import { createMockProject, createMockUser } from '../test-utils';

function ProjectsConsumer() {
  const { projects, loading, fetchProjects } = useProjects();
  return (
    <div>
      <span data-testid="loading">{loading.toString()}</span>
      <span data-testid="count">{projects.length}</span>
      <ul data-testid="project-list">
        {projects.map((p) => (
          <li key={p.id}>{p.name}</li>
        ))}
      </ul>
      <button data-testid="refresh-btn" onClick={fetchProjects}>
        Refresh
      </button>
    </div>
  );
}

function renderWithAuth(authenticated: boolean) {
  const mockUser = createMockUser();

  if (authenticated) {
    // checkAuth returns user, then projects endpoint
    vi.mocked(fetch)
      .mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ user: mockUser }),
      } as Response)
      .mockResolvedValueOnce({
        ok: true,
        json: () =>
          Promise.resolve({
            projects: [
              createMockProject({ id: 'p1', name: 'Alpha' }),
              createMockProject({ id: 'p2', name: 'Beta' }),
            ],
          }),
      } as Response);
  } else {
    // checkAuth fails
    vi.mocked(fetch).mockResolvedValueOnce({
      ok: false,
      status: 401,
      json: () => Promise.resolve({ error: 'Unauthorized' }),
    } as unknown as Response);
  }

  return render(
    <MemoryRouter>
      <AuthProvider>
        <ProjectsProvider>
          <ProjectsConsumer />
        </ProjectsProvider>
      </AuthProvider>
    </MemoryRouter>
  );
}

describe('ProjectsContext', () => {
  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
  });

  it('should fetch projects when user is authenticated', async () => {
    renderWithAuth(true);

    // Wait for both auth and projects to resolve
    await waitFor(() => {
      expect(screen.getByTestId('count').textContent).toBe('2');
    });

    expect(screen.getByText('Alpha')).toBeInTheDocument();
    expect(screen.getByText('Beta')).toBeInTheDocument();
  });

  it('should not fetch projects when user is not authenticated', async () => {
    renderWithAuth(false);

    await waitFor(() => {
      expect(screen.getByTestId('loading').textContent).toBe('false');
    });

    expect(screen.getByTestId('count').textContent).toBe('0');
  });

  it('should handle fetchProjects error gracefully', async () => {
    const mockUser = createMockUser();
    const consoleSpy = vi.spyOn(console, 'error').mockImplementation(() => {});

    vi.mocked(fetch)
      .mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ user: mockUser }),
      } as Response)
      .mockResolvedValueOnce({
        ok: false,
        status: 500,
        json: () => Promise.resolve({ error: 'Server error' }),
      } as unknown as Response);

    render(
      <MemoryRouter>
        <AuthProvider>
          <ProjectsProvider>
            <ProjectsConsumer />
          </ProjectsProvider>
        </AuthProvider>
      </MemoryRouter>
    );

    await waitFor(() => {
      expect(screen.getByTestId('loading').textContent).toBe('false');
    });

    expect(screen.getByTestId('count').textContent).toBe('0');
    expect(consoleSpy).toHaveBeenCalledWith('Failed to fetch projects:', expect.any(Error));
  });

  it('should handle null projects response', async () => {
    const mockUser = createMockUser();

    vi.mocked(fetch)
      .mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ user: mockUser }),
      } as Response)
      .mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ projects: null }),
      } as Response);

    render(
      <MemoryRouter>
        <AuthProvider>
          <ProjectsProvider>
            <ProjectsConsumer />
          </ProjectsProvider>
        </AuthProvider>
      </MemoryRouter>
    );

    await waitFor(() => {
      expect(screen.getByTestId('loading').textContent).toBe('false');
    });

    expect(screen.getByTestId('count').textContent).toBe('0');
  });

  it('should allow refetching projects', async () => {
    const mockUser = createMockUser();

    vi.mocked(fetch)
      .mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ user: mockUser }),
      } as Response)
      .mockResolvedValueOnce({
        ok: true,
        json: () =>
          Promise.resolve({ projects: [createMockProject({ id: 'p1', name: 'Alpha' })] }),
      } as Response);

    render(
      <MemoryRouter>
        <AuthProvider>
          <ProjectsProvider>
            <ProjectsConsumer />
          </ProjectsProvider>
        </AuthProvider>
      </MemoryRouter>
    );

    await waitFor(() => {
      expect(screen.getByTestId('count').textContent).toBe('1');
    });

    // Set up next fetch response for the refetch
    vi.mocked(fetch).mockResolvedValueOnce({
      ok: true,
      json: () =>
        Promise.resolve({
          projects: [
            createMockProject({ id: 'p1', name: 'Alpha' }),
            createMockProject({ id: 'p2', name: 'Gamma' }),
          ],
        }),
    } as Response);

    await act(async () => {
      await userEvent.click(screen.getByTestId('refresh-btn'));
    });

    await waitFor(() => {
      expect(screen.getByTestId('count').textContent).toBe('2');
    });
  });

});
