# Project name
NAME	= webserver

# Directories
SRC_DIR	= src/
INC_DIR	= include/

# Source files
SRC		= main.cpp \
		  $(SRC_DIR)WebServer.cpp $(SRC_DIR)ClientData.cpp $(SRC_DIR)RequestHandler.cpp \
		  $(SRC_DIR)config/Block.cpp        $(SRC_DIR)config/Directive.cpp    $(SRC_DIR)config/ServerConfig.cpp $(SRC_DIR)config/ConfigParser.cpp $(SRC_DIR)config/Location.cpp


# Objects
OBJ		= $(SRC:.cpp=.o)

# Compiler settings
CXX		= g++
CFLAGS	= -Wall -Wextra -std=c++11 -g3 -fsanitize=address -I$(INC_DIR) #You must write a HTTP server in C++ 98.
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