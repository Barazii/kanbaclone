import { defineConfig } from 'vitest/config';
import path from 'path';

export default defineConfig({
  resolve: {
    alias: { '@': path.resolve(__dirname, './src') },
  },
  test: {
    globalSetup: ['./tests/setup.ts'],
    setupFiles: ['./tests/helpers/mockCookies.ts'],
    env: {
      DB_NAME: 'kanba_test',
    },
    testTimeout: 15000,
    fileParallelism: false,
  },
});
