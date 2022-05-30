#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define RESPONSE_SIZE 256
#define CLIENT_MESS_SIZE 256
#define FRED "\x1B[31m"
#define NONE "\x1B[0m"
#define FBLU   "\x1B[34m"
int ping_pong_counter = 0;
char server_buff[RESPONSE_SIZE];
char client_buff[CLIENT_MESS_SIZE];
const char pong[] = "PONG ";
const char ping[] = "PING ";
const char client_quit[] = "QUIT";
const char client_help[] = "/HELP";
const char help[] = "Commands: \n"
                    "1. CNOTICE <nickname> <channel> :<message> \n"
                    "Sends a channel NOTICE message to <nickname> on <channel> that bypasses flood protection limits. \n"
                    "2. INVITE <nickname> <channel> \n"
                    "Invites <nickname> to the channel <channel>. \n"
                    "3. JOIN <channels> [<keys>] \n"
                    "Makes the client join the channels in the comma-separated list <channels>, specifying the passwords, if needed, in the comma-separated list <keys>. \n"
                    "4. KICK <channel> <client> :[<message>] \n"
                    "Forcibly removes <client> from <channel>. \n"
                    "5. LIST [<channels> [<server>]] \n"
                    "Lists all channels on the server. \n"
                    "6. NICK <nickname> \n"
                    "Allows a client to change their IRC nickname. \n"
                    "7. PART <channels> [<message>] \n"
                    "Causes a user to leave the channels in the comma-separated list <channels>. \n"
                    "8. PRIVMSG <msgtarget> :<message> \n"
                    "Sends <message> to <msgtarget>, which is usually a user or channel. \n"
                    "9. TIME [<server>] \n"
                    "Returns the local time on the current server, or <server> if specified. \n"
                    "10. WHOIS [<server>] <nicknames> \n"
                    "Returns information about the comma-separated list of nicknames masks <nicknames> \n";
char pong_mess[32];

int init_conn(char *server_name);

void *receive_mess(void *sock_fd);

char *get_pass(int argc, char **argv);

void set_client_parameters(char **argv, char *pass, int *sock_fd);

void handle_user_input(int *sock_fd);

void send_wrapper(const int *sock_fd, char *buff, int size_of);

void fgets_wrapper(char *buff, int size_of);


int main(int argc, char **argv) {
    pthread_t tid;
    char *pass;
    if (argc > 5 || argc < 4) {
        fprintf(stderr, "Error during starting client\n"
                        "Example of usage: %s <IRC server name> <nick> <username> <pass (optional)>\n", argv[0]);
        exit(0);
    }
    pass = get_pass(argc, argv);

    int socket_fd = init_conn(argv[1]);
    set_client_parameters(argv, pass, &socket_fd);
    pthread_create(&tid, NULL, receive_mess, (void *) &socket_fd);

    while (1) {
        handle_user_input(&socket_fd);
        sleep(1);
    }

    return 0;
}

init_conn(char *server_name) {
    struct addrinfo serveraddrinfo;
    serveraddrinfo.ai_family = AF_INET;
    serveraddrinfo.ai_socktype = SOCK_STREAM;
    serveraddrinfo.ai_flags = 0;
    serveraddrinfo.ai_protocol = 0;
    struct addrinfo *res;
    int addr_info_error;
    int socket_fd;
    addr_info_error = getaddrinfo(server_name, "6667", &serveraddrinfo, &res);
    if (addr_info_error != 0)
        fprintf(stderr, "Problem during getting info about server. %s", gai_strerror(addr_info_error));
    for (; res != NULL; res = res->ai_next) {
        socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (socket_fd < 0) {
            continue;
        }
        if (connect(socket_fd, res->ai_addr, res->ai_addrlen) != -1) {
            break;
        }
        close(socket_fd);
    }
    return socket_fd;
}

void *receive_mess(void *sock_fd) {
    bzero(&server_buff, sizeof(server_buff));
    while (1) {
        recv(*(int *) sock_fd, &server_buff, sizeof(server_buff), 0);
        if (strncmp(server_buff, ping, 5) == 0) {
            strcat(pong_mess, pong);
            strncpy(pong_mess + 5, server_buff + 5, strlen(server_buff));
            send_wrapper(sock_fd, pong_mess, sizeof(pong_mess));
            printf(FBLU "%s" NONE, server_buff);
            printf(FBLU "%s" NONE, pong_mess);
            printf(FBLU "Ping Pong counter: %d \n" NONE, ++ping_pong_counter);
            bzero(&pong_mess, sizeof(pong_mess));
            bzero(&server_buff, sizeof(server_buff));
            continue;
        }
        printf("%s", server_buff);
        bzero(&server_buff, sizeof(server_buff));
    }
}

char *get_pass(int argc, char **argv) {
    char *p;
    if (argc < 4) {
        p = "none";
        return p;
    }
    p = argv[4];
    return p;
}

void set_client_parameters(char **argv, char *pass, int *sock_fd) {
    char nick_command[128] = "NICK ";
    char pass_command[128] = "PASS ";
    char user_command[128] = "USER ";
    strcat(nick_command, argv[2]);
    strcat(nick_command, "\r\n");
    strcat(pass_command, pass);
    strcat(pass_command, "\r\n");
    strcat(user_command, argv[3]);
    strcat(user_command, " ");
    strcat(user_command, "test");
    strcat(user_command, " ");
    strcat(user_command, "test1");
    strcat(user_command, " ");
    strcat(user_command, ":Test Testowy");
    strcat(user_command, "\r\n");
    send_wrapper(sock_fd, pass_command, sizeof(pass_command));
    send_wrapper(sock_fd, nick_command, sizeof(nick_command));
    send_wrapper(sock_fd, user_command, sizeof(user_command));
}

void handle_user_input(int *sock_fd) {
    fgets_wrapper(client_buff, sizeof(client_buff));
    send_wrapper(sock_fd, client_buff, sizeof(client_buff));
    bzero(client_buff, sizeof(client_buff));
}

void send_wrapper(const int *sock_fd, char *buff, int size_of) {
    if (strlen(buff) == 0)
        return;
    if (strncmp(buff, client_help, 5) == 0) {
        printf(FBLU "%s" NONE, help);
        return;
    }
    int i;
    for (i = 0; i < 5; i++) {
        if (send(*(int *) sock_fd, buff, size_of, 0) != -1) {
            break;
        }
        printf(FRED "Error during sending message. Repeating...  %d/5" NONE, i + 1);
    }
    if (strncmp(buff, client_quit, 4) == 0) {
        printf(FBLU "Quiting ..." NONE);
        exit(1);
    }
}

void fgets_wrapper(char *buff, int size_of) {
    if (fgets(buff, size_of - 3, stdin) == NULL) {
        printf(FRED "Error during getting user input. Try again" NONE);
        return;
    }
    strcat(buff, "\r\n");
}
