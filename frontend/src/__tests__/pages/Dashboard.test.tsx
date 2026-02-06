import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { MemoryRouter } from 'react-router-dom';
import Dashboard from '@/pages/Dashboard';
import { AuthProvider } from '@/context/AuthContext';
import { ProjectsProvider } from '@/context/ProjectsContext';
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

describe('Dashboard Page', () => {
  const mockUser = createMockUser();

  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
    mockNavigate.mockClear();
  });

  describe('when user is authenticated with projects', () => {
    const projects = [
      createMockProject({ id: 'p1', name: 'Project Alpha', description: 'Alpha desc' }),
      createMockProject({ id: 'p2', name: 'Project Beta', description: undefined }),
    ];

    beforeEach(() => {
      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ user: mockUser }),
        } as Response)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ projects }),
        } as Response);
    });

    it('should display project cards', async () => {
      renderDashboard();
      await waitFor(() => {
        // Project names appear in sidebar and dashboard, so use getAllByText
        const alphaMatches = screen.getAllByText('Project Alpha');
        expect(alphaMatches.length).toBeGreaterThanOrEqual(1);
        const betaMatches = screen.getAllByText('Project Beta');
        expect(betaMatches.length).toBeGreaterThanOrEqual(1);
      });
    });

    it('should display project description or "No description"', async () => {
      renderDashboard();
      await waitFor(() => {
        expect(screen.getByText('Alpha desc')).toBeInTheDocument();
        expect(screen.getByText('No description')).toBeInTheDocument();
      });
    });

    it('should link project cards to /projects/:id', async () => {
      renderDashboard();
      await waitFor(() => {
        // Find the h3 with the project name (in dashboard cards) vs span (in sidebar)
        const alphaElements = screen.getAllByText('Project Alpha');
        const h3Element = alphaElements.find(el => el.tagName === 'H3');
        expect(h3Element).toBeTruthy();
        const cardLink = h3Element!.closest('a');
        expect(cardLink).toHaveAttribute('href', '/projects/p1');
      });
    });

    it('should display the New Project button', async () => {
      renderDashboard();
      await waitFor(() => {
        expect(screen.getByText('New Project')).toBeInTheDocument();
      });
    });

    it('should open project modal when New Project is clicked', async () => {
      const user = userEvent.setup();
      renderDashboard();

      await waitFor(() => {
        expect(screen.getByText('New Project')).toBeInTheDocument();
      });

      await user.click(screen.getByText('New Project'));

      await waitFor(() => {
        expect(screen.getByText('Create New Project')).toBeInTheDocument();
      });
    });
  });

  describe('when user is authenticated with no projects', () => {
    beforeEach(() => {
      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ user: mockUser }),
        } as Response)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ projects: [] }),
        } as Response);
    });

    it('should display empty state message', async () => {
      renderDashboard();
      await waitFor(() => {
        expect(screen.getByText('No projects yet')).toBeInTheDocument();
        expect(screen.getByText('Create your first project to get started')).toBeInTheDocument();
      });
    });

    it('should display Create Project button in empty state', async () => {
      renderDashboard();
      await waitFor(() => {
        expect(screen.getByText('Create Project')).toBeInTheDocument();
      });
    });

    it('should open project modal when Create Project is clicked in empty state', async () => {
      const user = userEvent.setup();
      renderDashboard();

      await waitFor(() => {
        expect(screen.getByText('Create Project')).toBeInTheDocument();
      });

      await user.click(screen.getByText('Create Project'));

      await waitFor(() => {
        expect(screen.getByText('Create New Project')).toBeInTheDocument();
      });
    });
  });

  describe('loading state', () => {
    it('should show loading spinner initially', () => {
      vi.mocked(fetch).mockReturnValue(new Promise(() => {}));

      renderDashboard();

      const spinner = document.querySelector('.animate-spin');
      expect(spinner).toBeInTheDocument();
    });
  });

  describe('project creation flow', () => {
    it('should call API and navigate on successful project creation', async () => {
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
        expect(screen.getByText('Create Project')).toBeInTheDocument();
      });

      // Open modal
      await user.click(screen.getByText('Create Project'));

      await waitFor(() => {
        expect(screen.getByPlaceholderText('Enter project name')).toBeInTheDocument();
      });

      // Setup create project response
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ id: 'new-project-id' }),
      } as Response);

      // Also setup the refetch projects call
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () =>
          Promise.resolve({
            projects: [createMockProject({ id: 'new-project-id', name: 'New Project' })],
          }),
      } as Response);

      // Fill form and submit -- use the submit button inside the form (type=submit)
      await user.type(screen.getByPlaceholderText('Enter project name'), 'New Project');
      const submitButtons = screen.getAllByRole('button', { name: 'Create Project' });
      const formSubmitBtn = submitButtons.find(btn => btn.getAttribute('type') === 'submit');
      await user.click(formSubmitBtn!);

      await waitFor(() => {
        expect(mockNavigate).toHaveBeenCalledWith('/projects/new-project-id');
      });
    });

    it('should handle project creation failure gracefully', async () => {
      const user = userEvent.setup();
      const consoleSpy = vi.spyOn(console, 'error').mockImplementation(() => {});

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
        expect(screen.getByText('Create Project')).toBeInTheDocument();
      });

      await user.click(screen.getByText('Create Project'));

      await waitFor(() => {
        expect(screen.getByPlaceholderText('Enter project name')).toBeInTheDocument();
      });

      // Setup failed create response
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: false,
        status: 500,
        json: () => Promise.resolve({ error: 'Server error' }),
      } as unknown as Response);

      await user.type(screen.getByPlaceholderText('Enter project name'), 'Failing Project');
      const submitButtons = screen.getAllByRole('button', { name: 'Create Project' });
      const formSubmitBtn = submitButtons.find(btn => btn.getAttribute('type') === 'submit');
      await user.click(formSubmitBtn!);

      await waitFor(() => {
        expect(consoleSpy).toHaveBeenCalledWith('Failed to create project:', expect.any(Error));
      });
    });
  });
});
