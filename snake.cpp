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
#include <ctime>
#include <limits>
#include <fstream>
#include <fcntl.h>
#include <cstring>
#include <iomanip>
#include <cctype>
//#include <conio.h> // Windows based console io (for arrow keys)

#define BORDER_CHAR '.'             // Border character
#define POINTS_FOR_FRUIT 10
#define GAME_TICK_MS_START 100      // Snake moves 10 tiles per second at start
#define KB_SLEEP_MS 20              // Keyboard sleep
#define DEFAULT_TERMINAL_ROWS 20
#define DEFAULT_TERMINAL_COLS 20
#define INIT_SNAKE_LENGTH 7
#define FRUITS_PER_SPEEDUP 5
#define SPEEDUP_MS 5                // Amount to speedup game by
#define DEATH_SCREEN_TICK 50
#define MIN_GAME_TICK 30
#define MAX_HIGHSCORES 10
#define HIGHSCORE_NAME_LEN 3
#define DIR_RGHT 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_UP   3
const std::string highscore_file = "hs.txt";

// ASCII values for keyboard input
#define KEY_w_ASCII 119
#define KEY_a_ASCII 97
#define KEY_s_ASCII 115
#define KEY_d_ASCII 100
#define KEY_W_ASCII 87
#define KEY_A_ASCII 65
#define KEY_S_ASCII 83
#define KEY_D_ASCII 68
#define KEY_UP false
#define KEY_DOWN true
#define BACKSPACE 8
#define DELETE 127
#define ENTER 10        // newline

struct HighscoreEntry {
    char name[HIGHSCORE_NAME_LEN + 1];  // Plus one for null terminal
    uint64_t score;
};

// Global variables
uint16_t game_tick_ms  = GAME_TICK_MS_START;
uint16_t term_rows     = DEFAULT_TERMINAL_ROWS;
uint16_t term_cols     = DEFAULT_TERMINAL_COLS;
bool key_w_pressed     = KEY_UP;
bool key_a_pressed     = KEY_UP;
bool key_s_pressed     = KEY_UP;
bool key_d_pressed     = KEY_UP;
bool end_kb_thread     = false;
uint64_t score         = 0;
uint8_t last_dir       = DIR_RGHT;
bool exit_condition    = false;
uint32_t fruits_coll   = 0;       // will get reset to zero when speeding up
uint32_t fruits_total  = 0;
uint32_t speed_level   = 0;
HighscoreEntry highscores[MAX_HIGHSCORES];

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
void cursor_back()  { printf("\033[D");    }
void blink_text()   { printf("\033[5m");   }
void clear_screen() { printf("\033[2J");   }
void flush()        { fflush(stdout);      }
void set_location(int y, int x) { printf("\033[%d;%dH", y, x); }
void sleep(int ms) { usleep(ms * 1000);   }

class Point {
 public:
    uint16_t x;
    uint16_t y;

    Point()
    {
        this->x = 0;
        this->y = 0;
    }

    Point(unsigned short y, unsigned short x)
    {
        this->x = x;
        this->y = y;
    }

    bool operator==(const Point& other)
    {
        return this->x == other.x && this->y == other.y;
    }
};
#define NO_FRUIT Point(65535, 65535)
Point fruit = NO_FRUIT;

class Snake {
 public:
    std::deque<Point> segments;  // Head of deque is head of snake
    uint16_t head_x() { return this->segments.front().x; }
    uint16_t head_y() { return this->segments.front().y; }
    Point& head()     { return this->segments.front();   }

    Snake(Point term_dim)
    {
        Point term_mid (term_dim.y / 2, (term_dim.x / 2) - (INIT_SNAKE_LENGTH / 2));
        for (int i = 0; i < INIT_SNAKE_LENGTH; i++) {
            Point temp_point (term_mid.y, term_mid.x + i);
            this->segments.push_front(temp_point);
        }
    }

    uint16_t length()
    {
        return this->segments.size();
    }

    bool point_on_snake(Point p)
    {
        for (Point& pnt : this->segments)
            if (pnt == p) return true;
        return false;
    }

    bool snake_head_collides_body()
    {
        for (uint32_t i = 1; i < this->segments.size(); i++)
            if (this->segments[0] == this->segments[i])
                return true;
        return false;
    }

