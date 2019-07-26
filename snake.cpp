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
#include <cstdint>      // for integer types
#include <deque>

//#include <conio.h> // Windows based console io (for arrow keys)
//curses for linux?
#include <fcntl.h>


    // TODO: speed up game the more fruit gotten
    // First trying to just use printf instead of using buffer
    // TODO: would be better to have no border and make whole console the playing field


#define BORDER_CHAR 'X'
#define POINTS_FOR_FRUIT 10
#define START_GAME_TICK_MS 200
#define KB_SLEEP_MS 20
#define DEFAULT_TERMINAL_ROWS 20
#define DEFAULT_TERMINAL_COLS 20
#define INIT_SNAKE_LENGTH 5
#define DIR_RGHT 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_UP   3

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

void fore_black()   { printf("\033[30m");  }
void fore_red()     { printf("\033[31m");  }
void fore_green()   { printf("\033[32m");  }
void fore_yellow()  { printf("\033[33m");  }
void fore_blue()    { printf("\033[34m");  }
void fore_magenta() { printf("\033[35m");  }
void fore_cyan()    { printf("\033[36m");  }
void fore_white()   { printf("\033[37m");  }
void back_black()   { printf("\033[40m");  }
void back_red()     { printf("\033[41m");  }
void back_green()   { printf("\033[42m");  }
void back_yellow()  { printf("\033[43m");  }
void back_blue()    { printf("\033[44m");  }
void back_magenta() { printf("\033[45m");  }
void back_cyan()    { printf("\033[46m");  }
void back_white()   { printf("\033[47m");  }
void clear_color()  { printf("\033[0m");   }
void hide_cursor()  { printf("\033[0;0H"); }
void flush()        { fflush(stdout);      }
void set_location(int y, int x) { printf("\033[%d;%dH", y, x); }
void sleep(int ms) { usleep(ms * 1000);   }

class Point {
 public:
    uint16_t x;
    uint16_t y;
    Point() {
        this->x = 0;
        this->y = 0;
    }
    Point(unsigned short y, unsigned short x) {
        this->x = x;
        this->y = y;
    }
    bool operator==(const Point& other) {
        return this->x == other.x && this->y == other.y;
    }
};

#define NO_FRUIT Point(65535, 65535)

class Snake {
 public:
    std::deque<Point> segments;  // Head of deque is head of snake
    Snake(Point term_dim) {
        Point term_mid (term_dim.y / 2, term_dim.x / 2);
        for (int i = 0; i < INIT_SNAKE_LENGTH; i++) {
            Point temp_point (term_mid.y, term_mid.x + i);
            this->segments.push_front(temp_point);
        }
    }
    uint16_t length() {
        return this->segments.size();
    }
    bool point_on_snake(Point p) {
        for (Point& pnt : this->segments)
            if (pnt == p) return true;
        return false;
    }
    void move(Point p, bool collected_fruit) {
        this->segments.push_front(p);
        if (!collected_fruit) {
            // Clear the last section of the snake on the screen (otherwise tail wouldn't stay on screen forever)
            clear_color();
            set_location(this->segments.back().y, this->segments.back().x);
            printf(" ");
            this->segments.pop_back();
        }
    }
    uint16_t head_x() {
        return this->segments.front().x;
    }
    uint16_t head_y() {
        return this->segments.front().y;
    }
};

// Global variables
uint16_t game_tick_ms = START_GAME_TICK_MS;
uint16_t term_rows = DEFAULT_TERMINAL_ROWS;
uint16_t term_cols = DEFAULT_TERMINAL_COLS;
bool key_w_pressed = KEY_UP;
bool key_a_pressed = KEY_UP;
bool key_s_pressed = KEY_UP;
bool key_d_pressed = KEY_UP;
bool key_space_pressed = KEY_UP;
bool end_kb_thread = false;
Point fruit = NO_FRUIT;
uint64_t score = 0;
Snake snek(Point(term_rows, term_cols));
uint8_t last_dir = DIR_RGHT;
bool exit_condition = false;

void print_snake()
{
    back_green();
    for (Point& p : snek.segments) {
        set_location(p.y, p.x);
        printf(" ");
    }
    clear_color();
}

