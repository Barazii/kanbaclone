import { POST as register } from '@/app/api/auth/register/route';

/**
 * Creates a user and sets up the session cookie for subsequent requests.
 * Returns the user object from the register response.
 */
export async function createAuthenticatedUser(
  email = 'test@test.com',
  password = 'password123',
  name = 'Test User'
) {
  const req = new Request('http://localhost/api/auth/register', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ email, password, name }),
  });
  const res = await register(req);
  const data = await res.json();
  return data.user;
}

/**
 * Creates a project for the authenticated user and returns the project id.
 */
export async function createTestProject(projectName = 'Test Project') {
  const { POST } = await import('@/app/api/projects/route');
  const req = new Request('http://localhost/api/projects', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ name: projectName }),
  });
  const res = await POST(req);
  const data = await res.json();
  return data.id;
}

/**
 * Gets the columns of a project. Returns array of column objects.
 */
export async function getProjectColumns(projectId: string) {
  const { GET } = await import('@/app/api/projects/[id]/route');
  const req = new Request(`http://localhost/api/projects/${projectId}`);
  const res = await GET(req, { params: Promise.resolve({ id: projectId }) });
  const data = await res.json();
  return data.columns;
}
