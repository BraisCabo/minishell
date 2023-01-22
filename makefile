test.out:
	gcc -Wall -Wextra -o minishell minishell.c libparser.a -static
