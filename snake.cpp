#include <cstring>      // strcpy(), memset()
#include <deque>
#include <fcntl.h>
#include <fstream>
#include <iostream>     // std::cin
#include <stdio.h>      // C version io
#include <string>       // C++ std::string
#include <sys/ioctl.h>  // Only on Linux
#include <termios.h>    // Only on Linux
#include <thread>       // for sleeping
#include <unistd.h>     // STDOUT_FILENO definition, usleep()

#define BORDER_CHAR             '#' // Border character
#define DOT_CHAR                '.'
#define DOT_SEP_X               8
#define DOT_SEP_Y               4
#define BASE_POINTS_FOR_FRUIT   10
#define KB_SLEEP_MS             20  // Keyboard sleep
#define DEFAULT_TERMINAL_ROWS   20
#define DEFAULT_TERMINAL_COLS   20
#define INIT_SNAKE_LENGTH       8
#define GAME_TICK_MS_START      100 // Snake moves 10 tiles per second at start
#define MIN_GAME_TICK           30
#define SPEEDUP_MS              5   // Amount to speedup game by
#define FRUITS_PER_SPEEDUP      5
#define DEATH_SCREEN_TICK       50
#define MAX_HIGHSCORES          10
#define HIGHSCORE_NAME_LEN      3
#define DIR_RGHT                0
#define DIR_DOWN                1
#define DIR_LEFT                2
#define DIR_UP                  3
#define NO_FRUIT                Point(65535, 65535)
const std::string highscore_file = "hs.txt";

// ASCII values for keyboard input
#define KEY_w_ASCII     119
#define KEY_a_ASCII     97
#define KEY_s_ASCII     115
#define KEY_d_ASCII     100
#define KEY_W_ASCII     87
#define KEY_A_ASCII     65
#define KEY_S_ASCII     83
#define KEY_D_ASCII     68
#define KEY_UP          false
#define KEY_DOWN        true
#define BACKSPACE       8
#define DELETE          127
#define ENTER           10  // Newline

struct HighscoreEntry {
    char name[HIGHSCORE_NAME_LEN + 1];  // Plus one for null terminal
    uint64_t score;
};

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
uint32_t fruits_coll   = 0;       // Will get reset to zero when speeding up
uint32_t fruits_total  = 0;
uint32_t speed_level   = 1;
uint16_t game_field_left   = 2;   // Second column is left-most spot on playing field
uint16_t game_field_top    = 2;
uint16_t game_field_right  = term_cols - 1;
uint16_t game_field_bottom = term_rows - 1;
uint16_t print_speed_level_x = 9;
uint16_t print_fruits_collected_x = 20;
uint16_t print_score_x = 30;
Point fruit = NO_FRUIT;
HighscoreEntry highscores[MAX_HIGHSCORES];
int unused = 0;     // Used to get around -Werror=unused

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
void sleep(int ms)  { usleep(ms * 1000);   }
void set_location(int y, int x) { printf("\033[%d;%dH", y, x); }

class Snake {
 public:
    std::deque<Point> segments;     // Head of deque is head of snake
    std::deque<uint8_t> seg_dirs;   // Stores the directions the were taken
    uint16_t head_x() { return this->segments.front().x; }
    uint16_t head_y() { return this->segments.front().y; }
    Point& head()     { return this->segments.front();   }

