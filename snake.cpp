#include <iostream>     // C++ version io
#include <stdio.h>      // C version io
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>     // STDOUT_FILENO definition

int main()
{
	std::cout << "Hello snake\n";
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	std::cout << "Console width:  " << size.ws_row << std::endl;
	std::cout << "Console height: " << size.ws_col << std::endl;

	return 0;
}
