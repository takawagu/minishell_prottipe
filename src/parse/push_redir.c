/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   push_redir.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: takawagu <takawagu@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/13 19:17:39 by takawagu          #+#    #+#             */
/*   Updated: 2025/10/13 19:35:36 by takawagu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static t_rtype	kind_from_token(t_tok_kind kind)
{
	if (kind == TK_REDIR_IN)
		return (R_IN);
	if (kind == TK_REDIR_OUT)
		return (R_OUT);
	if (kind == TK_REDIR_APP)
		return (R_APP);
	if (kind == TK_HEREDOC)
		return (R_HDOC);
	return (-1);
}

static char	*dup_token_arg(const t_token *token)
{
	if (token->args)
		return (ft_strdup(token->args));
	return (ft_strdup(""));
}

int	push_redir(t_cmd *cmd, const t_token *op_tok, const t_token *word_tok)
{
	t_redir	*new_redir;
	t_redir	**tail;
	t_rtype	kind;

	if (!cmd || !op_tok || !word_tok || word_tok->token_kind != TK_WORD)
		return (-1);
	kind = kind_from_token(op_tok->token_kind);
	if (kind < 0)
		return (-1);
	new_redir = malloc(sizeof(*new_redir));
	if (!new_redir)
		return (-1);
	new_redir->arg = dup_token_arg(word_tok);
	if (!new_redir->arg)
	{
		free(new_redir);
		return (-1);
	}
	new_redir->kind = kind;
	if (kind == R_HDOC)
		new_redir->quoted = op_tok->hdoc_quoted;
	else
		new_redir->quoted = 0;
	if (op_tok->fd_left >= 0)
		new_redir->fd_target = op_tok->fd_left;
	else
		new_redir->fd_target = -1;
	new_redir->hdoc_fd = -1;
	new_redir->next = NULL;
	tail = &cmd->redirs;
	while (*tail)
		tail = &(*tail)->next;
	*tail = new_redir;
	return (0);
}
