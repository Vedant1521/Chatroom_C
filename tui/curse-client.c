#include <ncurses.h>
#include "socketutils.h"

#define IP "127.0.0.1"
#define MAX_INPUT_LEN 256
#define CHAT_WIN_BORDER 1
#define INPUT_WIN_HEIGHT 4
#define STATUS_WIN_HEIGHT 1

WINDOW *chat_win;
WINDOW *input_win;
WINDOW *status_win;
WINDOW *chat_border_win;
WINDOW *input_border_win;

int chat_win_height, chat_win_width;
int input_win_width;
int max_y, max_x;
char username[64];
int connected = 1;

void cleanup_ncurses() {
    if (chat_win) delwin(chat_win);
    if (input_win) delwin(input_win);
    if (status_win) delwin(status_win);
    if (chat_border_win) delwin(chat_border_win);
    if (input_border_win) delwin(input_border_win);
    endwin();
}

void update_dimensions() {
    getmaxyx(stdscr, max_y, max_x);
    
    // Chat window takes most of the screen
    chat_win_height = max_y - INPUT_WIN_HEIGHT - STATUS_WIN_HEIGHT - 1;
    chat_win_width = max_x - 2; // Leave space for borders
    
    // Input window at bottom
    input_win_width = max_x - 2;
}

void draw_status_bar() {
    werase(status_win);
    wattron(status_win, A_REVERSE);
    
    char status_text[256];
    if (connected) {
        snprintf(status_text, sizeof(status_text), 
                " Connected as: %s | F1: Help | F2: Clear | Ctrl+C: Exit ", username);
    } else {
        snprintf(status_text, sizeof(status_text), 
                " DISCONNECTED | F1: Help | Ctrl+C: Exit ");
    }
    
    mvwprintw(status_win, 0, 0, "%-*s", max_x, status_text);
    wattroff(status_win, A_REVERSE);
    wrefresh(status_win);
}

void show_help() {
    WINDOW *help_win = newwin(15, 60, (max_y - 15) / 2, (max_x - 60) / 2);
    box(help_win, 0, 0);
    
    wattron(help_win, A_BOLD);
    mvwprintw(help_win, 1, 2, "CHAT CLIENT HELP");
    wattroff(help_win, A_BOLD);
    
    mvwprintw(help_win, 3, 2, "Commands:");
    mvwprintw(help_win, 4, 4, "exit       - Quit the application");
    mvwprintw(help_win, 5, 4, "/clear     - Clear chat window");
    mvwprintw(help_win, 6, 4, "/help      - Show this help");
    
    mvwprintw(help_win, 8, 2, "Keyboard Shortcuts:");
    mvwprintw(help_win, 9, 4, "F1         - Show help");
    mvwprintw(help_win, 10, 4, "F2         - Clear chat window");
    mvwprintw(help_win, 11, 4, "Ctrl+C     - Exit application");
    mvwprintw(help_win, 12, 4, "Enter      - Send message");
    
    mvwprintw(help_win, 13, 2, "Press any key to close...");
    wrefresh(help_win);
    
    wgetch(help_win);
    delwin(help_win);
    
    // Refresh all windows
    touchwin(stdscr);
    refresh();
    wrefresh(chat_border_win);
    wrefresh(chat_win);
    wrefresh(input_border_win);
    wrefresh(input_win);
    draw_status_bar();
}

void clear_chat_window() {
    werase(chat_win);
    wrefresh(chat_win);
    
    // Show welcome message again
    wattron(chat_win, A_BOLD | COLOR_PAIR(3));
    wprintw(chat_win, "=== Chat Window Cleared ===\n");
    wprintw(chat_win, "Welcome back to the chat, %s!\n", username);
    wattroff(chat_win, A_BOLD | COLOR_PAIR(3));
    wrefresh(chat_win);
}

void handle_resize() {
    endwin();
    refresh();
    
    update_dimensions();
    
    // Recreate windows with new dimensions
    if (chat_border_win) delwin(chat_border_win);
    if (chat_win) delwin(chat_win);
    if (input_border_win) delwin(input_border_win);
    if (input_win) delwin(input_win);
    if (status_win) delwin(status_win);
    
    // Status bar at top
    status_win = newwin(STATUS_WIN_HEIGHT, max_x, 0, 0);
    
    // Chat window with border
    chat_border_win = newwin(chat_win_height + 2, chat_win_width + 2, 
                            STATUS_WIN_HEIGHT, 0);
    chat_win = newwin(chat_win_height, chat_win_width, 
                     STATUS_WIN_HEIGHT + 1, 1);
    
    // Input window with border at bottom
    input_border_win = newwin(INPUT_WIN_HEIGHT, input_win_width + 2, 
                             max_y - INPUT_WIN_HEIGHT, 0);
    input_win = newwin(INPUT_WIN_HEIGHT - 2, input_win_width, 
                      max_y - INPUT_WIN_HEIGHT + 1, 1);
    
    // Configure windows
    scrollok(chat_win, TRUE);
    keypad(input_win, TRUE);
    
    // Draw borders and titles
    box(chat_border_win, 0, 0);
    mvwprintw(chat_border_win, 0, 2, " Chat ");
    
    box(input_border_win, 0, 0);
    mvwprintw(input_border_win, 0, 2, " Message ");
    
    // Refresh all windows
    draw_status_bar();
    wrefresh(chat_border_win);
    wrefresh(chat_win);
    wrefresh(input_border_win);
    wrefresh(input_win);
}

