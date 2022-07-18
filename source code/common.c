#include "common.h"

void check_error(int rc, char *msg) {
    if (rc == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void strip(char *str) {
    char buf[strlen(str) + 1];
    int i = 0;
    while (isspace(str[i])) {
        i++;
    }
    strcpy(buf, &str[i]);

    i = strlen(buf) - 1;
    while (isspace(buf[i])) {
        i--;
    }
    buf[i + 1] = 0;
    strcpy(str, buf);
}

struct client_node* make_client_node(char *nick, struct sockaddr_in *client_addr) {
    struct client_node *new_client = malloc(sizeof(struct client_node));

    new_client->next = NULL;
    new_client->client_addr.sin_family = AF_INET;
    new_client->client_addr.sin_addr.s_addr = client_addr->sin_addr.s_addr;
    new_client->client_addr.sin_port = client_addr->sin_port;
    strcpy(new_client->nick, nick);
    new_client->last_update = time(NULL);

    return new_client;
}

void add_client(struct client_list *clients, struct client_node *client) {
    struct client_node *current_node;

    if (clients->first == NULL) {
        clients->first = client;
        return;
    }

    current_node = clients->first;

    if (strcmp(current_node->nick, client->nick) == 0) {
        clients->first = client;
        client->next = current_node->next;
        free(current_node);
        return;
    }

    while (current_node->next) {
        if (strcmp(current_node->next->nick, client->nick) == 0) {
            client->next = current_node->next->next;
            free(current_node->next);
            current_node->next = client;
            return;
        }
        
        current_node = current_node->next;
    }

    current_node->next = client;
}

void remove_client(struct client_list *clients, char *nick) {
    struct client_node *current_node, *prev_node;

    prev_node = NULL;
    current_node = clients->first;

    while (current_node) {
        if (strcmp(current_node->nick, nick) == 0) {
            break;
        }

        prev_node = current_node;
        current_node = current_node->next;
    }

    if (current_node) {
        if (prev_node) {
            prev_node->next = current_node->next;
        } else {
            clients->first = current_node->next;
        }
        free(current_node);
    }
}

struct client_node* find_client(struct client_list *clients, char *nick) {
    struct client_node *current_node = clients->first;

    while (current_node) {
        if (strcmp(current_node->nick, nick) == 0) {
            return current_node;
        }

        current_node = current_node->next;
    }

    return NULL;
}

void free_clients(struct client_list *clients) {
    struct client_node *current_node, *prev_node;

    prev_node = NULL;
    current_node = clients->first;

    while (current_node) {
        prev_node = current_node;
        current_node = current_node->next;

        free(prev_node);
    }
}

void debug_clients(struct client_list *clients) {
    struct client_node *current_node = clients->first;

    printf("\nDebugging:\n\n");

    while (current_node) {
        printf("%s\n%s\n%hu\n\n", current_node->nick, inet_ntoa(current_node->client_addr.sin_addr), ntohs(current_node->client_addr.sin_port));

        current_node = current_node->next;
    }

    printf("\n");
}