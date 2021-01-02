CFLAGS := -Wall -Werror
CPP_VERSION := -std=c++17

## Debug flag
ifneq ($(D),1)
	CFLAGS	+= -O3
else
	CFLAGS	+= -O0
	CFLAGS	+= -g
endif

default: snake

snake: snake.cpp
	g++ $(CPP_VERSION) $(CFLAGS) -pthread $< -o snake

clean:
	rm -f snake
