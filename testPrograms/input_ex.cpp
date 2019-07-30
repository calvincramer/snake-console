#include <iostream>     // C++ version io
// #include <stdio.h>      // C version io
// #include <sys/ioctl.h>
// #include <termios.h>
// #include <unistd.h>     // STDOUT_FILENO definition, sleeping
// #include <chrono>       // for sleeping
// #include <thread>       // for sleeping
// #include <iostream>
// #include <limits>
// #include <ios>
// #include <thread>
// #include <cstdint>      // for integer types
// #include <deque>
// #include <ctime>
// #include <limits>

// //#include <conio.h> // Windows based console io (for arrow keys)
// //curses for linux?
// #include <fcntl.h>


int main()
{
    std::cout << "Enter something: ";
    int val = std::cin.get();
    std::cout << "You entered: " << val << std::endl;

    return 0;
}
