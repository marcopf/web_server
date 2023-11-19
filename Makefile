NAME    = webserv
HEADER  = ${NAME:.a=.h}
SRCS    = $(wildcard *.cpp) $(wildcard Socket/*.cpp) $(wildcard parser/*.cpp) $(wildcard RequestManager/*.cpp) $(wildcard utils/*.cpp)
OBJS    = ${SRCS:.cpp=.o}
FLAGS   = -std=c++98 -Wall  -Wextra 
OPTIONS = -c
CC      = g++
RM      = rm -f

RED		= \033[0;31m
GREEN	= \033[0;32m
YELLOW	= \033[1;33m
BLUE	= \033[1;34m
RESET	= \033[0;0m

# Rule to build object files
.cpp.o:
	$(CC) $(FLAGS) $(OPTIONS) $< -o $@

all: ${NAME}

	@printf "$(YELLOW)WEB SERVER  CREATED  SUCCESSUFULLY\n$(RESET)"
	@printf "$(BLUE)-------------------------------------------------------------------------\n$(RESET)"
	@printf "$(BLUE)██╗    ██╗███████╗██████╗     ███████╗███████╗██████╗ ██╗   ██╗███████╗██████╗ \n$(RESET)"
	@printf "$(BLUE)██║    ██║██╔════╝██╔══██╗    ██╔════╝██╔════╝██╔══██╗██║   ██║██╔════╝██╔══██╗\n$(RESET)"
	@printf "$(BLUE)██║ █╗ ██║█████╗  ██████╔╝    ███████╗█████╗  ██████╔╝██║   ██║█████╗  ██████╔╝\n$(RESET)"
	@printf "$(BLUE)██║███╗██║██╔══╝  ██╔══██╗    ╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██╔══╝  ██╔══██╗\n$(RESET)"
	@printf "$(BLUE)╚███╔███╔╝███████╗██████╔╝    ███████║███████╗██║  ██║ ╚████╔╝ ███████╗██║  ██║\n$(RESET)"
	@printf "$(BLUE) ╚══╝╚══╝ ╚══════╝╚═════╝     ╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝\n$(RESET)"
	@printf "$(BLUE)                                                                               \n$(RESET)"
	@printf "$(YELLOW)Made with love and bestemmie by Afraccal e Mpaterno.\n$(RESET)"
	@printf "$(BLUE)-------------------------------------------------------------------------\n$(RESET)"

${NAME}: ${OBJS}
	$(CC) $(FLAGS) ${OBJS} -o ${NAME}

clean:
	${RM} ${OBJS}

fclean: clean
	${RM} ${NAME}

re: fclean all

.PHONY: all clean fclean re libft