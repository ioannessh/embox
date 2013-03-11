/**
 * @file
 * @brief
 *
 * @date 13.09.11
 * @author Anton Kozlov
 */

#ifndef CMD_SHELL_H_
#define CMD_SHELL_H_

#include <errno.h>
#include <util/array.h>
#include <framework/mod/self.h>

extern int shell_line_input(const char *line);

typedef void (*shell_run_ft)(void);
typedef int  (*shell_exec_ft)(const char *line);

struct shell {
	const char *  name;
	shell_run_ft  run;
	shell_exec_ft exec;
};

static inline int shell_run(const struct shell *shell) {

	if (!shell) {
		return -EBADF;
	}

	if (!shell->run) {
		return -EBADF;
	}

	shell->run();

	return 0;
}

static inline int shell_exec(const struct shell *shell, const char *line) {
	if (!shell) {
		return -EBADF;
	}

	if (!shell->exec) {
		return -EBADF;
	}

	return shell->exec(line);
}

extern const struct shell *shell_lookup(const char *shell_name);

extern const struct shell *shell_any(void);

#define SHELL_DEF(...) \
	extern const struct shell __shell_registry[]; \
	ARRAY_SPREAD_ADD(__shell_registry, __VA_ARGS__)

#ifdef __CDT_PARSER__

# undef  SHELL_DEF
# define SHELL_DEF(...) \
	static struct shell MACRO_GUARD(shell) = __VA_ARGS__

#endif /* __CDT_PARSER__ */



#endif
