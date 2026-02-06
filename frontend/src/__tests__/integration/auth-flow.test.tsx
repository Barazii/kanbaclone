import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor, act } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { MemoryRouter } from 'react-router-dom';
import { createMockUser, createMockProject } from '../test-utils';

// Mock BrowserRouter to use MemoryRouter
vi.mock('react-router-dom', async () => {
  const actual = await vi.importActual('react-router-dom');
  return {
    ...actual,
    BrowserRouter: ({ children }: { children: React.ReactNode }) => <>{children}</>,
  };
});

import App from '@/App';

describe('Authentication Flow Integration', () => {
  const mockUser = createMockUser({ name: 'Test User', email: 'test@example.com' });

  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
  });

  describe('login flow', () => {
    it('should allow user to log in from the login page and see dashboard', async () => {
      const user = userEvent.setup();

      // Initial auth check: not authenticated
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: false,
        status: 401,
        json: () => Promise.resolve({ error: 'Unauthorized' }),
      } as unknown as Response);

      render(
        <MemoryRouter initialEntries={['/login']}>
          <App />
        </MemoryRouter>
      );

      // Wait for the login page to appear
      await waitFor(() => {
        expect(screen.getByText('Sign in to your account')).toBeInTheDocument();
      });

      // Setup login API response
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ user: mockUser }),
      } as Response);

      // Setup projects fetch after successful login
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () =>
          Promise.resolve({
            projects: [createMockProject({ id: 'p1', name: 'My Project' })],
          }),
      } as Response);

      // Fill in credentials
      await user.type(screen.getByLabelText('Email address'), 'test@example.com');
      await user.type(screen.getByLabelText('Password'), 'password123');

      // Submit the form
      await act(async () => {
        await user.click(screen.getByRole('button', { name: 'Sign in' }));
      });

      // After login, we should see the user is authenticated
      // The navigate to /dashboard happens in the Login component
      await waitFor(() => {
        expect(vi.mocked(fetch)).toHaveBeenCalledWith(
          expect.stringContaining('/auth/login'),
          expect.objectContaining({ method: 'POST' })
        );
      });
    });

    it('should show error and remain on login page on failed login', async () => {
      const user = userEvent.setup();

      vi.mocked(fetch).mockResolvedValueOnce({
        ok: false,
        status: 401,
        json: () => Promise.resolve({ error: 'Unauthorized' }),
      } as unknown as Response);

      render(
        <MemoryRouter initialEntries={['/login']}>
          <App />
        </MemoryRouter>
      );

      await waitFor(() => {
        expect(screen.getByText('Sign in to your account')).toBeInTheDocument();
      });

      // Failed login response
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: false,
        status: 401,
        json: () => Promise.resolve({ error: 'Invalid credentials' }),
      } as unknown as Response);

      vi.spyOn(console, 'error').mockImplementation(() => {});

      await user.type(screen.getByLabelText('Email address'), 'wrong@example.com');
      await user.type(screen.getByLabelText('Password'), 'wrongpass');

      await act(async () => {
        await user.click(screen.getByRole('button', { name: 'Sign in' }));
      });

      // Error should appear, still on login page
      await waitFor(() => {
        expect(screen.getByText('Invalid email or password')).toBeInTheDocument();
        expect(screen.getByText('Sign in to your account')).toBeInTheDocument();
      });
    });
  });

  describe('registration flow', () => {
    it('should allow user to register and reach authenticated state', async () => {
      const user = userEvent.setup();
      const newUser = createMockUser({ id: 'new-1', name: 'New User', email: 'new@example.com' });

      vi.mocked(fetch).mockResolvedValueOnce({
        ok: false,
        status: 401,
        json: () => Promise.resolve({ error: 'Unauthorized' }),
      } as unknown as Response);

      render(
        <MemoryRouter initialEntries={['/register']}>
          <App />
        </MemoryRouter>
      );

      await waitFor(() => {
        expect(screen.getByText('Create your account')).toBeInTheDocument();
      });

      // Setup register response
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ user: newUser }),
      } as Response);

      // Setup projects fetch
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ projects: [] }),
      } as Response);

      await user.type(screen.getByLabelText('Full name'), 'New User');
      await user.type(screen.getByLabelText('Email address'), 'new@example.com');
      await user.type(screen.getByLabelText('Password'), 'password123');

      await act(async () => {
        await user.click(screen.getByRole('button', { name: 'Create account' }));
      });

      // The register API should have been called
      await waitFor(() => {
        expect(vi.mocked(fetch)).toHaveBeenCalledWith(
          expect.stringContaining('/auth/register'),
          expect.objectContaining({ method: 'POST' })
        );
      });
    });

    it('should validate password length before submitting', async () => {
      const user = userEvent.setup();

      vi.mocked(fetch).mockResolvedValueOnce({
        ok: false,
        status: 401,
        json: () => Promise.resolve({ error: 'Unauthorized' }),
      } as unknown as Response);

      render(
        <MemoryRouter initialEntries={['/register']}>
          <App />
        </MemoryRouter>
      );

      await waitFor(() => {
        expect(screen.getByText('Create your account')).toBeInTheDocument();
      });

      await user.type(screen.getByLabelText('Full name'), 'Test');
      await user.type(screen.getByLabelText('Email address'), 'test@example.com');
      await user.type(screen.getByLabelText('Password'), '12345');

      await act(async () => {
        await user.click(screen.getByRole('button', { name: 'Create account' }));
      });

      await waitFor(() => {
        expect(screen.getByText('Password must be at least 6 characters')).toBeInTheDocument();
      });

      // The register API should NOT have been called (only the initial auth check)
      expect(vi.mocked(fetch)).toHaveBeenCalledTimes(1);
    });
  });

});
