import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor, act } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { MemoryRouter } from 'react-router-dom';
import { AuthProvider } from '@/context/AuthContext';
import { ProjectsProvider } from '@/context/ProjectsContext';
import Dashboard from '@/pages/Dashboard';
import { createMockUser, createMockProject } from '../test-utils';

const mockNavigate = vi.fn();

vi.mock('react-router-dom', async () => {
  const actual = await vi.importActual('react-router-dom');
  return {
    ...actual,
    useNavigate: () => mockNavigate,
  };
});

function renderDashboard() {
  return render(
    <MemoryRouter initialEntries={['/dashboard']}>
      <AuthProvider>
        <ProjectsProvider>
          <Dashboard />
        </ProjectsProvider>
      </AuthProvider>
    </MemoryRouter>
  );
}

describe('Project Workflow Integration', () => {
  const mockUser = createMockUser();

  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
    mockNavigate.mockClear();
  });

  describe('empty state to project creation', () => {
    it('should show empty state, open modal, create project, and navigate', async () => {
      const user = userEvent.setup();

      // Auth check + initial projects fetch (empty)
      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ user: mockUser }),
        } as Response)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ projects: [] }),
        } as Response);

      renderDashboard();

      // Wait for empty state
      await waitFor(() => {
        expect(screen.getByText('No projects yet')).toBeInTheDocument();
      });

      // Click Create Project
      await user.click(screen.getByText('Create Project'));

      // Modal should open
      await waitFor(() => {
        expect(screen.getByText('Create New Project')).toBeInTheDocument();
      });

      // Setup create project response
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ id: 'new-proj-id' }),
      } as Response);

      // Setup refetch projects
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () =>
          Promise.resolve({
            projects: [createMockProject({ id: 'new-proj-id', name: 'My First Project' })],
          }),
      } as Response);

      // Fill and submit form
      await user.type(screen.getByPlaceholderText('Enter project name'), 'My First Project');
      await user.type(
        screen.getByPlaceholderText("What's this project about?"),
        'A test project'
      );

      // Use the form's submit button (type=submit) to avoid duplicate "Create Project" buttons
      const submitButtons = screen.getAllByRole('button', { name: 'Create Project' });
      const formSubmitBtn = submitButtons.find(btn => btn.getAttribute('type') === 'submit');
      await act(async () => {
        await user.click(formSubmitBtn!);
      });

      // Should navigate to the new project
      await waitFor(() => {
        expect(mockNavigate).toHaveBeenCalledWith('/projects/new-proj-id');
      });

      // Verify API was called with correct data
      const createCall = vi.mocked(fetch).mock.calls.find(
        (call) => typeof call[0] === 'string' && call[0].includes('/projects')
          && call[1] && (call[1] as RequestInit).method === 'POST'
      );
      expect(createCall).toBeTruthy();
      const body = JSON.parse((createCall![1] as RequestInit).body as string);
      expect(body.name).toBe('My First Project');
      expect(body.description).toBe('A test project');
      expect(body.icon).toBe('folder');
    });
  });

  describe('dashboard with existing projects', () => {
    it('should display multiple projects and navigate on click', async () => {
      const projects = [
        createMockProject({ id: 'p1', name: 'Frontend', description: 'UI work' }),
        createMockProject({ id: 'p2', name: 'Backend', description: 'API work' }),
        createMockProject({ id: 'p3', name: 'DevOps', description: undefined }),
      ];

      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ user: mockUser }),
        } as Response)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ projects }),
        } as Response);

      renderDashboard();

      await waitFor(() => {
        // Project names appear in both sidebar and dashboard cards
        const frontendMatches = screen.getAllByText('Frontend');
        expect(frontendMatches.length).toBeGreaterThanOrEqual(1);
        const backendMatches = screen.getAllByText('Backend');
        expect(backendMatches.length).toBeGreaterThanOrEqual(1);
        const devopsMatches = screen.getAllByText('DevOps');
        expect(devopsMatches.length).toBeGreaterThanOrEqual(1);
      });

      // Check descriptions (unique to dashboard cards)
      expect(screen.getByText('UI work')).toBeInTheDocument();
      expect(screen.getByText('API work')).toBeInTheDocument();
      expect(screen.getByText('No description')).toBeInTheDocument();

      // Check that project cards are links - find the h3 (dashboard) not span (sidebar)
      const frontendElements = screen.getAllByText('Frontend');
      const h3Element = frontendElements.find(el => el.tagName === 'H3');
      expect(h3Element).toBeTruthy();
      const frontendLink = h3Element!.closest('a');
      expect(frontendLink).toHaveAttribute('href', '/projects/p1');
    });

    it('should show the New Project button when projects exist', async () => {
      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ user: mockUser }),
        } as Response)
        .mockResolvedValueOnce({
          ok: true,
          json: () =>
            Promise.resolve({ projects: [createMockProject()] }),
        } as Response);

      renderDashboard();

      await waitFor(() => {
        expect(screen.getByText('New Project')).toBeInTheDocument();
      });
    });
  });

  describe('modal interaction', () => {
    it('should close modal on Cancel without creating a project', async () => {
      const user = userEvent.setup();

      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ user: mockUser }),
        } as Response)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ projects: [] }),
        } as Response);

      renderDashboard();

      await waitFor(() => {
        expect(screen.getByText('No projects yet')).toBeInTheDocument();
      });

      // Open modal
      await user.click(screen.getByText('Create Project'));
      expect(screen.getByText('Create New Project')).toBeInTheDocument();

      // Cancel
      await user.click(screen.getByRole('button', { name: 'Cancel' }));

      // Modal should close
      await waitFor(() => {
        expect(screen.queryByText('Create New Project')).not.toBeInTheDocument();
      });

      // Empty state should still be visible
      expect(screen.getByText('No projects yet')).toBeInTheDocument();
    });
  });
});
