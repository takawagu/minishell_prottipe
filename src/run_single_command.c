#include "minishell.h"

int	run_single_command(t_cmd *cmd, t_shell *sh)
{
	pid_t	pid;

	if (!cmd->argv || !cmd->argv[0])
		return (1);
	if (cmd->is_builtin)
		return (1);
	// if (prepare_heredocs_for_cmd(sh, cmd) < 0)
	// {
	// 	// Ctrl-C などで中断された場合はここで終了
	// 	return (1);
	// }
	pid = fork();
	if (pid == 0)
	{
		if (apply_redirs(cmd) < 0)
			exit(1);
		exec_external(cmd->argv, sh->envp);
	}
	return (0);
}

static int	inter_target_fd(const t_redir *redir)
{
	if (!redir)
		return (-1);
	if (redir->fd_target >= 0)
		return (redir->fd_target);
	if (redir->kind == R_IN || redir->kind == R_HDOC)
		return (STDIN_FILENO);
	return (STDOUT_FILENO);
}

static int	open_src_fd(t_redir *redir)
{
	int	src_fd;

	src_fd = -1;
	if (redir->kind == R_HDOC)
	{
		src_fd = redir->hdoc_fd;
		if (src_fd < 0)
			return (-1);
	}
	else if (redir->kind == R_IN)
	{
		src_fd = open(redir->arg, O_RDONLY);
		if (src_fd < 0)
			return (perror(redir->arg), -1);
	}
	else if (redir->kind == R_OUT)
	{
		src_fd = open(redir->arg, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		if (src_fd < 0)
			return (perror(redir->arg), -1);
	}
	else if (redir->kind == R_APP)
	{
		src_fd = open(redir->arg, O_CREAT | O_WRONLY | O_APPEND, 0644);
		if (src_fd < 0)
			return (perror(redir->arg), -1);
	}
	return (src_fd);
}

int	apply_redirs(const t_cmd *cmd)
{
	t_redir	*redir;
	int		src_fd;
	int		target_fd;

	if (!cmd)
		return (-1);
	redir = cmd->redirs;
	while (redir)
	{
		src_fd = -1;
		target_fd = inter_target_fd(redir);
		if (target_fd < 0 || target_fd >= 1024)
			return (perror("bad target fd"), -1);
		src_fd = open_src_fd(redir);
		if (src_fd < 0)
			return (-1);
		if (src_fd != target_fd)
		{
			if (dup2(src_fd, target_fd) < 0)
			{
				perror("dup2");
				if (redir->kind != R_HDOC)
					close(src_fd);
				return (-1);
			}
		}
		if (redir->kind != R_HDOC || (redir->kind == R_HDOC
				&& src_fd != target_fd))
			close(src_fd);
		redir = redir->next;
	}
	return (0);
}