void ncurse_init() {
    initscr();
    
    // Check if terminal supports colors
    if (has_colors()) {
        start_color();
        use_default_colors();
        
        // Define color pairs
        init_pair(1, COLOR_GREEN, -1);   // Own messages
        init_pair(2, COLOR_CYAN, -1);    // Other messages
        init_pair(3, COLOR_YELLOW, -1);  // System messages
        init_pair(4, COLOR_RED, -1);     // Error messages
        init_pair(5, COLOR_MAGENTA, -1); // Timestamps
    }
    
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1); // Show cursor
    nodelay(stdscr, FALSE);
    
    // Handle window resizing
    #ifdef KEY_RESIZE
    keypad(stdscr, TRUE);
    #endif
    
    update_dimensions();
    
    // Create status bar at top
    status_win = newwin(STATUS_WIN_HEIGHT, max_x, 0, 0);
    
    // Create chat window with border
    chat_border_win = newwin(chat_win_height + 2, chat_win_width + 2, 
                            STATUS_WIN_HEIGHT, 0);
    chat_win = newwin(chat_win_height, chat_win_width, 
                     STATUS_WIN_HEIGHT + 1, 1);
    
    // Create input window with border at bottom
    input_border_win = newwin(INPUT_WIN_HEIGHT, input_win_width + 2, 
                             max_y - INPUT_WIN_HEIGHT, 0);
    input_win = newwin(INPUT_WIN_HEIGHT - 2, input_win_width, 
                      max_y - INPUT_WIN_HEIGHT + 1, 1);
    
    // Configure windows
    scrollok(chat_win, TRUE);
    keypad(input_win, TRUE);
    wtimeout(input_win, -1); // Blocking input
    
    // Draw borders and titles
    box(chat_border_win, 0, 0);
    mvwprintw(chat_border_win, 0, 2, " Chat ");
    
    box(input_border_win, 0, 0);
    mvwprintw(input_border_win, 0, 2, " Message ");
    
    // Initial refresh
    refresh();
    draw_status_bar();
    wrefresh(chat_border_win);
    wrefresh(input_border_win);
    
    // Welcome message
    wattron(chat_win, A_BOLD | COLOR_PAIR(3));
    wprintw(chat_win, "=== Welcome to Enhanced Chat Client ===\n");
    wprintw(chat_win, "Connected as: %s\n", username);
    wprintw(chat_win, "Type '/help' for commands or press F1\n");
    wprintw(chat_win, "========================================\n");
    wattroff(chat_win, A_BOLD | COLOR_PAIR(3));
    wrefresh(chat_win);
}

void add_timestamp_to_chat() {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "[%H:%M:%S] ", tm_info);
    
    if (has_colors()) {
        wattron(chat_win, COLOR_PAIR(5));
        wprintw(chat_win, "%s", timestamp);
        wattroff(chat_win, COLOR_PAIR(5));
    } else {
        wprintw(chat_win, "%s", timestamp);
    }
}

