/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   make_envtable.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: takawagu <takawagu@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 16:15:23 by takawagu          #+#    #+#             */
/*   Updated: 2025/10/20 14:17:57 by takawagu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static char	*ft_strndup(const char *str, size_t n)
{
	char	*dup;
	size_t	len;
	size_t	index;

	if (!str)
		return (NULL);
	len = 0;
	while (len < n && str[len])
		len++;
	dup = (char *)malloc(len + 1);
	if (!dup)
		return (NULL);
	index = 0;
	while (index < len)
	{
		dup[index] = str[index];
		index++;
	}
	dup[len] = '\0';
	return (dup);
}

static int	env_push_pair(t_env **head, const char *key, const char *value)
{
	t_env		*env;
	const char	*value_src = value;

	env = malloc(sizeof(*env));
	if (!env)
		return (-1);
	env->key = ft_strdup(key);
	if (value_src == NULL)
		value_src = "";
	env->val = ft_strdup(value_src);
	if (!env->key || !env->val)
	{
		free(env->key);
		free(env->val);
		free(env);
		return (-1);
	}
	env->exported = 1;
	env->next = *head;
	*head = env;
	return (0);
}

int	env_init_from_envp(t_env **head, char **envp)
{
	char	*eq;
	size_t	key_len;
	char	*key;
	char	*val;
	int		i;

	i = 0;
	*head = NULL;
	while (envp && envp[i])
	{
		eq = ft_strchr(envp[i], '=');
		if (!eq)
			continue ;
		key_len = (size_t)(eq - envp[i]);
		key = ft_strndup(envp[i], key_len);
		val = ft_strdup(eq + 1);
		if (!key || !val || env_push_pair(head, key, val) < 0)
			return (free(key), free(val), -1);
		free(key);
		free(val);
		i++;
	}
	return (0);
}
