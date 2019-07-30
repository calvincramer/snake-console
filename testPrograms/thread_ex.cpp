#include <iostream>     // C++ version io
// #include <stdio.h>      // C version io
// #include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>     // STDOUT_FILENO definition, sleeping
// #include <chrono>       // for sleeping
#include <thread>       // for sleeping
// #include <iostream>
// #include <limits>
// #include <ios>
// #include <thread>
#include <fcntl.h>

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
bool key_w_pressed = KEY_UP;
bool key_a_pressed = KEY_UP;
bool key_s_pressed = KEY_UP;
bool key_d_pressed = KEY_UP;
bool key_space_pressed = KEY_UP;
bool end_kb_thread = false;
#define KB_SLEEP_MS 50


void sleep(int ms)
{
    usleep(ms * 1000);
}


void keyboard_thread_loop()
{
    printf("Keyboard thread running!\n");

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
        key_w_pressed = KEY_UP;
        key_a_pressed = KEY_UP;
        key_s_pressed = KEY_UP;
        key_d_pressed = KEY_UP;
        key_space_pressed = KEY_UP;
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

    printf("Keyboard thread stopping!\n");
}

int main()
{
    std::thread kb_thread(keyboard_thread_loop);

    while (1) {
        // Goto start
        printf("\033[10;10H");
        printf((key_w_pressed) ? "W" : "w");
        printf((key_a_pressed) ? "A" : "a");
        printf((key_s_pressed) ? "S" : "s");
        printf((key_d_pressed) ? "D" : "d");
        printf(" ");
        printf((key_space_pressed) ? "SPACE" : "space");
        fflush(stdout);
        sleep(100);

    }

    end_kb_thread = true;
    kb_thread.join();

    return 0;
}

