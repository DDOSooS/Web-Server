# Project name
NAME	= webserver

# Directories
SRC_DIR	= src/
INC_DIR	= include/

# Source files
SRC		= main.cpp \
		  WebServer.cpp \
		  $(SRC_DIR)RequestHandler.cpp

# Objects
OBJ		= $(SRC:.cpp=.o)

# Compiler settings
CXX		= g++
CFLAGS	= -Wall -Wextra -std=c++11 -I$(INC_DIR)
LDFLAGS	= -pthread
RM		= rm -rf

# Rules
all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re