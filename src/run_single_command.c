#include "minishell.h"

int	run_single_command(t_cmd *cmd, t_shell *sh)
{
	pid_t	pid;

	if (!cmd->argv || !cmd->argv[0])
		return (1);
	if (cmd->is_builtin)
		return (1);
	pid = fork();
	if (pid == 0)
	{
		if (apply_redirs(cmd) < 0)
			exit(1);
		exec_external(cmd ->argv,sh ->envp);
	}
	return (0);
}

int	apply_redirs(const t_cmd *cmd)
{
	const t_redir	*redir;
	int				fd;

	if (!cmd)
		return (-1);
	redir = cmd->redirs;
	while (redir)
	{
		fd = -1;
		// if (redir->kind == R_HDOC)
		// {
		// 	fd = redir->hdoc_fd; // 事前収集済みの読み端
		// 	if (fd < 0)
		// 		return (-1);
		// 	if (dup2(fd, STDIN_FILENO) < 0)
		// 		return (perror("dup2"), -1);
		// 	close(fd);
		// }
		if (redir->kind == R_IN)
		{
			fd = open(redir->arg, O_RDONLY);
			if (fd < 0)
				return (perror(redir->arg), -1);
			if (dup2(fd, STDIN_FILENO) < 0)
			{
				perror("dup2");
				close(fd);
				return (-1);
			}
			close(fd);
		}
		else if (redir->kind == R_OUT)
		{
			fd = open(redir->arg, O_CREAT | O_WRONLY | O_TRUNC, 0644);
			if (fd < 0)
				return (perror(redir->arg), -1);
			if (dup2(fd, STDOUT_FILENO) < 0)
			{
				perror("dup2");
				close(fd);
				return (-1);
			}
			close(fd);
		}
		else if (redir->kind == R_APP)
		{
			fd = open(redir->arg, O_CREAT | O_WRONLY | O_APPEND, 0644);
			if (fd < 0)
				return (perror(redir->arg), -1);
			if (dup2(fd, STDOUT_FILENO) < 0)
			{
				perror("dup2");
				close(fd);
				return (-1);
			}
			close(fd);
		}
		redir = redir->next; // 次のリダイレクトへ
	}
	return (0);
}
