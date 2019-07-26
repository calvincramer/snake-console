// Citations for helpful stuff:
// Keyboard input:
// https://www.linuxquestions.org/questions/programming-9/game-programming-non-blocking-key-input-740422/

// #include <iostream>     // C++ version io
#include <stdio.h>      // C version io
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>     // STDOUT_FILENO definition, sleeping
#include <chrono>       // for sleeping
#include <thread>       // for sleeping
#include <iostream>
#include <limits>
#include <ios>
#include <thread>

//#include <conio.h> // Windows based console io (for arrow keys)
//curses for linux?
#include <fcntl.h>

#define BORDER_CHAR ' '
#define POINTS_FOR_FRUIT 10
#define GAME_TICK_MS 200
#define KB_SLEEP_MS 50
#define DEFAULT_TERMINAL_ROWS 20
#define DEFAULT_TERMINAL_COLS 20

// ASCII values for keyboard input
#define KEY_w_ASCII 119
#define KEY_a_ASCII 97
#define KEY_s_ASCII 115
#define KEY_d_ASCII 100
#define KEY_W_ASCII 87
#define KEY_A_ASCII 65
#define KEY_S_ASCII 83
#define KEY_D_ASCII 68
#define KEY_SPACE_ASCII 32
#define KEY_UP false
#define KEY_DOWN true

// Global variables
unsigned short term_rows = DEFAULT_TERMINAL_ROWS;
unsigned short term_cols = DEFAULT_TERMINAL_COLS;
bool key_w_pressed = KEY_UP;
bool key_a_pressed = KEY_UP;
bool key_s_pressed = KEY_UP;
bool key_d_pressed = KEY_UP;
bool key_space_pressed = KEY_UP;
bool end_kb_thread = false;



    // TODO: print and keep track of SCORE
    // TODO: USER KEY INPUT
    // TODO: speed up game the more fruit gotten
    // Allocate game field buffer?
    // First trying to just use printf instead of using buffer



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



void game_loop()
{
    // Game loop
    // while true
        // gather input keys
        // move snake
        // draw
        // clear key variables

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

        // Clear keys pressed
        set_location(10, 10);
        printf(" ");
        set_location(11, 9);
        printf(" ");
        set_location(11, 11);
        printf(" ");
        set_location(12, 10);
        printf(" ");

        // Display input
        set_location(30, 30);
        printf((key_w_pressed) ? "W" : "w");
        printf((key_a_pressed) ? "A" : "a");
        printf((key_s_pressed) ? "S" : "s");
        printf((key_d_pressed) ? "D" : "d");
        printf(" ");
        printf((key_space_pressed) ? "SPACE" : "space");

        clear_color();
        hide_cursor();
        flush();

        // Clear keyboard inputs
        key_w_pressed = KEY_UP;
        key_a_pressed = KEY_UP;
        key_s_pressed = KEY_UP;
        key_d_pressed = KEY_UP;
        key_space_pressed = KEY_UP;

        // Sleep
        sleep(GAME_TICK_MS);
    }
}

