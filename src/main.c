#include "minishell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char	**make_argv(int n, const char *a0, const char *a1, const char *a2,
		const char *a3, const char *a4)
{
	char	**v;

	(void)n; // 必要なら可変長に
	v = calloc(6, sizeof(char *));
	if (!v)
		return (NULL);
	v[0] = a0 ? strdup(a0) : NULL;
	v[1] = a1 ? strdup(a1) : NULL;
	v[2] = a2 ? strdup(a2) : NULL;
	v[3] = a3 ? strdup(a3) : NULL;
	v[4] = a4 ? strdup(a4) : NULL;
	v[5] = NULL;
	return (v);
}

// ---- redir リスト操作 -------------------------------------------------

// 末尾追加（順序保持 = 後勝ちの検証に都合が良い）
static t_redir	*push_redir_ex(t_redir **head, t_rtype kind, const char *arg,
		int fd_target)
{
	t_redir	*r;
	t_redir	*t;

	r = calloc(1, sizeof(*r));
	if (!r)
		return (NULL);
	// まずゼロ化してから個別に設定（ゴミ値防止）
	// calloc なので0初期化済みだが、方針としてここにコメントを残す
	r->kind = kind;
	r->arg = arg ? strdup(arg) : strdup("");
	r->quoted = 0;
	r->hdoc_fd = -1;                            // heredocは別途セット
	r->fd_target = (fd_target >= 0) ? fd_target // 左辺FDを明示
									: -1;       // 未指定は -1（デフォルト: 入力=0/出力=1）
	r->next = NULL;
	if (!*head)
	{
		*head = r;
	}
	else
	{
		t = *head;
		while (t->next)
			t = t->next;
		t->next = r;
	}
	return (r);
}

// 左辺未指定版（糖衣）
static inline t_redir	*push_redir(t_redir **head, t_rtype kind,
		const char *arg)
{
	return (push_redir_ex(head, kind, arg, -1));
}

static void	free_redirs(t_redir *r)
{
	t_redir	*n;

	while (r)
	{
		n = r->next;
		free(r->arg);
		// r->hdoc_fd は収集側のポリシーに合わせて閉じる/閉じないを決める
		free(r);
		r = n;
	}
}

static void	free_argv(char **v)
{
	if (!v)
		return ;
	for (int i = 0; v[i]; i++)
		free(v[i]);
	free(v);
}

// ---- テストケース -----------------------------------------------------

// 1) grep b < infile > outfile
static void	build_case1_grep(t_ast *out_ast)
{
	memset(out_ast, 0, sizeof(*out_ast));
	out_ast->type = AST_CMD;
	out_ast->as.cmd.argv = make_argv(2, "grep", "b", NULL, NULL, NULL);
	out_ast->as.cmd.redirs = NULL;
	push_redir(&out_ast->as.cmd.redirs, R_IN, "infile");   // デフォルトで fd 0
	push_redir(&out_ast->as.cmd.redirs, R_OUT, "outfile"); // デフォルトで fd 1
	out_ast->as.cmd.is_builtin = 0;
}

// 2) /bin/ls -l
static void	build_case2_ls(t_ast *out_ast)
{
	memset(out_ast, 0, sizeof(*out_ast));
	out_ast->type = AST_CMD;
	out_ast->as.cmd.argv = make_argv(2, "/bin/ls", "-l", NULL, NULL, NULL);
	out_ast->as.cmd.redirs = NULL;
	out_ast->as.cmd.is_builtin = 0;
}

// 3) echo -n hello > out.txt（外部 echo 検証）
static void	build_case3_echo(t_ast *out_ast, int as_builtin)
{
	memset(out_ast, 0, sizeof(*out_ast));
	out_ast->type = AST_CMD;
	if (as_builtin)
	{
		out_ast->as.cmd.argv = make_argv(3, "echo", "-n", "hello", NULL, NULL);
		out_ast->as.cmd.is_builtin = 1;
	}
	else
	{
		out_ast->as.cmd.argv = make_argv(3, "/bin/echo", "-n", "hello", NULL,
				NULL);
		out_ast->as.cmd.is_builtin = 0;
	}
	out_ast->as.cmd.redirs = NULL;
	push_redir(&out_ast->as.cmd.redirs, R_OUT, "out.txt"); // 1 に向く
}

