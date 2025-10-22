/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_external.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: takawagu <takawagu@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/04 14:05:28 by takawagu          #+#    #+#             */
/*   Updated: 2025/10/22 18:12:10 by takawagu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static void	validate_and_exec(const char *path, char *const argv[], t_shell *sh)
{
	if (access(path, F_OK) != 0)
		exit(127);
	if (access(path, X_OK) != 0)
		exit(126);
	execve(path, argv, sh->envp);
	if (errno == ENOENT)
		exit(127);
	exit(126);
}

void	exec_external(char *const argv[], t_shell *sh)
{
	char	*path;

	if (!argv || !argv[0])
		exit(0);
	path = find_cmd_path(argv[0], sh);
	if (!path)
		exit(127);
	validate_and_exec(path, argv, sh);
}