void keyboard_thread_loop()
{
    // Set up stdin for keyboard input
    struct termios oldt, newt;
    int oldf;
    // Set stdin into noncannonical and to not echo characters
    tcgetattr(STDIN_FILENO, &oldt);             // Get current terminal parameters
    newt = oldt;                                // Copy oldt to newt
    newt.c_lflag &= ~(ICANON | ECHO);           // c_lflag control local modes. Sets the local mode to be noncannoical and to not echo characters
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);    // Set STDIN file descriptor to have new terminal parameters effective immediately
    // Set stdin to be non-blocking
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);     // Get file access mode from STDIN, 0 is default third argument and isn't needed(?)
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);    // Sets to non-blocking mode

    int ch;

    while (!end_kb_thread) {
        // Gather input
        // key_w_pressed = KEY_UP;
        // key_a_pressed = KEY_UP;
        // key_s_pressed = KEY_UP;
        // key_d_pressed = KEY_UP;
        // key_space_pressed = KEY_UP;
        do {
            ch = getchar();
            if      (ch == KEY_W_ASCII || ch == KEY_w_ASCII)  key_w_pressed = KEY_DOWN;
            else if (ch == KEY_A_ASCII || ch ==  KEY_a_ASCII) key_a_pressed = KEY_DOWN;
            else if (ch == KEY_S_ASCII || ch ==  KEY_s_ASCII) key_s_pressed = KEY_DOWN;
            else if (ch == KEY_D_ASCII || ch ==  KEY_d_ASCII) key_d_pressed = KEY_DOWN;
            else if (ch == KEY_SPACE_ASCII)            key_space_pressed = KEY_DOWN;
        } while (ch != EOF);
        // Wait a little
        sleep(KB_SLEEP_MS);
    }

    // Reset stdin to original state
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);    // Set the terminal parameters to original (TCSANOW forces change now)
    fcntl(STDIN_FILENO, F_SETFL, oldf);         // Set flag for STDIN file descriptor
}

int main()
{
    // Launch keyboard listener thread
    std::thread kb_thread(keyboard_thread_loop);

    // Get and set terminal dimensions
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    term_rows = size.ws_row;
    term_cols = size.ws_col;

    // Clear screen
    clear_color();
    flush();
    printf("\033[2J");
    flush();
    sleep(500);

    // Print field
    print_field(term_cols, term_rows);
    hide_cursor();
    flush();
    sleep(1000);

    // Print snake
    // print_snake();
    // hide_cursor();
    // flush();
    // sleep(2000);

    game_loop();

    // End of game clear screen
    printf("\033[2J");  // clear
    flush();
    sleep(500);

    // Join with keyboard listener thread before exiting
    end_kb_thread = true;
    kb_thread.join();

	return 0;
}





















// UNUSED:

// Keyboard hit
// Credit to sd9: https://www.linuxquestions.org/questions/programming-9/game-programming-non-blocking-key-input-740422/
// Comments by Calvin
// Uses STDIN to accomplish task
int kbhit(void)
{
        struct termios oldt, newt;
        int ch;
        int oldf;

        // Set stdin into noncannonical and to not echo characters
        tcgetattr(STDIN_FILENO, &oldt);             // Get current terminal parameters
        newt = oldt;                                // Copy oldt to newt
        newt.c_lflag &= ~(ICANON | ECHO);           // c_lflag control local modes. Sets the local mode to be noncannoical and to not echo characters
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);    // Set STDIN file descriptor to have new terminal parameters effective immediately
        // Set stdin to be non-blocking
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);     // Get file access mode from STDIN, 0 is default third argument and isn't needed(?)
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);    // Sets to non-blocking mode

        ch = getchar();                             // Get a character

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);    // Set the terminal parameters to original (TCSANOW forces change now)
        fcntl(STDIN_FILENO, F_SETFL, oldf);         // Set flag for STDIN file descriptor

        if(ch != EOF) {
            ungetc(ch, stdin);                      // Puts the character back into STDIN if not EOF
            return 1;
        }

        return 0;
}

// Keyboard test
// Credit to sd9: https://www.linuxquestions.org/questions/programming-9/game-programming-non-blocking-key-input-740422/
void kbtest()
{
    char c;
    int v;
    while(!kbhit());
    c = getchar();
    v = (int)c;
    printf("\nInteger value=%d",v);
    switch(v)
    {
        case 27:printf("\nEsc Pressed");break;
        case 32:printf("\nSpace Pressed");break;
        case 10:printf("\nEnter Pressed");break;
        case 9:printf("\nTab Pressed");break;
        //case 72:printf("\nUp arrow Pressed");break;
        //case 80:printf("\nDown arrow Pressed");break;
        //case 75:printf("\nLeft arrow Pressed");break;
        //case 77:printf("\nRight arrow Pressed");break;
        default:printf("\n%c Pressed",c);break;
    }//switch
}
