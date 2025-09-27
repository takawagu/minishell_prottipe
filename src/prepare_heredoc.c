/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prepare_heredoc.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: takawagu <takawagu@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/26 13:38:08 by takawagu          #+#    #+#             */
/*   Updated: 2025/09/26 14:47:08 by takawagu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include"minishell.h"

// 1つの R_HDOC を準備する（変数名は元のまま）
static int prepare_one_heredoc(t_redir *redir, t_shell *sh, t_ast *node)
{
    int fd[2];
    if (pipe(fd) == -1)
        return 1;

    int write_fd = fd[1];
    int read_fd  = fd[0];
    int status = read_heredoc_into_fd(write_fd, redir, sh);
    close(write_fd);

    if (status != 0) {              // 130 や 1
        close(read_fd);
        close_all_prepared_hdocs(node);
        return status;
    }
    redir->hdoc_fd = read_fd;          // 実行時に dup2 するため保存
    return 0;
}

// CMDノード内の全 heredoc を準備する
static int prepare_cmd_heredocs(t_cmd *cmd, t_shell *sh, t_ast *node)
{
    t_redir *redir = cmd->redirs;
    while (redir) {
        if (redir->kind == R_HDOC) {
            int st = prepare_one_heredoc(redir, sh, node);
            if (st != 0) return st;
        }
        redir = redir->next;
    }
    return 0;
}

int prepare_heredocs(t_ast *node, t_shell *sh)
{
    if (!node) return 0;

    if (node->type == AST_CMD)
        return prepare_cmd_heredocs(&node->as.cmd, sh, node);

    // AST_PIPE
    int st = prepare_heredocs(node->as.pipe.left, sh);
    if (st != 0) return st;
    return prepare_heredocs(node->as.pipe.right, sh);
}

int read_heredoc_into_fd(int write_fd, t_redir *redir, t_shell *sh)
{
    (void)sh; // いまは未使用（展開しない最小版）
    char  *limmiter = redir->arg;
    char  *line;

    while ((line = readline("heredoc> ")) != NULL)
    {
        // リミッタに「完全一致」したら終了（readlineは改行を含まない）
        if (ft_strcmp(line, limmiter) == 0)
        {
            free(line);
            break;
        }

        //環境変数があってquotationで囲まれていない場合は展開する
        // if (redir->quoted == 0) 
        // {
        //     char *expanded = expand_dollar(line, sh);
        //     if (!expanded) 
        //         {
        //             free(line); return 1;
        //         } 
        //     free(line);
        //     line = expanded;
        // }

        // 本文を書き出し（改行は自分で付ける）
        if (write(write_fd, line, ft_strlen(line)) < 0) 
            return (free(line),1);
        if (write(write_fd, "\n", 1) < 0)
            return (free(line),1);
        free(line);
    }

    // Ctrl-D(EOF)で抜けた場合も今は成功扱い（仕様に合わせて後で調整可）
    return 0;
}

void close_all_prepared_hdocs(t_ast *node)
{
    if (!node) return;
    if (node->type == AST_CMD) {
        t_redir *redir = node->as.cmd.redirs;
        while (redir) {
            if (redir->kind == R_HDOC && redir->hdoc_fd >= 0) {
                close(redir->hdoc_fd);
                redir->hdoc_fd = -1;
            }
            redir = redir->next;
        }
        return;
    }
    close_all_prepared_hdocs(node->as.pipe.left);
    close_all_prepared_hdocs(node->as.pipe.right);
}

