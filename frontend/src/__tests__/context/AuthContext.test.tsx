import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor, act } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { AuthProvider, useAuth } from '@/context/AuthContext';
import { MemoryRouter } from 'react-router-dom';
import { createMockUser } from '../test-utils';

// Helper component to access and display auth context values
function AuthConsumer() {
  const { user, loading, login, register, logout } = useAuth();
  return (
    <div>
      <span data-testid="loading">{loading.toString()}</span>
      <span data-testid="user">{user ? user.name : 'null'}</span>
      <span data-testid="email">{user ? user.email : 'null'}</span>
      <button data-testid="login-btn" onClick={() => login('test@example.com', 'password123')}>
        Login
      </button>
      <button
        data-testid="register-btn"
        onClick={() => register('new@example.com', 'password123', 'New User')}
      >
        Register
      </button>
      <button data-testid="logout-btn" onClick={logout}>
        Logout
      </button>
    </div>
  );
}

function renderAuthConsumer() {
  return render(
    <MemoryRouter>
      <AuthProvider>
        <AuthConsumer />
      </AuthProvider>
    </MemoryRouter>
  );
}

describe('AuthContext', () => {
  const mockUser = createMockUser();

  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
  });

  describe('initial auth check', () => {
    it('should set user when /auth/me succeeds', async () => {
      vi.mocked(fetch).mockResolvedValue({
        ok: true,
        json: () => Promise.resolve({ user: mockUser }),
      } as Response);

      renderAuthConsumer();

      // Initially loading
      expect(screen.getByTestId('loading').textContent).toBe('true');

      await waitFor(() => {
        expect(screen.getByTestId('loading').textContent).toBe('false');
      });
      expect(screen.getByTestId('user').textContent).toBe('Test User');
    });

    it('should leave user null when /auth/me fails', async () => {
      vi.mocked(fetch).mockResolvedValue({
        ok: false,
        status: 401,
        json: () => Promise.resolve({ error: 'Unauthorized' }),
      } as unknown as Response);

      renderAuthConsumer();

      await waitFor(() => {
        expect(screen.getByTestId('loading').textContent).toBe('false');
      });
      expect(screen.getByTestId('user').textContent).toBe('null');
    });

    it('should handle network errors on auth check gracefully', async () => {
      vi.mocked(fetch).mockRejectedValue(new Error('Network error'));

      renderAuthConsumer();

      await waitFor(() => {
        expect(screen.getByTestId('loading').textContent).toBe('false');
      });
      expect(screen.getByTestId('user').textContent).toBe('null');
    });
  });

  describe('login', () => {
    it('should set user on successful login', async () => {
      // First call is checkAuth, second call is login
      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: false,
          status: 401,
          json: () => Promise.resolve({ error: 'Unauthorized' }),
        } as unknown as Response)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ user: mockUser }),
        } as Response);

      renderAuthConsumer();

      await waitFor(() => {
        expect(screen.getByTestId('loading').textContent).toBe('false');
      });

      await act(async () => {
        await userEvent.click(screen.getByTestId('login-btn'));
      });

      await waitFor(() => {
        expect(screen.getByTestId('user').textContent).toBe('Test User');
      });
    });

    it('should leave user null on failed login', async () => {
      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: false,
          status: 401,
          json: () => Promise.resolve({ error: 'Unauthorized' }),
        } as unknown as Response)
        .mockResolvedValueOnce({
          ok: false,
          status: 401,
          json: () => Promise.resolve({ error: 'Invalid credentials' }),
        } as unknown as Response);

      const consoleSpy = vi.spyOn(console, 'error').mockImplementation(() => {});

      renderAuthConsumer();

      await waitFor(() => {
        expect(screen.getByTestId('loading').textContent).toBe('false');
      });

      await act(async () => {
        await userEvent.click(screen.getByTestId('login-btn'));
      });

      expect(screen.getByTestId('user').textContent).toBe('null');
      expect(consoleSpy).toHaveBeenCalledWith('Login failed:', expect.any(Error));
    });
  });

  describe('register', () => {
    it('should set user on successful registration', async () => {
      const newUser = createMockUser({ id: 'user-2', name: 'New User', email: 'new@example.com' });

      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: false,
          status: 401,
          json: () => Promise.resolve({ error: 'Unauthorized' }),
        } as unknown as Response)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ user: newUser }),
        } as Response);

      renderAuthConsumer();

      await waitFor(() => {
        expect(screen.getByTestId('loading').textContent).toBe('false');
      });

      await act(async () => {
        await userEvent.click(screen.getByTestId('register-btn'));
      });

      await waitFor(() => {
        expect(screen.getByTestId('user').textContent).toBe('New User');
      });
    });

    it('should leave user null on failed registration', async () => {
      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: false,
          status: 401,
          json: () => Promise.resolve({ error: 'Unauthorized' }),
        } as unknown as Response)
        .mockResolvedValueOnce({
          ok: false,
          status: 409,
          json: () => Promise.resolve({ error: 'Email already in use' }),
        } as unknown as Response);

      const consoleSpy = vi.spyOn(console, 'error').mockImplementation(() => {});

      renderAuthConsumer();

      await waitFor(() => {
        expect(screen.getByTestId('loading').textContent).toBe('false');
      });

      await act(async () => {
        await userEvent.click(screen.getByTestId('register-btn'));
      });

      expect(screen.getByTestId('user').textContent).toBe('null');
      expect(consoleSpy).toHaveBeenCalledWith('Registration failed:', expect.any(Error));
    });
  });

  describe('logout', () => {
    it('should clear user on logout', async () => {
      vi.mocked(fetch)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({ user: mockUser }),
        } as Response)
        .mockResolvedValueOnce({
          ok: true,
          json: () => Promise.resolve({}),
        } as Response);

      renderAuthConsumer();

      await waitFor(() => {
        expect(screen.getByTestId('user').textContent).toBe('Test User');
      });

      await act(async () => {
        await userEvent.click(screen.getByTestId('logout-btn'));
      });

      await waitFor(() => {
        expect(screen.getByTestId('user').textContent).toBe('null');
      });
    });

    it('should log error on failed logout', async () => {
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

      const consoleSpy = vi.spyOn(console, 'error').mockImplementation(() => {});

      renderAuthConsumer();

      await waitFor(() => {
        expect(screen.getByTestId('user').textContent).toBe('Test User');
      });

      await act(async () => {
        await userEvent.click(screen.getByTestId('logout-btn'));
      });

      expect(consoleSpy).toHaveBeenCalledWith('Logout failed:', expect.any(Error));
    });
  });

});
