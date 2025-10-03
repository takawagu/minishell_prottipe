#include "minishell.h"

int	exec_entry(t_ast *root, t_shell *sh)
{
	if (!root)
		return (sh->last_status);
	if (root->type == AST_CMD)
		return (run_single_command(&root->as.cmd, sh));
	else
		return (run_pipeline(root, sh));
}
