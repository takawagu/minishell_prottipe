/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   push_word_info.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: takawagu <takawagu@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/17 15:04:34 by takawagu          #+#    #+#             */
/*   Updated: 2025/10/17 16:00:24 by takawagu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static void	*free_wordinfo_parts(t_wordinfo *info, size_t filled)
{
	while (filled > 0)
	{
		filled--;
		free(info->parts[filled].text);
	}
	free(info->parts);
	free(info);
	return (NULL);
}

t_wordinfo	*clone_wordinfo(const t_wordinfo *src)
{
	t_wordinfo	*dup;
	size_t		i;

	dup = malloc(sizeof(*dup));
	if (!dup)
		return (NULL);
	*dup = *src;
	if (src->parts_count == 0)
	{
		dup->parts = NULL;
		return (dup);
	}
	dup->parts = malloc(sizeof(*dup->parts) * src->parts_count);
	if (!dup->parts)
		return (free(dup), NULL);
	i = 0;
	while (i < src->parts_count)
	{
		dup->parts[i].quote = src->parts[i].quote;
		dup->parts[i].text = ft_strdup(src->parts[i].text);
		if (!dup->parts[i].text)
			return (free_wordinfo_parts(dup, i));
		i++;
	}
	return (dup);
}

int	append_wordinfo(t_cmd *cmd, const t_wordinfo *info)
{
	t_wordinfo	**new_infos;
	t_wordinfo	*dup;

	new_infos = malloc(sizeof(*new_infos) * (cmd->argc + 1));
	if (!new_infos)
		return (-1);
	ft_memcpy(new_infos, cmd->word_infos, sizeof(*new_infos) * cmd->argc);
	dup = clone_wordinfo(info);
	if (!dup)
		return (free(new_infos), -1);
	new_infos[cmd->argc] = dup;
	free(cmd->word_infos);
	cmd->word_infos = new_infos;
	return (0);
}
