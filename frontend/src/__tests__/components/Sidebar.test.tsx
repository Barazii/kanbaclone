import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import Sidebar from '@/components/Sidebar';
import { AuthProvider } from '@/context/AuthContext';
import { MemoryRouter } from 'react-router-dom';
import { createMockProject, createMockUser } from '../test-utils';

function renderSidebar(
  projects = [createMockProject()],
  initialEntries = ['/dashboard']
) {
  const mockUser = createMockUser();
  vi.mocked(fetch).mockResolvedValue({
    ok: true,
    json: () => Promise.resolve({ user: mockUser }),
  } as Response);

  return render(
    <MemoryRouter initialEntries={initialEntries}>
      <AuthProvider>
        <Sidebar projects={projects} />
      </AuthProvider>
    </MemoryRouter>
  );
}

describe('Sidebar', () => {
  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
  });

  describe('project list', () => {
    it('should render project names in the sidebar when expanded', () => {
      const projects = [
        createMockProject({ id: 'p1', name: 'Project Alpha' }),
        createMockProject({ id: 'p2', name: 'Project Beta' }),
      ];
      renderSidebar(projects);

      expect(screen.getByText('Project Alpha')).toBeInTheDocument();
      expect(screen.getByText('Project Beta')).toBeInTheDocument();
    });

    it('should not render project list when projects array is empty', () => {
      renderSidebar([]);
      expect(screen.getByText('Projects')).toBeInTheDocument();
      expect(screen.queryByText('Test Project')).not.toBeInTheDocument();
    });

    it('should link projects to /projects/:id', () => {
      const projects = [createMockProject({ id: 'p1', name: 'Alpha' })];
      renderSidebar(projects);

      const link = screen.getByText('Alpha').closest('a');
      expect(link).toHaveAttribute('href', '/projects/p1');
    });
  });

  describe('user info section', () => {
    it('should display user initial after auth resolves', async () => {
      renderSidebar();

      // Wait for the auth check to resolve and the user data to appear
      await waitFor(() => {
        expect(screen.getByText('Test User')).toBeInTheDocument();
      });

      // The avatar circle should show 'T' for 'Test User'
      const avatarEl = screen.getByText('T');
      expect(avatarEl).toBeInTheDocument();
    });

    it('should display user name after auth resolves', async () => {
      renderSidebar();

      await waitFor(() => {
        expect(screen.getByText('Test User')).toBeInTheDocument();
      });
    });

    it('should display user email after auth resolves', async () => {
      renderSidebar();

      await waitFor(() => {
        expect(screen.getByText('test@example.com')).toBeInTheDocument();
      });
    });

    it('should display fallback values when user is not yet loaded', () => {
      // Make auth check never resolve
      vi.mocked(fetch).mockReturnValue(new Promise(() => {}));

      render(
        <MemoryRouter>
          <AuthProvider>
            <Sidebar projects={[]} />
          </AuthProvider>
        </MemoryRouter>
      );

      expect(screen.getByText('U')).toBeInTheDocument();
      expect(screen.getByText('User')).toBeInTheDocument();
      expect(screen.getByText('user@email.com')).toBeInTheDocument();
    });
  });

  describe('collapse functionality', () => {
    it('should collapse sidebar when the toggle button is clicked', async () => {
      const user = userEvent.setup();
      renderSidebar();

      // The sidebar starts expanded with nav items visible
      expect(screen.getByText('Projects')).toBeInTheDocument();
      expect(screen.getByText('AI Assistant')).toBeInTheDocument();

      // Find all buttons. The collapse button is the first non-sign-out button.
      const buttons = screen.getAllByRole('button');
      const collapseBtn = buttons.find(btn => btn.getAttribute('title') !== 'Sign out');
      expect(collapseBtn).toBeTruthy();

      await user.click(collapseBtn!);

      // After collapsing, text labels should be hidden
      // The nav items text should no longer be visible
      expect(screen.queryByText('Projects')).not.toBeInTheDocument();
      expect(screen.queryByText('AI Assistant')).not.toBeInTheDocument();
      expect(screen.queryByText('Settings')).not.toBeInTheDocument();
    });

    it('should show Sign out button with title', () => {
      renderSidebar();
      expect(screen.getByTitle('Sign out')).toBeInTheDocument();
    });
  });

  describe('active state', () => {
    it('should highlight Projects link when on /dashboard', () => {
      renderSidebar([], ['/dashboard']);
      const projectsLink = screen.getByText('Projects').closest('a');
      expect(projectsLink).toHaveClass('bg-gray-100');
    });

    it('should highlight Settings link when on /settings', () => {
      renderSidebar([], ['/settings']);
      const settingsLink = screen.getByText('Settings').closest('a');
      expect(settingsLink).toHaveClass('bg-gray-100');
    });

    it('should highlight AI Assistant link when on /ai-assistant', () => {
      renderSidebar([], ['/ai-assistant']);
      const aiLink = screen.getByText('AI Assistant').closest('a');
      expect(aiLink).toHaveClass('bg-gray-100');
    });
  });
});
