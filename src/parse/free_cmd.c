/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free_cmd.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: takawagu <takawagu@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/13 18:15:56 by takawagu          #+#    #+#             */
/*   Updated: 2025/10/17 16:41:46 by takawagu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static void	free_cmd_argv(t_cmd *cmd)
{
	size_t	i;

	if (!cmd->argv)
		return ;
	i = 0;
	while (cmd->argv[i])
		free(cmd->argv[i++]);
	free(cmd->argv);
	cmd->argv = NULL;
}



static void	free_cmd_redirs(t_cmd *cmd)
{
	t_redir	*next;

	while (cmd->redirs)
	{
		next = cmd->redirs->next;
		free(cmd->redirs->arg);
		free_wordinfo(cmd->redirs->word_info);
		free(cmd->redirs);
		cmd->redirs = next;
	}
}

void	free_cmd(t_cmd *cmd)
{
	if (!cmd)
		return ;
	free_cmd_argv(cmd);
	free_cmd_wordinfos(cmd);
	free_cmd_redirs(cmd);
}

t_ast	*parse_command_fail(t_cmd *cmd)
{
	free_cmd(cmd);
	return (NULL);
}
