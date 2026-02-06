import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { MemoryRouter } from 'react-router-dom';
import Settings from '@/pages/Settings';
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

function renderSettings() {
  return render(
    <MemoryRouter initialEntries={['/settings']}>
      <AuthProvider>
        <Settings />
      </AuthProvider>
    </MemoryRouter>
  );
}

describe('Settings Page', () => {
  const mockUser = createMockUser({ name: 'John Doe', email: 'john@example.com' });

  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
    mockNavigate.mockClear();
  });

  describe('when authenticated', () => {
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

    it('should display user name and email', async () => {
      renderSettings();
      await waitFor(() => {
        // User name appears in sidebar AND settings profile -- both are valid
        const nameMatches = screen.getAllByText('John Doe');
        expect(nameMatches.length).toBeGreaterThanOrEqual(1);
        const emailMatches = screen.getAllByText('john@example.com');
        expect(emailMatches.length).toBeGreaterThanOrEqual(1);
      });
    });

    it('should pre-fill the Display Name input with user name', async () => {
      renderSettings();
      await waitFor(() => {
        const nameInput = screen.getByDisplayValue('John Doe');
        expect(nameInput).toBeInTheDocument();
      });
    });

  });

  describe('saving settings', () => {
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

    it('should show success message on successful save', async () => {
      const user = userEvent.setup();

      renderSettings();

      await waitFor(() => {
        expect(screen.getByDisplayValue('John Doe')).toBeInTheDocument();
      });

      // Setup successful PUT response
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({}),
      } as Response);

      await user.click(screen.getByRole('button', { name: 'Save Changes' }));

      await waitFor(() => {
        expect(screen.getByText('Settings saved successfully')).toBeInTheDocument();
      });
    });

    it('should show error message on failed save', async () => {
      const user = userEvent.setup();

      renderSettings();

      await waitFor(() => {
        expect(screen.getByDisplayValue('John Doe')).toBeInTheDocument();
      });

      vi.mocked(fetch).mockResolvedValueOnce({
        ok: false,
        status: 500,
        json: () => Promise.resolve({ error: 'Server error' }),
      } as unknown as Response);

      await user.click(screen.getByRole('button', { name: 'Save Changes' }));

      await waitFor(() => {
        expect(screen.getByText('Failed to save settings')).toBeInTheDocument();
      });
    });

    it('should show Saving... while saving', async () => {
      const user = userEvent.setup();

      renderSettings();

      await waitFor(() => {
        expect(screen.getByDisplayValue('John Doe')).toBeInTheDocument();
      });

      vi.mocked(fetch).mockReturnValueOnce(new Promise(() => {}));

      await user.click(screen.getByRole('button', { name: 'Save Changes' }));

      await waitFor(() => {
        expect(screen.getByText('Saving...')).toBeInTheDocument();
      });
    });

    it('should disable button while saving', async () => {
      const user = userEvent.setup();

      renderSettings();

      await waitFor(() => {
        expect(screen.getByDisplayValue('John Doe')).toBeInTheDocument();
      });

      vi.mocked(fetch).mockReturnValueOnce(new Promise(() => {}));

      await user.click(screen.getByRole('button', { name: 'Save Changes' }));

      await waitFor(() => {
        expect(screen.getByRole('button', { name: 'Saving...' })).toBeDisabled();
      });
    });

    it('should allow editing the name field', async () => {
      const user = userEvent.setup();

      renderSettings();

      await waitFor(() => {
        expect(screen.getByDisplayValue('John Doe')).toBeInTheDocument();
      });

      const nameInput = screen.getByDisplayValue('John Doe');
      await user.clear(nameInput);
      await user.type(nameInput, 'Jane Smith');

      expect(nameInput).toHaveValue('Jane Smith');
    });
  });

  describe('loading state', () => {
    it('should show spinner while loading', () => {
      vi.mocked(fetch).mockReturnValue(new Promise(() => {}));

      renderSettings();

      const spinner = document.querySelector('.animate-spin');
      expect(spinner).toBeInTheDocument();
    });
  });
});