// Function to strip ANSI color codes
void strip_ansi_codes(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (*src == '\033' && src[1] == '[') {
            // Skip ANSI escape sequence
            src += 2;
            while (*src && (*src != 'm')) {
                src++;
            }
            if (*src == 'm') src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Function to detect message type and apply appropriate colors
int get_message_color_pair(const char *message) {
    if (strstr(message, "joined") != NULL) {
        return 1; // Blue/cyan for joins
    } else if (strstr(message, "left") != NULL || strstr(message, "disconnected") != NULL) {
        return 4; // Red for leaves
    }
    return 2; // Default color for regular messages
}

void print_received_message(const char *message) {
    char clean_message[BUF_SIZE];
    strncpy(clean_message, message, sizeof(clean_message) - 1);
    clean_message[sizeof(clean_message) - 1] = '\0';
    
    // Strip ANSI codes
    strip_ansi_codes(clean_message);
    
    add_timestamp_to_chat();
    
    if (has_colors()) {
        int color_pair = get_message_color_pair(clean_message);
        wattron(chat_win, COLOR_PAIR(color_pair));
        if (color_pair == 1 || color_pair == 4) {
            wattron(chat_win, A_BOLD); // Make join/leave messages bold
        }
    }
    wprintw(chat_win, "%s\n", clean_message);
    if (has_colors()) {
        wattroff(chat_win, A_BOLD);
        wattroff(chat_win, COLOR_PAIR(get_message_color_pair(clean_message)));
    }
    wrefresh(chat_win);
}

void print_own_message(const char *message) {
    add_timestamp_to_chat();
    
    if (has_colors()) {
        wattron(chat_win, COLOR_PAIR(1) | A_BOLD);
    }
    wprintw(chat_win, "You: %s\n", message);
    if (has_colors()) {
        wattroff(chat_win, COLOR_PAIR(1) | A_BOLD);
    }
    wrefresh(chat_win);
}

void print_system_message(const char *message) {
    add_timestamp_to_chat();
    
    if (has_colors()) {
        wattron(chat_win, COLOR_PAIR(3) | A_BOLD);
    }
    wprintw(chat_win, "*** %s ***\n", message);
    if (has_colors()) {
        wattroff(chat_win, COLOR_PAIR(3) | A_BOLD);
    }
    wrefresh(chat_win);
}

void *listenPrintChatsOfOthers(void *arg) {
    int fd = *(int *)arg;
    free(arg);
    char buffer[BUF_SIZE];
    
    while (connected) {
        ssize_t bytes_received = recv(fd, buffer, BUF_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            print_received_message(buffer);
        } else if (bytes_received == 0) {
            connected = 0;
            print_system_message("Server disconnected");
            draw_status_bar();
            break;
        } else {
            if (connected) {
                print_system_message("Connection error");
                connected = 0;
                draw_status_bar();
            }
            break;
        }
    }
    close(fd);
    return NULL;
}

void listenOnNewThread(int fd) {
    int *arg = malloc(sizeof(int));
    *arg = fd;
    pthread_t id;
    pthread_create(&id, NULL, listenPrintChatsOfOthers, arg);
}

int main() {
    int socket_fd;
    if ((socket_fd = createTCPipv4Socket()) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in *addr;
    if ((addr = createIpv4Address(IP, PORT)) == NULL) {
        printf("Invalid Address\n");
        return EXIT_FAILURE;
    }

    if (connect(socket_fd, (struct sockaddr*)addr, sizeof(*addr)) < 0) {
        perror("connect");
        return EXIT_FAILURE;
    }

    char *name = NULL;
    size_t nameSize = 0;
    printf("Enter your name: ");
    ssize_t byteCount = getline(&name, &nameSize, stdin);
    name[byteCount - 1] = '\0';
    strncpy(username, name, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';
    send(socket_fd, name, strlen(name), 0);

    ncurse_init();
    listenOnNewThread(socket_fd);

    char msg[MAX_INPUT_LEN];
    int ch;
    
    while (connected) {
        werase(input_win);
        mvwprintw(input_win, 0, 0, "> ");
        wrefresh(input_win);
        
        // Get input with enhanced handling
        wmove(input_win, 0, 2);
        echo();
        wgetnstr(input_win, msg, MAX_INPUT_LEN - 1);
        noecho();
        
        // Handle special commands
        if (strcmp(msg, "exit") == 0) {
            break;
        } else if (strcmp(msg, "/help") == 0) {
            show_help();
            continue;
        } else if (strcmp(msg, "/clear") == 0) {
            clear_chat_window();
            continue;
        }
        
        // Don't send empty messages
        if (strlen(msg) == 0) {
            continue;
        }
        
        // Send message to server
        char buffer[BUF_SIZE];
        snprintf(buffer, sizeof(buffer), "%s: %s", username, msg);
        if (send(socket_fd, buffer, strlen(buffer), 0) < 0) {
            print_system_message("Failed to send message");
            continue;
        }
        
        // Display own message
        print_own_message(msg);
        
        // Handle special keys
        ch = wgetch(stdscr);
        if (ch == KEY_F(1)) {
            show_help();
        } else if (ch == KEY_F(2)) {
            clear_chat_window();
        }
        #ifdef KEY_RESIZE
        else if (ch == KEY_RESIZE) {
            handle_resize();
        }
        #endif
    }
    
    close(socket_fd);
    cleanup_ncurses();
    free(name);
    
    printf("Chat session ended. Goodbye!\n");
    return EXIT_SUCCESS;
}
