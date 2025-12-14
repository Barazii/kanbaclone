// Simple in-memory session store
// In production, use Redis or a database

const sessions = new Map<string, string>();

export function setSession(sessionId: string, userId: string) {
  sessions.set(sessionId, userId);
}

export function getSession(sessionId: string): string | undefined {
  return sessions.get(sessionId);
}

export function deleteSession(sessionId: string) {
  sessions.delete(sessionId);
}

export function hasSession(sessionId: string): boolean {
  return sessions.has(sessionId);
}
