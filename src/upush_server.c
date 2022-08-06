#include "upush_server.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <port> <loss probability>\n", argv[0]);
        return -1;
    }


    /* Variables */
    struct client_list clients;
    struct client_node *client;
    struct sockaddr_in server_addr, client_addr;
    struct pkt pkt;
    socklen_t client_addr_len;
    char buf[BUFSIZE];
    int rc, fd;
    fd_set fds;

    
    /* Initialization */ 
    srand48(3);     /* Setting seed for testing in WSL */
    
    set_loss_probability(.01f * atoi(argv[2]));

    clients.first = NULL;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    rc = bind(fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));
    check_error(rc, "bind()");

    printf("Enter QUIT to quit\n");

    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(STDIN_FILENO, &fds);

        rc = select(FD_SETSIZE, &fds, 0, 0, 0);
        check_error(rc, "select()");

        clients_cleanup(&clients);

        if (FD_ISSET(fd, &fds)) {
            client_addr_len = sizeof(struct sockaddr_in);
            rc = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *) &client_addr, &client_addr_len);
            check_error(rc, "recvfrom()");
            buf[rc] = 0;

            rc = parse_msg(buf, &pkt);

            switch (rc) {
                case REG:
                    client = find_client(&clients, pkt.nick);
                    if (client 
                        && client->client_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr
                        && client->client_addr.sin_port == client_addr.sin_port) {

                        client->last_update = time(NULL);
                        
                    } else {
                        add_client(&clients, make_client_node(pkt.nick, &client_addr));
                        sprintf(buf, "ACK %d OK", pkt.pkt_n);

                        send_packet(fd,
                                    buf,
                                    strlen(buf),
                                    0,
                                    (struct sockaddr *) &client_addr,
                                    sizeof(struct sockaddr_in));
                    }

                    // debug_clients(&clients);
                    break;

                case LOOKUP:
                    client = find_client(&clients, pkt.nick);

                    if (client) {
                        sprintf(buf, "ACK %d NICK %s %s PORT %hu",
                                pkt.pkt_n,
                                client->nick,
                                inet_ntoa(client->client_addr.sin_addr),
                                ntohs(client->client_addr.sin_port));

                        send_packet(fd,
                                    buf,
                                    strlen(buf),
                                    0,
                                    (struct sockaddr *) &client_addr,
                                    sizeof(struct sockaddr_in));
                    } else {
                        sprintf(buf, "ACK %d NOT FOUND", pkt.pkt_n);

                        send_packet(fd,
                                    buf,
                                    strlen(buf),
                                    0,
                                    (struct sockaddr *) &client_addr,
                                    sizeof(struct sockaddr_in));
                    }
                    break;
            }

        } else if (FD_ISSET(STDIN_FILENO, &fds)) {
            fgets(buf, BUFSIZE, stdin);
            strip(buf);
            if (strcmp(buf, "QUIT") == 0) {
                free_clients(&clients);
                return EXIT_SUCCESS;

            } else if (strcmp(buf, "DEBUG") == 0) {
                debug_clients(&clients);

            } else {
                printf("Invalid input! Enter QUIT to quit.\n");
            }
        }
    }
}

int parse_msg(char *msg, struct pkt *pkt) {
    char *tok;
    int pkt_type;

    tok = strtok(msg, " ");
    if (tok == NULL || strcmp(tok, "PKT")) {
        return INVALID;
    }

    tok = strtok(NULL, " ");
    if (tok == NULL) {
        return INVALID;
    }
    pkt->pkt_n = atoi(tok);

    tok = strtok(NULL, " ");
    if (tok == NULL) {
        return INVALID;
    }

    if (strcmp(tok, "REG") == 0) {
        pkt_type = REG;

    } else if (strcmp(tok, "LOOKUP") == 0) {
        pkt_type = LOOKUP;

    } else {
        return INVALID;
    }

    tok = strtok(NULL, "\0");
    if (tok == NULL) {
        return INVALID;
    }

    strcpy(pkt->nick, tok);

    return pkt_type;
}

void clients_cleanup(struct client_list *clients) {
    struct client_node *current_client, *prev_client;

    prev_client = NULL;
    current_client = clients->first;

    while (current_client) {
        if (time(NULL) - current_client->last_update >= 30) {
            if (prev_client) {
                prev_client->next = current_client->next;
                free(current_client);
                current_client = prev_client;
            } else {
                clients->first = current_client->next;
                free(current_client);
                current_client = clients->first;

                if (current_client == NULL) { return; }
            }
        }

        prev_client = current_client;
        current_client = current_client->next;
    }
}
