'use client';

import { useEffect, useState } from 'react';
import { Droppable, DroppableProps } from '@hello-pangea/dnd';

/**
 * StrictModeDroppable is a wrapper around Droppable that fixes
 * compatibility issues with React 18+ Strict Mode.
 * 
 * The issue: React 18+ StrictMode double-renders components which
 * causes @hello-pangea/dnd (and react-beautiful-dnd) to break because
 * the droppable tries to register before the DragDropContext is ready.
 * 
 * Solution: Delay rendering the Droppable until after the first render
 * cycle is complete.
 */
export default function StrictModeDroppable({ children, ...props }: DroppableProps) {
  const [enabled, setEnabled] = useState(false);

  useEffect(() => {
    // Use requestAnimationFrame to defer enabling until after paint
    const animation = requestAnimationFrame(() => setEnabled(true));
    return () => {
      cancelAnimationFrame(animation);
      setEnabled(false);
    };
  }, []);

  if (!enabled) {
    return null;
  }

  return <Droppable {...props}>{children}</Droppable>;
}
