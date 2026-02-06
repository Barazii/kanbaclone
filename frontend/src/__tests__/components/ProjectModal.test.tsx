import { describe, it, expect, vi } from 'vitest';
import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import ProjectModal from '@/components/ProjectModal';

describe('ProjectModal', () => {
  const defaultProps = {
    onClose: vi.fn(),
    onSave: vi.fn(),
  };

  describe('create mode (no project prop)', () => {
    it('should call onSave with form data on submit', async () => {
      const user = userEvent.setup();
      const onSave = vi.fn();

      render(<ProjectModal {...defaultProps} onSave={onSave} />);

      await user.type(screen.getByPlaceholderText('Enter project name'), 'My New Project');
      await user.type(
        screen.getByPlaceholderText("What's this project about?"),
        'Some description'
      );
      await user.click(screen.getByRole('button', { name: 'Create Project' }));

      expect(onSave).toHaveBeenCalledWith({
        name: 'My New Project',
        description: 'Some description',
        icon: 'folder',
      });
    });

    it('should call onClose when Cancel is clicked', async () => {
      const user = userEvent.setup();
      const onClose = vi.fn();

      render(<ProjectModal {...defaultProps} onClose={onClose} />);
      await user.click(screen.getByRole('button', { name: 'Cancel' }));

      expect(onClose).toHaveBeenCalledTimes(1);
    });

    it('should call onClose when X button is clicked', async () => {
      const user = userEvent.setup();
      const onClose = vi.fn();

      render(<ProjectModal {...defaultProps} onClose={onClose} />);
      // The X button has an X icon - it is not labeled, find by structure
      const buttons = screen.getAllByRole('button');
      // X close button is the one that is not Cancel or Create Project
      const closeButton = buttons.find(
        (btn) =>
          btn.textContent !== 'Cancel' &&
          btn.textContent !== 'Create Project' &&
          btn.textContent !== 'Save Changes'
      );
      await user.click(closeButton!);

      expect(onClose).toHaveBeenCalledTimes(1);
    });

  });

  describe('edit mode (with project prop)', () => {
    const existingProject = {
      id: 'project-1',
      name: 'Existing Project',
      description: 'Old description',
      icon: 'folder',
    };

    it('should call onSave with updated data on submit', async () => {
      const user = userEvent.setup();
      const onSave = vi.fn();

      render(<ProjectModal {...defaultProps} onSave={onSave} project={existingProject} />);

      const nameInput = screen.getByPlaceholderText('Enter project name');
      await user.clear(nameInput);
      await user.type(nameInput, 'Updated Project');
      await user.click(screen.getByRole('button', { name: 'Save Changes' }));

      expect(onSave).toHaveBeenCalledWith({
        name: 'Updated Project',
        description: 'Old description',
        icon: 'folder',
      });
    });

    it('should handle project with no description', () => {
      const projectNoDesc = { id: 'p1', name: 'No Desc', icon: 'folder' };
      render(<ProjectModal {...defaultProps} project={projectNoDesc} />);
      expect(screen.getByPlaceholderText("What's this project about?")).toHaveValue('');
    });
  });

});
