import { vi } from 'vitest';

const cookieStore = new Map<string, string>();

export const mockCookies = {
  get: (name: string) => {
    const val = cookieStore.get(name);
    return val ? { name, value: val } : undefined;
  },
  set: (name: string, value: string, _options?: Record<string, unknown>) => {
    cookieStore.set(name, value);
  },
  delete: (name: string) => {
    cookieStore.delete(name);
  },
};

export function clearCookiesForTest() {
  cookieStore.clear();
}

vi.mock('next/headers', () => ({
  cookies: async () => mockCookies,
}));