    Snake(Point term_dim)
    {
        Point term_mid (term_dim.y / 2, (term_dim.x / 2) - (INIT_SNAKE_LENGTH / 2));
        for (int i = 0; i < INIT_SNAKE_LENGTH; i++) {
            Point temp_point (term_mid.y, term_mid.x + i);
            this->segments.push_front(temp_point);
            this->seg_dirs.push_front(DIR_RGHT);
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

    bool on_board_dot(Point p) {
        return (p.x - game_field_left) % DOT_SEP_X == 0
            && (p.y - game_field_top) % DOT_SEP_Y == 0;
    }

    void move(Point p)
    {
        this->segments.push_front(p);
        this->seg_dirs.push_front(last_dir);
        bool collected_fruit = (p == fruit);
        if (!collected_fruit) {
            // Clear the last section of the snake on the screen (otherwise tail wouldn't stay on screen forever)
            clear_color();  // clear color needed for background color
            set_location(this->segments.back().y, this->segments.back().x);
            // Check if need to reprint border
            if (on_board_dot(this->segments.back())) {
                printf("%c", DOT_CHAR);
            } else {
                printf(" ");
            }
            this->segments.pop_back();
            this->seg_dirs.pop_back();
        }
    }
};
Snake snek(Point(term_rows, term_cols));

void print_snake()
{
    back_green();
    int seg_num = 0;
    bool alt = true;
    for (Point& p : snek.segments) {
        if (seg_num % 5 == 0 && seg_num > 0) {
            if (alt) {
                fore_green();
                back_red();
                alt = false;
            }
            else {
                fore_green();
                back_yellow();
                alt = true;
            }
        }
        set_location(p.y, p.x);
        if      (seg_num == 0) { printf( (last_dir == DIR_UP || last_dir == DIR_DOWN) ? "|" : "-"); }
        else if (seg_num == 1) { printf("O"); }
        else if (seg_num == snek.length() - 1) { // Point tail towards second to last segment
            switch (snek.seg_dirs[seg_num - 1]) {
            case DIR_RGHT: printf("<"); break;
            case DIR_DOWN: printf("^"); break;
            case DIR_LEFT: printf(">"); break;
            case DIR_UP:   printf("V"); break;
            }
        }
        else printf("o");

        if (seg_num % 5 == 0) {
            fore_white();
            back_green();
        }

        ++seg_num;
    }
}

void play_sound()
{
    printf("\a");
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
    fore_yellow();
    blink_text();
    set_location(fruit.y, fruit.x);
    printf("O");
}

uint8_t length_of_num(uint32_t n) {
    uint8_t digits = 0;
    do {
         ++digits;
         n /= 10;
    } while (n);
    return digits;
}

void print_score_fruit_speed_level()
{
    fore_blue();
    back_black();
    set_location(term_rows, 4);
    printf(" Speed: %i %c Fruits: %i %c Score %lu ", speed_level, BORDER_CHAR, fruits_total, BORDER_CHAR, score);
}

void place_random_fruit()
{
    do {
        fruit.x = (rand() % (game_field_right - game_field_left)) + game_field_left;
        fruit.y = (rand() % (game_field_bottom - game_field_top)) + game_field_top;
    } while (snek.point_on_snake(fruit));
    print_fruit();
}

void print_field()
{
    // Border
    back_black();
    fore_blue();
    // Left
    for (int x = 1; x < game_field_left; ++x) {
    for (int y = 1; y <= term_rows; ++y) {
        set_location(y, x);
        printf("%c", BORDER_CHAR);
    }}
    // Right
    for (int x = game_field_right + 1; x <= term_cols; ++x) {
    for (int y = 1; y <= term_rows; ++y) {
        set_location(y, x);
        printf("%c", BORDER_CHAR);
    }}
    // Top
    for (int y = 1; y < game_field_top; ++y) {
        set_location(y, 1);
        printf("%s", std::string(term_cols, BORDER_CHAR).c_str());
    }
    // Bottom
    for (int y = game_field_bottom + 1; y <= term_rows; ++y) {
        set_location(y, 1);
        printf("%s", std::string(term_cols, BORDER_CHAR).c_str());
    }
    // Dots
    clear_color();
    for (int y = game_field_top; y <= game_field_bottom; y += DOT_SEP_Y) {
    for (int x = game_field_left; x <= game_field_right; x += DOT_SEP_X) {
        set_location(y, x);
        printf("%c", DOT_CHAR);
    }}
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

uint64_t points_for_fruit()
{
    switch (speed_level) {
    case 1:  return 10;
    case 2:  return 14;
    case 3:  return 19;
    case 4:  return 24;
    case 5:  return 31;
    case 6:  return 39;
    case 7:  return 49;
    case 8:  return 61;
    case 9:  return 75;
    case 10: return 92;
    case 11: return 112;
    case 12: return 137;
    case 13: return 166;
    case 14: return 201;
    case 15: return 250;
    default: return BASE_POINTS_FOR_FRUIT;
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
        score += points_for_fruit();
        ++fruits_coll;
        ++fruits_total;
        if (fruits_coll >= FRUITS_PER_SPEEDUP) {    // SPEED THE GAME UP!
            fruits_coll = 0;
            if (game_tick_ms > MIN_GAME_TICK) {
                game_tick_ms = std::max(game_tick_ms - SPEEDUP_MS, MIN_GAME_TICK);
                ++speed_level;
            }
        }
        play_sound();
        place_random_fruit();
        // extended snake handled by snek.move()
        print_score_fruit_speed_level();
    }
    // Check if hit self or outside edge
    if (snek.head_x() < game_field_left || snek.head_x() > game_field_right
     || snek.head_y() < game_field_top  || snek.head_y() > game_field_bottom
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
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    set_location(4,4);
    printf("WELCOME TO SNAKE!");
    set_location(6,4);
    printf("wasd to move");
    print_highscores(8, 4);
    set_location(term_rows - 2, 4);
    printf("by Calvin Cramer - calvincramer@gmail.com");
    set_location(term_rows, term_cols / 2 - 8);
    printf("<enter to start>");

    hide_cursor();
    flush();
    wait_for_enter();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);    // Set the terminal parameters to original (TCSANOW forces change now)
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
    // Color in reverse direction
    // for (int i = term_rows + term_cols - (((term_rows + term_cols) % 2 == 1) ? 1 : 2); i >= 1; i -= 2)
    //     color_diagonal(i);
    // sleep(1000);
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
    unused = system("setterm -cursor on");    // Turn cursor on
    memset(name, ' ', HIGHSCORE_NAME_LEN);  // Clear name

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
    unused = system("setterm -cursor off");
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
        set_location(term_rows, term_cols / 2 - 11);
        printf("<enter to insert name>");
        set_location(9, 14);    // Location where name input starts
        flush();

        // Enter name
        char name [HIGHSCORE_NAME_LEN + 1];
        input_user_name(name);
        int hs_index = insert_highscore(score, name);
        write_highscores();         // Write scores to file right away
        print_highscores(11, 2);    // Reprint highscores
        // Print new highscore with green background
        back_green();
        set_location(11 + hs_index + 1, 2);
        print_highscore(hs_index);
        clear_color();
        // Clear the <enter to insert name>
        set_location(term_rows, term_cols / 2 - 11);
        printf("                      ");
    }
    else {
        print_highscores(8, 2);
    }

    set_location(term_rows, term_cols / 2 - 7);
    printf("<enter to quit>");
    hide_cursor();
    flush();
    // Set stdin to not echo any input
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    wait_for_enter();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);    // Set the terminal parameters to original (TCSANOW forces change now)
}

void set_field_dimensions(uint16_t rows, uint16_t cols)
{
    term_rows = rows;
    term_cols = cols;

    int num_dots_hor = 1;
    while (num_dots_hor * DOT_SEP_X <= cols) ++num_dots_hor;
    while (num_dots_hor * DOT_SEP_X >= cols - 2) --num_dots_hor;
    int game_field_width = (num_dots_hor * DOT_SEP_X) + 1;
    int padding_hor      = cols - game_field_width;
    int padding_left     = padding_hor / 2;
    int padding_right    = padding_hor / 2 + (padding_hor % 2);
    game_field_left      = padding_left + 1;
    game_field_right     = cols - padding_right;

    int num_dots_ver = 1;
    while (num_dots_ver * DOT_SEP_Y <= rows) ++num_dots_ver;
    while (num_dots_ver * DOT_SEP_Y >= rows - 2) --num_dots_ver;
    int game_field_height = (num_dots_ver * DOT_SEP_Y) + 1;
    int padding_ver       = rows - game_field_height;
    int padding_top       = padding_ver / 2;
    int padding_bottom    = padding_ver / 2 + (padding_ver % 2);
    game_field_top        = padding_top + 1;
    game_field_bottom     = rows - padding_bottom;
}

int main()
{
    unused = system("setterm -cursor off");

    srand(time(NULL));  // For the fruit placement
    read_highscores();

    // Get and set terminal dimensions
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    set_field_dimensions(size.ws_row, size.ws_col);

    welcome_screen();
    snek = Snake(Point(term_rows, term_cols));

    // Clear screen
    clear_color();
    clear_screen();

    // Print field
    print_field();
    print_snake();
    print_score_fruit_speed_level();
    place_random_fruit();
    hide_cursor();
    flush();

    sleep(750);
    game_loop();

    // Now died
    death_screen();
    score_screen();

    // End of game clear screen
    clear_color();
    clear_screen();
    flush();

    unused = system("setterm -cursor on");
    if (unused == 134982) unused = 2;

	return 0;
}
