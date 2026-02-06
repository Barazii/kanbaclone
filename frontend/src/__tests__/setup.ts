import '@testing-library/jest-dom';
import { cleanup } from '@testing-library/react';
import { afterEach, vi } from 'vitest';

afterEach(() => {
  cleanup();
  vi.restoreAllMocks();
});

// Mock window.confirm globally
vi.stubGlobal('confirm', vi.fn(() => true));

// Mock window.prompt globally
vi.stubGlobal('prompt', vi.fn(() => 'Test Input'));

// Mock window.alert globally
vi.stubGlobal('alert', vi.fn());

// Mock IntersectionObserver
vi.stubGlobal('IntersectionObserver', vi.fn(() => ({
  observe: vi.fn(),
  unobserve: vi.fn(),
  disconnect: vi.fn(),
})));

// Mock requestAnimationFrame for StrictModeDroppable
vi.stubGlobal('requestAnimationFrame', (cb: FrameRequestCallback) => {
  cb(0);
  return 0;
});
vi.stubGlobal('cancelAnimationFrame', vi.fn());

// Mock scrollIntoView
Element.prototype.scrollIntoView = vi.fn();
