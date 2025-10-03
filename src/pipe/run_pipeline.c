/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run_pipeline.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: takawagu <takawagu@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/30 14:24:15 by takawagu          #+#    #+#             */
/*   Updated: 2025/10/03 13:07:31 by takawagu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static size_t	count_cmds_in_pipeline(const t_ast *node)
{
	if (!node)
		return (0);
	if (node->type == AST_CMD)
		return (1);
	return (count_cmds_in_pipeline(node->as.pipe.left)
		+ count_cmds_in_pipeline(node->as.pipe.right));
}

static void	flatten_pipeline(const t_ast *node, t_cmd **cmd_array,
		size_t *index)
{
	if (!node)
		return ;
	if (node->type == AST_PIPE)
	{
		flatten_pipeline(node->as.pipe.left, cmd_array, index);
		flatten_pipeline(node->as.pipe.right, cmd_array, index);
	}
	else // AST_CMD
		cmd_array[(*index)++] = (t_cmd *)&node->as.cmd;
}

static int	prepare_step_pipe(size_t cmd_idx, size_t n, int pipefd[2])
{
	pipefd[0] = -1;
	pipefd[1] = -1;
	if (cmd_idx + 1 >= n)
		return (0); // 最後のコマンド → パイプ不要
	if (pipe(pipefd) < 0)
		return (-1); // エラー
	return (1);      // パイプ作成した
}

// root からパイプ列を展開して seq/n を作る。
// 成功: 0、失敗: 非0（sh->last_status に適切な値を格納済み）
static int	build_pipeline_seq(const t_ast *root, t_cmd ***out_seq,
		size_t *out_n, t_shell *sh)
{
	size_t	n;
	size_t	i;
	t_cmd	**seq;

	*out_seq = NULL;
	*out_n = 0;
	// 1) 個数チェック
	n = count_cmds_in_pipeline(root);
	if (n < 2)
	{
		// パーサで弾けなかった異常（構文エラー相当）
		sh->last_status = 2;
		return (sh->last_status);
	}
	// 2) 配列確保 → flatten
	seq = (t_cmd **)malloc(sizeof(t_cmd *) * n);
	if (!seq)
	{
		perror("malloc");
		sh->last_status = 1;
		return (sh->last_status);
	}
	i = 0;
	flatten_pipeline(root, seq, &i);
	if (i != n)
		n = i; // 念のため同期
	*out_seq = seq;
	*out_n = n;
	return (0);
}

// 成功: 0 を返し、*out_seq と *out_n をセット（seq の所有権は呼び出し側）。
// 失敗: 非0を返す。必要に応じて perror 済み。sh->last_status も設定済み。
// 失敗時は内部で seq を free 済みなので、呼び出し側は free 不要。
static int	setup_pipeline_exec(const t_ast *root, t_cmd ***out_seq,
		size_t *out_n, t_shell *sh)
{
	t_cmd	**seq;
	size_t	n;

	seq = NULL;
	n = 0;
	*out_seq = NULL;
	*out_n = 0;
	if (!root)
	{
		sh->last_status = 0;
		return (sh->last_status); // 空入力扱い（成功相当にしてもOK）
	}
	// 1+2) 個数チェック→配列確保→flatten
	if (build_pipeline_seq(root, &seq, &n, sh) != 0)
		return (sh->last_status); // sh->last_status はヘルパ内でセット済み
	// 3) heredoc を事前に全部開く（中断時はここで止める）
	if (prepare_all_heredocs(seq, n, sh) != 0)
	{
		// 途中中断（130など）を sh->last_status に積んでいる想定
		free(seq);
		return (sh->last_status);
	}
	*out_seq = seq;
	*out_n = n;
	return (0);
}

// ここが「全部入り」版。
// root は AST_PIPE を想定（AST_CMD のときは exec_entry 側で run_single_command を呼ぶ）
int	run_pipeline(const t_ast *root, t_shell *sh)
{
	t_cmd	**seq;
	size_t	n;
	int		prev_read;
	pid_t	last_pid;
	int		pipefd[2];
	pid_t	pid;
	int		st;
	size_t	i;

	seq = NULL;
	n = 0;
	prev_read = -1;
	last_pid = -1;
	int pstate, need_pipe_out;
	// ★ 事前準備（seq 構築～heredoc 準備）だけヘルパに任せる
	if (setup_pipeline_exec(root, &seq, &n, sh) != 0)
		return (sh->last_status);
	// 4) 実行ループ
	for (i = 0; i < n; ++i)
	{
		pstate = prepare_step_pipe(i, n, pipefd);
		if (pstate < 0)
		{
			perror("pipe");
			if (prev_read != -1)
				close(prev_read);
			close_hdocs_in_pipeline(seq, n);
			free(seq);
			return (sh->last_status = 1);
		}
		need_pipe_out = (pstate == 1);
		
		pid = fork();
		if (pid < 0)
		{
			perror("fork");
			if (prev_read != -1)
				close(prev_read);
			if (need_pipe_out)
			{
				close(pipefd[0]);
				close(pipefd[1]);
			}
			close_hdocs_in_pipeline(seq, n);
			free(seq);
			return (sh->last_status = 1);
		}
		if (pid == 0)
		{
			// ---- child ----
			if (prev_read != -1 && dup2(prev_read, STDIN_FILENO) < 0)
				_exit(1);
			if (need_pipe_out && dup2(pipefd[1], STDOUT_FILENO) < 0)
				_exit(1);
			if (prev_read != -1)
				close(prev_read);
			if (need_pipe_out)
			{
				close(pipefd[0]);
				close(pipefd[1]);
			}
			if (apply_redirs(seq[i]) < 0)
				_exit(1);
			if (seq[i]->is_builtin)
			{
				st = run_builtin(seq[i], sh);
				_exit(st & 0xFF);
			}
			exec_external(seq[i]->argv, sh->envp);
			_exit(errno == ENOENT ? 127 : 126);
		}
		// ---- parent ----
		last_pid = pid;
		if (prev_read != -1)
			close(prev_read);
		if (need_pipe_out)
		{
			close(pipefd[1]);
			prev_read = pipefd[0];
		}
		else
		{
			prev_read = -1;
		}
	}
	// heredoc FD は親側でも閉じる（子側には dup 済み）
	close_hdocs_in_pipeline(seq, n);
	if (reap_pipeline_and_set_last_status(last_pid, sh) < 0)
		sh->last_status = 1;
	free(seq);
	return (sh->last_status);
}
