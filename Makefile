CC = gcc
#CFLAGS = -Wall -Wextra -Werror -std=c99 -g
CFLAGS = -std=c11 -g
LDFLAGS = -lm

NAME = ft_ping
SRC = src/main.c src/cli.c src/list.c
OBJ = $(SRC:.c=.o)

all: $(NAME)


$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)


fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

