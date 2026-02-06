import { describe, it, expect, vi, beforeEach } from 'vitest';
import { apiRequest, api } from '@/api/client';

describe('apiRequest', () => {
  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
  });

  it('should make a request to the correct URL with default options', async () => {
    const mockData = { success: true };
    vi.mocked(fetch).mockResolvedValue({
      ok: true,
      json: () => Promise.resolve(mockData),
    } as Response);

    const result = await apiRequest('/test');

    expect(fetch).toHaveBeenCalledWith(
      'http://localhost:3001/api/test',
      expect.objectContaining({
        credentials: 'include',
        headers: expect.objectContaining({
          'Content-Type': 'application/json',
        }),
      })
    );
    expect(result).toEqual(mockData);
  });

  it('should merge custom headers with defaults', async () => {
    vi.mocked(fetch).mockResolvedValue({
      ok: true,
      json: () => Promise.resolve({}),
    } as Response);

    await apiRequest('/test', {
      headers: { Authorization: 'Bearer token123' },
    });

    expect(fetch).toHaveBeenCalledWith(
      expect.any(String),
      expect.objectContaining({
        headers: expect.objectContaining({
          'Content-Type': 'application/json',
          Authorization: 'Bearer token123',
        }),
      })
    );
  });

  it('should merge custom options like method and body', async () => {
    vi.mocked(fetch).mockResolvedValue({
      ok: true,
      json: () => Promise.resolve({}),
    } as Response);

    await apiRequest('/test', {
      method: 'POST',
      body: JSON.stringify({ key: 'value' }),
    });

    expect(fetch).toHaveBeenCalledWith(
      expect.any(String),
      expect.objectContaining({
        method: 'POST',
        body: JSON.stringify({ key: 'value' }),
        credentials: 'include',
      })
    );
  });

  it('should throw an error with server message on non-ok response', async () => {
    vi.mocked(fetch).mockResolvedValue({
      ok: false,
      status: 400,
      json: () => Promise.resolve({ error: 'Bad request' }),
    } as unknown as Response);

    await expect(apiRequest('/test')).rejects.toThrow('Bad request');
  });

  it('should throw a generic error when response has no error message', async () => {
    vi.mocked(fetch).mockResolvedValue({
      ok: false,
      status: 500,
      json: () => Promise.resolve({}),
    } as unknown as Response);

    await expect(apiRequest('/test')).rejects.toThrow('Request failed: 500');
  });

  it('should throw a generic error when response body is not valid JSON', async () => {
    vi.mocked(fetch).mockResolvedValue({
      ok: false,
      status: 502,
      json: () => Promise.reject(new Error('not json')),
    } as unknown as Response);

    await expect(apiRequest('/test')).rejects.toThrow('Request failed: 502');
  });

});

describe('api convenience methods', () => {
  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn());
    vi.mocked(fetch).mockResolvedValue({
      ok: true,
      json: () => Promise.resolve({ data: 'ok' }),
    } as Response);
  });

  it('api.get should make a GET request', async () => {
    const result = await api.get('/items');

    expect(fetch).toHaveBeenCalledWith(
      'http://localhost:3001/api/items',
      expect.objectContaining({
        credentials: 'include',
      })
    );
    // GET requests should not have method explicitly set (defaults to GET)
    const callArgs = vi.mocked(fetch).mock.calls[0][1] as RequestInit;
    expect(callArgs.method).toBeUndefined();
    expect(result).toEqual({ data: 'ok' });
  });

  it('api.post should make a POST request with JSON body', async () => {
    await api.post('/items', { name: 'test' });

    expect(fetch).toHaveBeenCalledWith(
      'http://localhost:3001/api/items',
      expect.objectContaining({
        method: 'POST',
        body: JSON.stringify({ name: 'test' }),
      })
    );
  });

  it('api.post should work without a body', async () => {
    await api.post('/logout');

    expect(fetch).toHaveBeenCalledWith(
      'http://localhost:3001/api/logout',
      expect.objectContaining({
        method: 'POST',
      })
    );
    const callArgs = vi.mocked(fetch).mock.calls[0][1] as RequestInit;
    expect(callArgs.body).toBeUndefined();
  });

  it('api.put should make a PUT request with JSON body', async () => {
    await api.put('/items/1', { name: 'updated' });

    expect(fetch).toHaveBeenCalledWith(
      'http://localhost:3001/api/items/1',
      expect.objectContaining({
        method: 'PUT',
        body: JSON.stringify({ name: 'updated' }),
      })
    );
  });

  it('api.delete should make a DELETE request', async () => {
    await api.delete('/items/1');

    expect(fetch).toHaveBeenCalledWith(
      'http://localhost:3001/api/items/1',
      expect.objectContaining({
        method: 'DELETE',
      })
    );
    const callArgs = vi.mocked(fetch).mock.calls[0][1] as RequestInit;
    expect(callArgs.body).toBeUndefined();
  });
});
