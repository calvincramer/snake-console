CFLAGS := -Wall -Werror

## Debug flag
ifneq ($(D),1)
	CFLAGS	+= -O3
else
	CFLAGS	+= -O0
	CFLAGS	+= -g
endif

default: snake

snake: snake.cpp
	g++ $(CFLAGS) -pthread $< -o snake

clean:
	rm -f snake


# g++ $(CFLAGS) -Laudio/audiere/src -pthread $< -o snake