import { Router, Request, Response } from 'express';

const router = Router();

// POST /api/ai-chat
router.post('/', async (req: Request, res: Response) => {
  try {
    const { messages, apiKey } = req.body;

    if (!apiKey) {
      return res.status(400).json({ error: 'API key is required' });
    }

    if (!messages || !Array.isArray(messages) || messages.length === 0) {
      return res.status(400).json({ error: 'Messages are required' });
    }

    const response = await fetch('https://api.openai.com/v1/chat/completions', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        Authorization: `Bearer ${apiKey}`,
      },
      body: JSON.stringify({
        model: 'gpt-4o-mini',
        messages: messages.map((m: { role: string; content: string }) => ({
          role: m.role,
          content: m.content,
        })),
      }),
    });

    if (!response.ok) {
      const err = (await response.json().catch(() => ({}))) as { error?: { message?: string } };
      const message = err?.error?.message || `OpenAI API error (${response.status})`;
      return res.status(response.status).json({ error: message });
    }

    const data = (await response.json()) as { choices?: { message?: { content?: string } }[] };
    const assistantMessage = data.choices?.[0]?.message?.content || 'No response';

    return res.json({ message: assistantMessage });
  } catch (error) {
    const message = error instanceof Error ? error.message : 'Internal server error';
    return res.status(500).json({ error: message });
  }
});

export default router;
