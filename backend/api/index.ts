import express from 'express';
import cors from 'cors';
import cookieParser from 'cookie-parser';

import authRoutes from '../src/routes/auth.js';
import projectsRoutes from '../src/routes/projects.js';
import tasksRoutes from '../src/routes/tasks.js';
import columnsRoutes from '../src/routes/columns.js';
import aiChatRoutes from '../src/routes/ai-chat.js';

const app = express();

const allowedOrigins = [
  'https://frontend-seven-smoky-dzcjimc6xj.vercel.app',
  process.env.FRONTEND_URL,
  'http://localhost:5173',
  'http://localhost:3000',
].filter(Boolean) as string[];

// Middleware
app.use(
  cors({
    origin: allowedOrigins,
    credentials: true,
    methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
    allowedHeaders: ['Content-Type', 'Authorization', 'Cookie'],
  })
);
app.use(express.json());
app.use(cookieParser());

// Routes
app.use('/api/auth', authRoutes);
app.use('/api/projects', projectsRoutes);
app.use('/api/tasks', tasksRoutes);
app.use('/api/columns', columnsRoutes);
app.use('/api/ai-chat', aiChatRoutes);

// Health check
app.get('/api/health', (req, res) => {
  res.json({ status: 'ok' });
});

// Root endpoint
app.get('/', (req, res) => {
  res.json({ message: 'Kanba API', status: 'running' });
});

// Export for Vercel serverless
export default app;
