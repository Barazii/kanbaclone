import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { MemoryRouter } from 'react-router-dom';
import Login from '@/pages/Login';
import { AuthProvider } from '@/context/AuthContext';
import { createMockUser } from '../test-utils';

const mockNavigate = vi.fn();

vi.mock('react-router-dom', async () => {
  const actual = await vi.importActual('react-router-dom');
  return {
    ...actual,
    useNavigate: () => mockNavigate,
  };
});

function renderLogin() {
  return render(
    <MemoryRouter initialEntries={['/login']}>
      <AuthProvider>
        <Login />
      </AuthProvider>
    </MemoryRouter>
  );
}

describe('Login Page', () => {
  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
    mockNavigate.mockClear();
    // Default: not authenticated
    vi.mocked(fetch).mockResolvedValueOnce({
      ok: false,
      status: 401,
      json: () => Promise.resolve({ error: 'Unauthorized' }),
    } as unknown as Response);
  });

  describe('password visibility toggle', () => {
    it('should toggle password visibility when eye icon is clicked', async () => {
      const user = userEvent.setup();
      renderLogin();

      await waitFor(() => {
        expect(screen.getByLabelText('Password')).toBeInTheDocument();
      });

      // Find the toggle button (contains the eye icon)
      const passwordField = screen.getByLabelText('Password');
      const toggleButton = passwordField.parentElement?.querySelector('button');
      expect(toggleButton).toBeTruthy();

      await user.click(toggleButton!);
      expect(screen.getByLabelText('Password')).toHaveAttribute('type', 'text');

      await user.click(toggleButton!);
      expect(screen.getByLabelText('Password')).toHaveAttribute('type', 'password');
    });
  });

  describe('form submission', () => {
    it('should navigate to dashboard on successful login', async () => {
      const user = userEvent.setup();
      const mockUser = createMockUser();

      renderLogin();

      // Setup login response
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ user: mockUser }),
      } as Response);

      await waitFor(() => {
        expect(screen.getByLabelText('Email address')).toBeInTheDocument();
      });

      await user.type(screen.getByLabelText('Email address'), 'test@example.com');
      await user.type(screen.getByLabelText('Password'), 'password123');
      await user.click(screen.getByRole('button', { name: 'Sign in' }));

      await waitFor(() => {
        expect(mockNavigate).toHaveBeenCalledWith('/dashboard');
      });
    });

    it('should display error message on failed login', async () => {
      const user = userEvent.setup();

      renderLogin();

      // Setup failed login response
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: false,
        status: 401,
        json: () => Promise.resolve({ error: 'Invalid credentials' }),
      } as unknown as Response);

      vi.spyOn(console, 'error').mockImplementation(() => {});

      await waitFor(() => {
        expect(screen.getByLabelText('Email address')).toBeInTheDocument();
      });

      await user.type(screen.getByLabelText('Email address'), 'test@example.com');
      await user.type(screen.getByLabelText('Password'), 'wrong');
      await user.click(screen.getByRole('button', { name: 'Sign in' }));

      await waitFor(() => {
        expect(screen.getByText('Invalid email or password')).toBeInTheDocument();
      });
    });

    it('should show loading state on the submit button while logging in', async () => {
      const user = userEvent.setup();

      renderLogin();

      // Make login request hang
      vi.mocked(fetch).mockReturnValueOnce(new Promise(() => {}));

      await waitFor(() => {
        expect(screen.getByLabelText('Email address')).toBeInTheDocument();
      });

      await user.type(screen.getByLabelText('Email address'), 'test@example.com');
      await user.type(screen.getByLabelText('Password'), 'password');
      await user.click(screen.getByRole('button', { name: 'Sign in' }));

      await waitFor(() => {
        expect(screen.getByText('Signing in...')).toBeInTheDocument();
      });
    });

    it('should disable the submit button while loading', async () => {
      const user = userEvent.setup();

      renderLogin();

      vi.mocked(fetch).mockReturnValueOnce(new Promise(() => {}));

      await waitFor(() => {
        expect(screen.getByLabelText('Email address')).toBeInTheDocument();
      });

      await user.type(screen.getByLabelText('Email address'), 'test@example.com');
      await user.type(screen.getByLabelText('Password'), 'password');
      await user.click(screen.getByRole('button', { name: 'Sign in' }));

      await waitFor(() => {
        const submitButton = screen.getByRole('button', { name: 'Signing in...' });
        expect(submitButton).toBeDisabled();
      });
    });
  });

});