    void move(Point p)
    {
        this->segments.push_front(p);
        bool collected_fruit = (p == fruit);
        if (!collected_fruit) {
            // Clear the last section of the snake on the screen (otherwise tail wouldn't stay on screen forever)
            clear_color();  // clear color needed for background color
            set_location(this->segments.back().y, this->segments.back().x);
            // Check if need to reprint border
            if ( (this->segments.back().y == 1 || this->segments.back().y == term_rows) && this->segments.back().x % 2 == 1) {  // TODO replace with function
                printf("%c", BORDER_CHAR);
            } else {
                printf(" ");
            }
            this->segments.pop_back();
        }
    }
};
Snake snek(Point(term_rows, term_cols));

void print_snake()
{
    back_green();
    int seg_num = -1;
    for (Point& p : snek.segments) {
        set_location(p.y, p.x);
        if (seg_num == -1) {
            if (last_dir == DIR_UP || last_dir == DIR_DOWN)
                printf("|");
            else
                printf("-");
        } else {
            printf("o");
        }
        ++seg_num;
    }
}

void play_sound()
{
    printf("\a");
    // TODO: doesn't always work, and might sound like windows annoying sound
}

void write_default_highscore_file()
{
    std::ofstream hs_file(highscore_file);
    for (int i = 0; i < MAX_HIGHSCORES; i++)
        hs_file << "___ 0\n";
    hs_file.close();
}

void read_highscores()
{
    std::ifstream hs_file(highscore_file);
    if (!hs_file) {
        write_default_highscore_file();
        read_highscores();
        return;
    }
    // Read from file
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        hs_file.get(highscores[i].name, HIGHSCORE_NAME_LEN + 1, ' ');
        hs_file.get();  // Consume space
        hs_file >> highscores[i].score;
        hs_file.get();  // Consume newline
    }
    hs_file.close();
}

void write_highscores()
{
    std::ofstream hs_file(highscore_file);
    for (int i = 0; i < MAX_HIGHSCORES; i++)
        hs_file << highscores[i].name << " " << highscores[i].score << "\n";
    hs_file.close();
}

void wait_for_enter()
{
    std::cin.get();
}

void print_fruit()
{
    back_magenta();
    fore_red();
    blink_text();
    set_location(fruit.y, fruit.x);
    printf("O");
}

void print_score()
{
    fore_cyan();
    back_blue();
    set_location(term_rows, term_cols / 2);
    printf("Score: %lu", score);
}

void place_random_fruit()
{
    do {
        fruit.x = (rand() % term_cols) + 1;
        fruit.y = (rand() % term_rows) + 1;
    } while (snek.point_on_snake(fruit));
    print_fruit();
}

void print_field()
{
    int width = term_cols;
    int height = term_rows;
    std::string bar (width, BORDER_CHAR);
    for (uint16_t i = 1; i < bar.length(); i += 2)
        bar[i] = ' ';
    const char* bar_cstr = bar.c_str();

    clear_color();
    fore_white();
    set_location(1, 1);
    printf("%s", bar_cstr);

    for (uint16_t y = 2; y <= height - 1; y++) {
        set_location(y, 1);
        printf("%c", BORDER_CHAR);
        set_location(y, width);
        printf("%c", BORDER_CHAR);
    }

    set_location(height, 1);
    printf("%s", bar_cstr);
}

