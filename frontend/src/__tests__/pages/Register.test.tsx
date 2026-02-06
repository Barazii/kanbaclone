import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { MemoryRouter } from 'react-router-dom';
import Register from '@/pages/Register';
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

function renderRegister() {
  return render(
    <MemoryRouter initialEntries={['/register']}>
      <AuthProvider>
        <Register />
      </AuthProvider>
    </MemoryRouter>
  );
}

describe('Register Page', () => {
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
    it('should toggle password visibility', async () => {
      const user = userEvent.setup();
      renderRegister();

      await waitFor(() => {
        expect(screen.getByLabelText('Password')).toBeInTheDocument();
      });

      const passwordField = screen.getByLabelText('Password');
      const toggleButton = passwordField.parentElement?.querySelector('button');

      await user.click(toggleButton!);
      expect(screen.getByLabelText('Password')).toHaveAttribute('type', 'text');

      await user.click(toggleButton!);
      expect(screen.getByLabelText('Password')).toHaveAttribute('type', 'password');
    });
  });

  describe('form submission', () => {
    it('should navigate to dashboard on successful registration', async () => {
      const user = userEvent.setup();
      const mockUser = createMockUser({ name: 'New User', email: 'new@example.com' });

      renderRegister();

      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ user: mockUser }),
      } as Response);

      await waitFor(() => {
        expect(screen.getByLabelText('Full name')).toBeInTheDocument();
      });

      await user.type(screen.getByLabelText('Full name'), 'New User');
      await user.type(screen.getByLabelText('Email address'), 'new@example.com');
      await user.type(screen.getByLabelText('Password'), 'password123');
      await user.click(screen.getByRole('button', { name: 'Create account' }));

      await waitFor(() => {
        expect(mockNavigate).toHaveBeenCalledWith('/dashboard');
      });
    });

    it('should display error for short password', async () => {
      const user = userEvent.setup();

      renderRegister();

      await waitFor(() => {
        expect(screen.getByLabelText('Full name')).toBeInTheDocument();
      });

      await user.type(screen.getByLabelText('Full name'), 'Test');
      await user.type(screen.getByLabelText('Email address'), 'test@example.com');
      await user.type(screen.getByLabelText('Password'), '12345');
      await user.click(screen.getByRole('button', { name: 'Create account' }));

      await waitFor(() => {
        expect(screen.getByText('Password must be at least 6 characters')).toBeInTheDocument();
      });
    });

    it('should display error on failed registration', async () => {
      const user = userEvent.setup();

      renderRegister();

      vi.mocked(fetch).mockResolvedValueOnce({
        ok: false,
        status: 409,
        json: () => Promise.resolve({ error: 'Email already in use' }),
      } as unknown as Response);

      vi.spyOn(console, 'error').mockImplementation(() => {});

      await waitFor(() => {
        expect(screen.getByLabelText('Full name')).toBeInTheDocument();
      });

      await user.type(screen.getByLabelText('Full name'), 'Test');
      await user.type(screen.getByLabelText('Email address'), 'existing@example.com');
      await user.type(screen.getByLabelText('Password'), 'password123');
      await user.click(screen.getByRole('button', { name: 'Create account' }));

      await waitFor(() => {
        expect(
          screen.getByText('Registration failed. Email may already be in use.')
        ).toBeInTheDocument();
      });
    });

    it('should show loading state while registering', async () => {
      const user = userEvent.setup();

      renderRegister();

      vi.mocked(fetch).mockReturnValueOnce(new Promise(() => {}));

      await waitFor(() => {
        expect(screen.getByLabelText('Full name')).toBeInTheDocument();
      });

      await user.type(screen.getByLabelText('Full name'), 'Test');
      await user.type(screen.getByLabelText('Email address'), 'test@example.com');
      await user.type(screen.getByLabelText('Password'), 'password123');
      await user.click(screen.getByRole('button', { name: 'Create account' }));

      await waitFor(() => {
        expect(screen.getByText('Creating account...')).toBeInTheDocument();
      });
    });

    it('should disable the submit button while loading', async () => {
      const user = userEvent.setup();

      renderRegister();

      vi.mocked(fetch).mockReturnValueOnce(new Promise(() => {}));

      await waitFor(() => {
        expect(screen.getByLabelText('Full name')).toBeInTheDocument();
      });

      await user.type(screen.getByLabelText('Full name'), 'Test');
      await user.type(screen.getByLabelText('Email address'), 'test@example.com');
      await user.type(screen.getByLabelText('Password'), 'password123');
      await user.click(screen.getByRole('button', { name: 'Create account' }));

      await waitFor(() => {
        expect(screen.getByRole('button', { name: 'Creating account...' })).toBeDisabled();
      });
    });
  });

});
