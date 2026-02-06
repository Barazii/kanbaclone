import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { MemoryRouter } from 'react-router-dom';
import AIAssistant from '@/pages/AIAssistant';
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

function renderAIAssistant() {
  return render(
    <MemoryRouter initialEntries={['/ai-assistant']}>
      <AuthProvider>
        <AIAssistant />
      </AuthProvider>
    </MemoryRouter>
  );
}

describe('AIAssistant Page', () => {
  const mockUser = createMockUser();

  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
    mockNavigate.mockClear();
    localStorage.clear();
  });

  describe('without API key', () => {
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

    it('should display API key input', async () => {
      renderAIAssistant();
      await waitFor(() => {
        expect(screen.getByPlaceholderText('sk-...')).toBeInTheDocument();
      });
    });

    it('should display Save API Key button', async () => {
      renderAIAssistant();
      await waitFor(() => {
        expect(screen.getByRole('button', { name: 'Save API Key' })).toBeInTheDocument();
      });
    });

    it('should disable Save API Key button when input is empty', async () => {
      renderAIAssistant();
      await waitFor(() => {
        expect(screen.getByRole('button', { name: 'Save API Key' })).toBeDisabled();
      });
    });

    it('should enable Save API Key button when input has content', async () => {
      const user = userEvent.setup();
      renderAIAssistant();

      await waitFor(() => {
        expect(screen.getByPlaceholderText('sk-...')).toBeInTheDocument();
      });

      await user.type(screen.getByPlaceholderText('sk-...'), 'sk-test-key');

      expect(screen.getByRole('button', { name: 'Save API Key' })).not.toBeDisabled();
    });

    it('should save API key to localStorage and show chat', async () => {
      const user = userEvent.setup();
      renderAIAssistant();

      await waitFor(() => {
        expect(screen.getByPlaceholderText('sk-...')).toBeInTheDocument();
      });

      await user.type(screen.getByPlaceholderText('sk-...'), 'sk-test-key-123');
      await user.click(screen.getByRole('button', { name: 'Save API Key' }));

      expect(localStorage.getItem('openai_api_key')).toBe('sk-test-key-123');

      // After saving key, should switch to chat interface
      await waitFor(() => {
        expect(screen.getByPlaceholderText('Type a message...')).toBeInTheDocument();
      });
    });

    it('should save API key on Enter press', async () => {
      const user = userEvent.setup();
      renderAIAssistant();

      await waitFor(() => {
        expect(screen.getByPlaceholderText('sk-...')).toBeInTheDocument();
      });

      const input = screen.getByPlaceholderText('sk-...');
      await user.type(input, 'sk-enter-key{Enter}');

      expect(localStorage.getItem('openai_api_key')).toBe('sk-enter-key');
    });

  });

  describe('with API key', () => {
    beforeEach(() => {
      localStorage.setItem('openai_api_key', 'sk-existing-key');
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

    it('should show the chat interface', async () => {
      renderAIAssistant();
      await waitFor(() => {
        expect(screen.getByPlaceholderText('Type a message...')).toBeInTheDocument();
      });
    });

    it('should show the Remove API Key button', async () => {
      renderAIAssistant();
      await waitFor(() => {
        expect(screen.getByText('Remove API Key')).toBeInTheDocument();
      });
    });

    it('should display empty state message when no messages', async () => {
      renderAIAssistant();
      await waitFor(() => {
        expect(screen.getByText('Discuss your tasks with ChatGPT')).toBeInTheDocument();
      });
    });

    it('should remove API key and return to setup on Remove click', async () => {
      const user = userEvent.setup();
      renderAIAssistant();

      await waitFor(() => {
        expect(screen.getByText('Remove API Key')).toBeInTheDocument();
      });

      await user.click(screen.getByText('Remove API Key'));

      expect(localStorage.getItem('openai_api_key')).toBeNull();

      await waitFor(() => {
        expect(screen.getByPlaceholderText('sk-...')).toBeInTheDocument();
      });
    });

    it('should send a message and display response', async () => {
      const user = userEvent.setup();
      renderAIAssistant();

      await waitFor(() => {
        expect(screen.getByPlaceholderText('Type a message...')).toBeInTheDocument();
      });

      // Setup chat response
      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ message: 'Hello! How can I help you?' }),
      } as Response);

      const input = screen.getByPlaceholderText('Type a message...');
      await user.type(input, 'Hello AI{Enter}');

      // Wait for user message to appear
      await waitFor(() => {
        expect(screen.getByText('Hello AI')).toBeInTheDocument();
      });

      // Wait for response
      await waitFor(() => {
        expect(screen.getByText('Hello! How can I help you?')).toBeInTheDocument();
      });
    });

    it('should send message on Enter key', async () => {
      const user = userEvent.setup();
      renderAIAssistant();

      await waitFor(() => {
        expect(screen.getByPlaceholderText('Type a message...')).toBeInTheDocument();
      });

      vi.mocked(fetch).mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ message: 'Response!' }),
      } as Response);

      const input = screen.getByPlaceholderText('Type a message...');
      await user.type(input, 'Hello{Enter}');

      await waitFor(() => {
        expect(screen.getByText('Hello')).toBeInTheDocument();
      });
    });

    it('should handle API errors in chat', async () => {
      const user = userEvent.setup();
      renderAIAssistant();

      await waitFor(() => {
        expect(screen.getByPlaceholderText('Type a message...')).toBeInTheDocument();
      });

      vi.mocked(fetch).mockResolvedValueOnce({
        ok: false,
        status: 500,
        json: () => Promise.resolve({ error: 'API rate limit exceeded' }),
      } as unknown as Response);

      const input = screen.getByPlaceholderText('Type a message...');
      await user.type(input, 'Hello{Enter}');

      await waitFor(() => {
        expect(screen.getByText(/Error: API rate limit exceeded/)).toBeInTheDocument();
      });
    });

    it('should not send empty messages', async () => {
      const user = userEvent.setup();
      renderAIAssistant();

      await waitFor(() => {
        expect(screen.getByPlaceholderText('Type a message...')).toBeInTheDocument();
      });

      const input = screen.getByPlaceholderText('Type a message...');
      await user.type(input, '   ');

      // The send button should be disabled (or clicking it does nothing)
      const sendButtons = screen.getAllByRole('button');
      const sendBtn = sendButtons[sendButtons.length - 1]; // The send button is the last one in the chat area
      expect(sendBtn).toBeDisabled();
    });

    it('should disable input while sending', async () => {
      const user = userEvent.setup();
      renderAIAssistant();

      await waitFor(() => {
        expect(screen.getByPlaceholderText('Type a message...')).toBeInTheDocument();
      });

      // Make the API call hang
      vi.mocked(fetch).mockReturnValueOnce(new Promise(() => {}));

      const input = screen.getByPlaceholderText('Type a message...');
      await user.type(input, 'Hello{Enter}');

      await waitFor(() => {
        expect(screen.getByPlaceholderText('Type a message...')).toBeDisabled();
      });
    });
  });

  describe('loading state', () => {
    it('should show spinner while loading', () => {
      vi.mocked(fetch).mockReturnValue(new Promise(() => {}));

      renderAIAssistant();

      const spinner = document.querySelector('.animate-spin');
      expect(spinner).toBeInTheDocument();
    });
  });
});