void print_field(int width, int height)
{
    std::string bar (width, BORDER_CHAR);
    const char* bar_cstr = bar.c_str();

    printf("\033[H");   // Home
    back_blue();

    printf("%s", bar_cstr);
    for (int y = 2; y <= height - 1; y++) {
        printf("\033[%d;1H", y);    // Move to yth column
        printf("%c", BORDER_CHAR);
        printf("\033[%d;%dH", y, width);    // Right side
        printf("%c", BORDER_CHAR);
    }
    printf("\033[%d;1H", height);
    printf("%s", bar_cstr);
    // Print score
    printf("\033[%d;%dH", height, width / 2);
    printf("Score: %lu", score);

    // Temporary end game region
    std::string finish_bar ((width / 4) + 1, 'F');
    const char* finish_bar_cstr = finish_bar.c_str();
    back_red();
    fore_cyan();
    for (int y = height * 3 / 4; y < height; y++) {
        set_location(y, width * 3 / 4);
        printf("%s", finish_bar_cstr);
    }
    clear_color();


}


void update_movement_direction() {
    uint8_t num_directions_down = 0;
    if (key_w_pressed) ++num_directions_down;
    if (key_a_pressed) ++num_directions_down;
    if (key_s_pressed) ++num_directions_down;
    if (key_d_pressed) ++num_directions_down;

    if (num_directions_down == 0 || num_directions_down == 4)
        return; // No change in movement
    else if (num_directions_down == 1) {
        if      (key_w_pressed) last_dir = DIR_UP;
        else if (key_a_pressed) last_dir = DIR_LEFT;
        else if (key_s_pressed) last_dir = DIR_DOWN;
        else if (key_d_pressed) last_dir = DIR_RGHT;
        return;
    }
    else if (num_directions_down == 3) {
        if (key_w_pressed && key_s_pressed) {
            if (key_a_pressed) last_dir = DIR_LEFT;
            else               last_dir = DIR_RGHT;
        } else {    // a and d pressed
            if (key_w_pressed) last_dir = DIR_UP;
            else               last_dir = DIR_DOWN;
        }
        return;
    }
    // num directions pressed is 2
    if ( (key_w_pressed && key_s_pressed) || (key_a_pressed && key_d_pressed) ) return; // No change
    // Be nice and try to not move snake in opposite direction as the last direction (not onto the neck of the snake)
    if (last_dir == DIR_UP || last_dir == DIR_DOWN) {
        if (key_a_pressed) last_dir = DIR_LEFT;
        else               last_dir = DIR_RGHT; // d is pressed
    } else {    // last direction is left or right
        if (key_w_pressed) last_dir = DIR_UP;
        else               last_dir = DIR_DOWN; // s is pressed
    }

}

void move_snake() {
    update_movement_direction();
    // Now move snake in direction of last_dir
    switch (last_dir) {
        case DIR_UP:   snek.move(Point(snek.head_y() - 1, snek.head_x()), false); break;
        case DIR_LEFT: snek.move(Point(snek.head_y(), snek.head_x() - 1), false); break;
        case DIR_DOWN: snek.move(Point(snek.head_y() + 1, snek.head_x()), false); break;
        case DIR_RGHT: snek.move(Point(snek.head_y(), snek.head_x() + 1), false); break;
    }

    // Check if in exit box
    if (snek.head_y() >= (term_rows * 3) / 4 && snek.head_x() >= (term_cols * 3) / 4)
        exit_condition = true;
}

void game_loop()
{
    while (!exit_condition) {
        move_snake();
        print_snake();

        // Display input
        set_location(30, 30);
        clear_color();
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
        sleep(game_tick_ms);
    }
}

// Credit to sd9: https://www.linuxquestions.org/questions/programming-9/game-programming-non-blocking-key-input-740422/
// Comments by Calvin
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
        do {
            ch = getchar();
            if      (ch == KEY_W_ASCII || ch ==  KEY_w_ASCII) key_w_pressed = KEY_DOWN;
            else if (ch == KEY_A_ASCII || ch ==  KEY_a_ASCII) key_a_pressed = KEY_DOWN;
            else if (ch == KEY_S_ASCII || ch ==  KEY_s_ASCII) key_s_pressed = KEY_DOWN;
            else if (ch == KEY_D_ASCII || ch ==  KEY_d_ASCII) key_d_pressed = KEY_DOWN;
            else if (ch == KEY_SPACE_ASCII)                   key_space_pressed = KEY_DOWN;
        } while (ch != EOF);
        // Wait a little
        sleep(KB_SLEEP_MS);
        // Keys will be reset to down in draw loop
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

    // Initialize snake again now that we know terminal dimensions
    snek = Snake(Point(term_rows, term_cols));

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
