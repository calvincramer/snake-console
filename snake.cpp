// #include <iostream>     // C++ version io
#include <stdio.h>      // C version io
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>     // STDOUT_FILENO definition, sleeping
#include <chrono>       // for sleeping
#include <thread>       // for sleeping

#define BORDER_CHAR ' '
#define POINTS_FOR_FRUIT 10
#define GAME_TICK_MS 200

/*
ANSI escape codes
http://www.termsys.demon.co.uk/vtansi.htm

ex: printf("\033[Hthis is the start of the terminal")

Remember home is the first column and first row, so
\033[H is the same as \033[1;1H


\033[H      Home (top left of terminal)
\033[5;5H   Moves cursor to 5th line and 5th column
\033[5;5f   Moves cursor to 5th line and 5th column
\033[2J     Clear the entire screen and move cursor to home

*/

void sleep(int ms)
{
    usleep(ms * 1000);
    //std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void flush()
{
    fflush(stdout);
}

void blue()
{
    printf("\033[44m");
}

void green()
{
    printf("\033[42m");
}

void red()
{
    printf("\033[41m");
}

void clear_color()
{
    printf("\033[0m");
}

void hide_cursor()
{
    printf("\033[0;0H");
}

void print_field(int width, int height)
{
    std::string bar (width, BORDER_CHAR);
    const char* bar_cstr = bar.c_str();

    printf("\033[H");   // Home
    blue();

    printf("%s", bar_cstr);
    for (int y = 2; y <= height - 1; y++) {
        printf("\033[%d;1H", y);    // Move to yth column
        printf("%c", BORDER_CHAR);
        printf("\033[%d;%dH", y, width);    // Right side
        printf("%c", BORDER_CHAR);
    }
    printf("\033[%d;1H", height);
    printf("%s", bar_cstr);
    clear_color();
}

void print_snake()
{
    printf("\033[10;10H");
    green();
    printf("     ");
    clear_color();
}

void set_location(int y, int x)
{
    printf("\033[%d;%dH", y, x);
}


int main()
{
	//std::cout << "Hello snake\n";
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    unsigned short term_rows = size.ws_row;
    unsigned short term_cols = size.ws_col;

    // Allocate game field buffer?
    // First trying to just use printf instead of using buffer

    // TODO: print and keep track of SCORE
    // TODO: USER KEY INPUT
    // TODO: SOUND
    // TODO: speed up game the more fruit gotten

    // Clear screen
    printf("\033[2J");
    flush();
    sleep(500);

    // Print field
    print_field(term_cols, term_rows);
    hide_cursor();
    flush();
    sleep(1000);

    // Print snake
    print_snake();
    hide_cursor();
    flush();
    sleep(2000);

    // Test animation loop
    for (int i = 3; i < term_rows; i++) {
        // Erase previous snake
        clear_color();
        set_location(i-1, 20);
        printf("      ");
        // Display
        green();
        set_location(i, 20);
        printf("      ");
        clear_color();
        hide_cursor();
        flush();
        // Sleep
        sleep(GAME_TICK_MS);
    }

    // End of game clear screen
    printf("\033[2J");  // clear
    flush();
    sleep(500);

	return 0;
}

// printf("\033[2J");  // clear
// printf("\033[20;0H");
// printf("Rows: %d, Cols: %d\n", term_rows, term_cols);
// flush();

// sleep(500);

// printf("\033[10;0H");
// printf("\033[32mThis bit is green. \033[0mIsn't that nice?\n");
// flush();

// sleep(500);

// printf("\033[H");
// printf("HOME");
// flush();

// sleep(500);

// printf("\033[5;5H");
// printf("THIS IS IN THE MIDDLE");
// flush();

// sleep(300);

// for (int i = 0; i < 64; i++) {
//     printf("\033[%d;%df", i, i);
//     printf("%d", i);
//     flush();
//     sleep(10);
// }

// sleep(300);