// 4) 左辺FD指定の検証：grep b NO_SUCH_FILE < infile > out.txt 2>> err.txt
//    → stdout は out.txt、stderr は err.txt（追記）
static void	build_case4_fd_targets(t_ast *out_ast)
{
	memset(out_ast, 0, sizeof(*out_ast));
	out_ast->type = AST_CMD;
	// 存在しないファイルを引数に入れて stderr を必ず発生させる
	out_ast->as.cmd.argv = make_argv(3, "grep", "b", "NO_SUCH_FILE", NULL,
			NULL);
	out_ast->as.cmd.is_builtin = 0;
	out_ast->as.cmd.redirs = NULL;
	push_redir(&out_ast->as.cmd.redirs, R_IN, "infile");
	// < infile      → fd 0
	push_redir(&out_ast->as.cmd.redirs, R_OUT, "out.txt");
	// > out.txt     → fd 1
	push_redir_ex(&out_ast->as.cmd.redirs, R_APP, "err.txt", 2);
	// 2>> err.txt   → fd 2
}

// 5) 複数リダイレクト（後勝ち）の検証：echo -n hello > first.txt > second.txt 2> err.txt
//    → 出力は最終的に second.txt のみ（first.txt は空 or 途中まで）
static void	build_case5_multi_redirs(t_ast *out_ast)
{
	memset(out_ast, 0, sizeof(*out_ast));
	out_ast->type = AST_CMD;
	out_ast->as.cmd.argv = make_argv(3, "/bin/echo", "-n", "hello", NULL, NULL);
	out_ast->as.cmd.is_builtin = 0;
	out_ast->as.cmd.redirs = NULL;
	push_redir(&out_ast->as.cmd.redirs, R_OUT, "first.txt");
	// > first.txt   → fd 1
	push_redir(&out_ast->as.cmd.redirs, R_OUT, "second.txt");
	// > second.txt  → fd 1（後勝ち）
	push_redir_ex(&out_ast->as.cmd.redirs, R_OUT, "err.txt", 2);
	// 2> err.txt    → fd 2
}

// ---- main -------------------------------------------------------------

int	main(int argc, char **argv, char **envp)
{
	t_shell	sh;
	t_ast	ast;
	int		which;
	int		status;

	ft_memset(&sh, 0, sizeof sh);
	ft_memset(&ast, 0, sizeof ast);
	which = 1;
	status = 0;
	sh.envp = envp;
	sh.last_status = 0;
	sh.interactive = 0;
	if (argc >= 2)
		which = atoi(argv[1]);
	switch (which)
	{
	case 1: // grep b < infile > outfile
		build_case1_grep(&ast);
		break ;
	case 2: // /bin/ls -l
		build_case2_ls(&ast);
		break ;
	case 3: // echo -n hello > out.txt
		build_case3_echo(&ast, /*as_builtin=*/0);
		break ;
	case 4: // FD 指定のリダイレクト（2>> err.txt 等）
		build_case4_fd_targets(&ast);
		break ;
	case 5: // 複数リダイレクト（後勝ち）
		build_case5_multi_redirs(&ast);
		break ;
	default:
		fprintf(stderr, "usage: %s [1|2|3|4|5]\n", argv[0]);
		fprintf(stderr, "  1: grep b < infile > outfile\n");
		fprintf(stderr, "  2: /bin/ls -l\n");
		fprintf(stderr, "  3: echo -n hello > out.txt\n");
		fprintf(stderr,
			"  4: grep b NO_SUCH_FILE < infile > out.txt 2>> err.txt\n");
		fprintf(stderr,
			"  5: echo-n hello > first.txt > second.txt 2> err.txt\n");
		return (2);
	}
	status = run_single_command(&ast.as.cmd, &sh);
	printf("[status] %d\n", status);
	// cleanup
	free_argv(ast.as.cmd.argv);
	free_redirs(ast.as.cmd.redirs);
	return (status != 0);
}