void update_movement_direction()
{
    uint8_t num_directions_down = 0;
    if (key_w_pressed) ++num_directions_down;
    if (key_a_pressed) ++num_directions_down;
    if (key_s_pressed) ++num_directions_down;
    if (key_d_pressed) ++num_directions_down;

    if (num_directions_down == 0 || num_directions_down == 4) {
        return; // No change in movement
    }
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

void move_snake()
{
    update_movement_direction();
    // Now move snake in direction of last_dir
    switch (last_dir) {
        case DIR_UP:   snek.move(Point(snek.head_y() - 1, snek.head_x())); break;
        case DIR_LEFT: snek.move(Point(snek.head_y(), snek.head_x() - 1)); break;
        case DIR_DOWN: snek.move(Point(snek.head_y() + 1, snek.head_x())); break;
        case DIR_RGHT: snek.move(Point(snek.head_y(), snek.head_x() + 1)); break;
    }
    // Check if hit fruit
    if (snek.head() == fruit) {
        score += POINTS_FOR_FRUIT;
        ++fruits_coll;
        ++fruits_total;
        if (fruits_coll >= FRUITS_PER_SPEEDUP) {
            // SPEED THE GAME UP!
            fruits_coll = 0;
            game_tick_ms = std::max(game_tick_ms - SPEEDUP_MS, MIN_GAME_TICK);
            ++speed_level;
        }
        play_sound();
        place_random_fruit();
        // extended snake handled by snek.move()
        print_score();
    }
    // Check if hit self or outside edge
    if (snek.head_x() < 1 || snek.head_x() > term_cols
     || snek.head_y() < 1 || snek.head_y() > term_rows
     || snek.snake_head_collides_body()) {
        exit_condition = true;
    }
}

void print_inputs()
{
    set_location(2, 2);
    clear_color();
    printf((key_w_pressed) ? "W" : "w");
    printf((key_a_pressed) ? "A" : "a");
    printf((key_s_pressed) ? "S" : "s");
    printf((key_d_pressed) ? "D" : "d");
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
        } while (ch != EOF);
        // Wait a little
        sleep(KB_SLEEP_MS);
        // Keys will be reset to down in draw loop
    }
    // Reset stdin to original state
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);    // Set the terminal parameters to original (TCSANOW forces change now)
    fcntl(STDIN_FILENO, F_SETFL, oldf);         // Set flag for STDIN file descriptor
}

void game_loop()
{
    std::thread kb_thread(keyboard_thread_loop);    // Launch keyboard listener thread
    while (!exit_condition) {
        move_snake();
        //print_inputs();
        print_snake();
        hide_cursor();

        // Clear keyboard inputs
        key_w_pressed = KEY_UP;
        key_a_pressed = KEY_UP;
        key_s_pressed = KEY_UP;
        key_d_pressed = KEY_UP;

        flush();
        // Sleep
        sleep(game_tick_ms);
    }
    // Join with keyboard listener thread when not controlling snake
    end_kb_thread = true;
    kb_thread.join();
}

// Prints the ith highscore at the current location, with the current terminal color
void print_highscore(int i) {
    if (i < 0 || i >= MAX_HIGHSCORES) return;
    if (i == 0) printf("#%d %c%c%c %lu", i+1, highscores[i].name[0], highscores[i].name[1], highscores[i].name[2], highscores[i].score);
    else        printf("%2d %c%c%c %lu", i+1, highscores[i].name[0], highscores[i].name[1], highscores[i].name[2], highscores[i].score);
}

void print_highscores(int y, int x)
{
    set_location(y, x);
    printf("HIGHSCORES");
    ++y;
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        set_location(y, x);
        print_highscore(i);
        ++y;
    }
}

void welcome_screen()
{
    clear_color();
    back_white();
    fore_black();
    clear_screen();
    // Set stdin to not echo any input
    // struct termios oldt, newt;
    // tcgetattr(STDIN_FILENO, &oldt);
    // newt = oldt;
    // newt.c_lflag &= ~(ECHO);
    // tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    set_location(4,4);
    printf("WELCOME TO SNAKE!");
    set_location(6,4);
    printf("wasd to move");
    print_highscores(8, 4);
    set_location(term_rows - 2, 4);
    printf("by Calvin Cramer - calvincramer@gmail.com");
    set_location(term_rows, term_cols / 2 - 8);
    printf("<enter to start>");

    // TODO display highscores on the welcome screen
    hide_cursor();
    flush();
    //sleep(2000);

    wait_for_enter();
}

void death_screen()
{
    auto color_diagonal = [](int i) {
        int start_y = i;
        int start_x = 1;
        if (start_y > term_rows) {
            start_x += start_y - term_rows;
            start_y = term_rows;
        }
        int y = start_y; int x = start_x;
        while (x <= term_cols && y >= 1) {
            set_location(y, x);
            printf(" ");
            --y;
            ++x;
        }
        hide_cursor();
        flush();
        sleep(DEATH_SCREEN_TICK);
    };

    clear_color();
    back_red();
    for (int i = 1; i <= term_rows + term_cols; i += 2)
        color_diagonal(i);
    for (int i = term_rows + term_cols - (((term_rows + term_cols) % 2 == 1) ? 1 : 2); i >= 1; i -= 2)
        color_diagonal(i);
    sleep(1000);
}

