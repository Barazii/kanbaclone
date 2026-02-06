import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import { createMockUser } from './test-utils';

// We cannot use BrowserRouter in tests, so we mock react-router-dom's BrowserRouter
// and replace it with MemoryRouter. The App component uses BrowserRouter internally.
vi.mock('react-router-dom', async () => {
  const actual = await vi.importActual('react-router-dom');
  return {
    ...actual,
    BrowserRouter: ({ children }: { children: React.ReactNode }) => <>{children}</>,
  };
});

// We need to import App after mocking
import App from '@/App';
import { MemoryRouter } from 'react-router-dom';

function renderApp(initialEntries: string[] = ['/']) {
  return render(
    <MemoryRouter initialEntries={initialEntries}>
      <App />
    </MemoryRouter>
  );
}

describe('App routing', () => {
  const mockUser = createMockUser();

  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
  });

  describe('public routes when unauthenticated', () => {
    beforeEach(() => {
      vi.mocked(fetch).mockResolvedValue({
        ok: false,
        status: 401,
        json: () => Promise.resolve({ error: 'Unauthorized' }),
      } as unknown as Response);
    });

    it('should render landing page at /', async () => {
      renderApp(['/']);

      await waitFor(() => {
        expect(screen.getByText(/Simple Platform to Help You/)).toBeInTheDocument();
      });
    });

    it('should render login page at /login', async () => {
      renderApp(['/login']);

      await waitFor(() => {
        expect(screen.getByText('Sign in to your account')).toBeInTheDocument();
      });
    });

    it('should render register page at /register', async () => {
      renderApp(['/register']);

      await waitFor(() => {
        expect(screen.getByText('Create your account')).toBeInTheDocument();
      });
    });
  });

  describe('protected routes when unauthenticated', () => {
    beforeEach(() => {
      vi.mocked(fetch).mockResolvedValue({
        ok: false,
        status: 401,
        json: () => Promise.resolve({ error: 'Unauthorized' }),
      } as unknown as Response);
    });

    it('should redirect /dashboard to /login when not authenticated', async () => {
      renderApp(['/dashboard']);

      await waitFor(() => {
        expect(screen.getByText('Sign in to your account')).toBeInTheDocument();
      });
    });

    it('should redirect /settings to /login when not authenticated', async () => {
      renderApp(['/settings']);

      await waitFor(() => {
        expect(screen.getByText('Sign in to your account')).toBeInTheDocument();
      });
    });

    it('should redirect /ai-assistant to /login when not authenticated', async () => {
      renderApp(['/ai-assistant']);

      await waitFor(() => {
        expect(screen.getByText('Sign in to your account')).toBeInTheDocument();
      });
    });

    it('should redirect /projects/:id to /login when not authenticated', async () => {
      renderApp(['/projects/some-id']);

      await waitFor(() => {
        expect(screen.getByText('Sign in to your account')).toBeInTheDocument();
      });
    });
  });

  describe('public routes when authenticated', () => {
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

    it('should redirect / to /dashboard when authenticated', async () => {
      renderApp(['/']);

      await waitFor(() => {
        // Dashboard shows "Your Projects"
        expect(screen.getByText('Your Projects')).toBeInTheDocument();
      });
    });

    it('should redirect /login to /dashboard when authenticated', async () => {
      renderApp(['/login']);

      await waitFor(() => {
        expect(screen.getByText('Your Projects')).toBeInTheDocument();
      });
    });

    it('should redirect /register to /dashboard when authenticated', async () => {
      renderApp(['/register']);

      await waitFor(() => {
        expect(screen.getByText('Your Projects')).toBeInTheDocument();
      });
    });
  });

  describe('loading state', () => {
    it('should show loading spinner while checking auth', () => {
      vi.mocked(fetch).mockReturnValue(new Promise(() => {}));

      renderApp(['/dashboard']);

      const spinner = document.querySelector('.animate-spin');
      expect(spinner).toBeInTheDocument();
    });
  });

  describe('fallback route', () => {
    it('should redirect unknown routes to /', async () => {
      vi.mocked(fetch).mockResolvedValue({
        ok: false,
        status: 401,
        json: () => Promise.resolve({ error: 'Unauthorized' }),
      } as unknown as Response);

      renderApp(['/nonexistent-page']);

      await waitFor(() => {
        // Should redirect to / which shows the landing page
        expect(screen.getByText(/Simple Platform to Help You/)).toBeInTheDocument();
      });
    });
  });
});
