#include "minishell.h"

static char	**make_argv(int n, const char *a0, const char *a1, const char *a2,
		const char *a3, const char *a4)
{
	char	**v;

	(void)n; // 呼び方簡略化のため。必要なら n を使って動的化してもOK
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

// redir を末尾に追加（単方向リスト）
static t_redir	*push_redir(t_redir **head, t_rtype kind, const char *arg)
{
	t_redir	*r;
	t_redir	*t;

	r = calloc(1, sizeof(*r));
	if (!r)
		return (NULL);
	r->kind = kind;
	r->arg = arg ? strdup(arg) : strdup("");
	r->quoted = 0;
	r->next = NULL;
	if (!*head)
		*head = r;
	else
	{
		t = *head;
		while (t->next)
			t = t->next;
		t->next = r;
	}
	return (r);
}

static void	free_redirs(t_redir *r)
{
	t_redir	*n;

	while (r)
	{
		n = r->next;
		free(r->arg);
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
static void	build_case1_grep(t_ast *out_ast)
{
	memset(out_ast, 0, sizeof(*out_ast));
	out_ast->type = AST_CMD;
	out_ast->as.cmd.argv = make_argv(2, "grep", "b", NULL, NULL, NULL);
	out_ast->as.cmd.redirs = NULL;
	push_redir(&out_ast->as.cmd.redirs, R_IN, "infile");
	push_redir(&out_ast->as.cmd.redirs, R_OUT, "outfile");
	out_ast->as.cmd.is_builtin = 0; // 外部コマンド
}

static void	build_case2_ls(t_ast *out_ast)
{
	memset(out_ast, 0, sizeof(*out_ast));
	out_ast->type = AST_CMD;
	out_ast->as.cmd.argv = make_argv(2, "/bin/ls", "-l", NULL, NULL, NULL);
	out_ast->as.cmd.redirs = NULL;  // リダイレクトなし
	out_ast->as.cmd.is_builtin = 0; // 外部コマンド
}

static void	build_case3_echo(t_ast *out_ast, int as_builtin)
{
	memset(out_ast, 0, sizeof(*out_ast));
	out_ast->type = AST_CMD;
	if (as_builtin)
	{
		out_ast->as.cmd.argv = make_argv(3, "echo", "-n", "hello", NULL, NULL);
		out_ast->as.cmd.is_builtin = 1; // 親実行の分岐を持っているなら 1
	}
	else
	{
		out_ast->as.cmd.argv = make_argv(3, "/bin/echo", "-n", "hello", NULL,
				NULL);
		out_ast->as.cmd.is_builtin = 0; // 外部 echo として
	}
	out_ast->as.cmd.redirs = NULL;
	push_redir(&out_ast->as.cmd.redirs, R_OUT, "out.txt");
}

int	main(int argc, char **argv, char **envp)
{
	t_shell sh = {0};
	t_ast ast = {0};
	int which = 1;
	int status = 0;

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
	case 3: // echo -n hello > out.txt（ビルトイン or 外部 echo）
		build_case3_echo(&ast, /*as_builtin=*/0);
		break ;
	default:
		fprintf(stderr, "usage: %s [1|2|3]\n", argv[0]);
		fprintf(stderr, "  1: grep b < infile > outfile\n");
		fprintf(stderr, "  2: /bin/ls -l\n");
		fprintf(stderr, "  3: echo -n hello > out.txt (外部 echo で検証)\n");
		return (2);
	}

	// 実行：単一コマンドの入口
	status = run_single_command(&ast.as.cmd, &sh);
	printf("[status] %d\n", status);

	// 後片付け
	free_argv(ast.as.cmd.argv);
	free_redirs(ast.as.cmd.redirs);
	return (status != 0);
}