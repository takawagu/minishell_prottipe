/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free_word_info.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: takawagu <takawagu@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/17 16:41:24 by takawagu          #+#    #+#             */
/*   Updated: 2025/10/17 16:41:51 by takawagu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

void	free_wordinfo(t_wordinfo *info)
{
	size_t	i;

	if (!info)
		return ;
	i = 0;
	while (i < info->parts_count)
		free(info->parts[i++].text);
	free(info->parts);
	free(info);
}

void	free_cmd_wordinfos(t_cmd *cmd)
{
	size_t i;

	if (!cmd->word_infos)
		return ;
	i = 0;
	while (i < cmd->argc)
		free_wordinfo(cmd->word_infos[i++]);
	free(cmd->word_infos);
	cmd->word_infos = NULL;
	cmd->argc = 0;
}