int insert_highscore(uint64_t s, const char* name)
{
    uint8_t i = 0;
    while (i < MAX_HIGHSCORES && highscores[i].score >= s) ++i;
    uint8_t index_inserted = i;
    HighscoreEntry prev = highscores[i];
    strcpy(highscores[i].name, name);  // strcpy can cause buffer overflows, use strncpy
    highscores[i].score = s;
    ++i;
    for (; i < MAX_HIGHSCORES; ++i) {
        HighscoreEntry temp = highscores[i];
        highscores[i] = prev;
        prev = temp;
    }
    return index_inserted;
}

// For some reason I can get a buffer overflow when compiling with -03 and pressing many keys and
// hitting delete quickly, but it doesn't do that without optimizations.
// KNOWN BUG: pressing a key and backspace at the same time can cause buffer overflow on last character
// This puts a space in name where the null terminator should habe been
void input_user_name(char* name)
{
    // Clear name
    memset(name, ' ', HIGHSCORE_NAME_LEN);

    // Set up stdin for keyboard input
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);             // Get current terminal parameters
    newt = oldt;                                // Copy oldt to newt
    newt.c_lflag &= ~(ICANON | ECHO);           // c_lflag control local modes. Sets the local mode to be noncannoical and to not echo characters
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);    // Set STDIN file descriptor to have new terminal parameters effective immediately

    // Assume name is HIGHSCORE_NAME_LEN characters plus null terminator
    int i = 0;
    char c = '\0';
    while (true) {
        c = std::cin.get();
        if (c == ENTER && i > 0){
            break;
        }
        else if ((c == BACKSPACE || c == DELETE) && i > 0) {
            name[i] = ' ';
            --i;
            cursor_back();
            printf("_");
            cursor_back();
        }
        else if (isalpha(c) && i < HIGHSCORE_NAME_LEN) {
            c = toupper(c);
            name[i] = c;   // Place in name array
            i++;
            printf("%c", c);  // Echo to stdout
        }
    }
    name[HIGHSCORE_NAME_LEN] = '\0';    // This fixes the bug(?)

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);    // Set the terminal parameters to original (TCSANOW forces change now)
}

void score_screen()
{
    clear_color();
    clear_screen();
    // Show score, number of fruits collected, speed level
    set_location(2, 2);
    printf("YOU DIED :(");
    set_location(4, 2);
    printf("SCORE: %lu", score);
    set_location(5, 2);
    printf("FRUITS: %d", fruits_coll);
    set_location(6, 2);
    printf("SPEED LEVEL: %d", speed_level);

    if (score > highscores[MAX_HIGHSCORES - 1].score) { // GOT A HIGHSCORE
        print_highscores(11, 2);
        set_location(8, 2);
        printf("HIGHSCORE GET, YAY!");
        set_location(9, 2);
        printf("ENTER NAME: ___");
        set_location(9, 14);
        flush();

        // Enter name
        //const char* name = "ABC";
        //std::string name;
        //std::cin >> std::setw(HIGHSCORE_NAME_LEN) >> name;
        char name [HIGHSCORE_NAME_LEN + 1];
        input_user_name(name);
        // Debug print name
        set_location(24, 1);
        //for (int i = 0; i < 21; i++)
        //    printf("%c", name[i]);

        int hs_index = insert_highscore(score, name);

        write_highscores();         // Write scores to file right away
        print_highscores(11, 2);    // Reprint highscores
        // Print new highscore with green background
        back_green();
        set_location(11 + hs_index + 1, 2);
        print_highscore(hs_index);
        clear_color();
    }
    else {
        print_highscores(8, 2);
    }

    set_location(term_rows, term_cols / 2 - 7);
    printf("<enter to quit>");
    hide_cursor();
    flush();
    fflush(stdin);
    wait_for_enter();
}

int main()
{
    srand(time(NULL));  // For the fruit placement
    read_highscores();

    // Get and set terminal dimensions
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    term_rows = size.ws_row;
    term_cols = size.ws_col;

    welcome_screen();
    snek = Snake(Point(term_rows, term_cols));

    // Clear screen
    clear_color();
    clear_screen();

    // Print field
    print_field();
    print_snake();
    print_score();
    place_random_fruit();
    hide_cursor();
    flush();

    game_loop();

    death_screen();
    score_screen();

    // End of game clear screen
    clear_color();
    clear_screen();
    flush();

	return 0;
}
