#include "upush_client.h"

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: %s, <nick> <server ip> <server port> <timeout> <loss probability>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Variables */
    struct sockaddr_in client_addr, server_addr, src_addr;
    struct timeval tv, hb;
    struct nick_list blocked_nicks;
    struct client_list clients;
    struct pkt pkt, last_msgs[SAVED_MSGS];
    socklen_t src_addr_len;
    fd_set fds;
    int rc, fd, pkt_count, timeout, i;
    char buf_in[MSGSIZE], buf_out[MSGSIZE], nick[21], *tok;

    
    /*
    srand48(3);     // Setting seed for testing in WSL
    */
    
    
    if (strlen(argv[1]) > 20) {
        printf("Nick may be at most 20 chars long!\n");
        return EXIT_FAILURE;
    }
    
    strcpy(nick, argv[1]);
    timeout = atoi(argv[4]);
    last_msgs[0].pkt_type = 0;
    for (i = 0; i < SAVED_MSGS; i++) {
        last_msgs[i].nick[0] = 0;
    }
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    hb.tv_sec = 10;
    hb.tv_usec = 0;
    blocked_nicks.first = NULL;
    clients.first = NULL;
    set_loss_probability(atoi(argv[5]) * .01f);
    pkt_count = 0;

    server_addr.sin_family = AF_INET;
    inet_aton(argv[2], &server_addr.sin_addr);
    server_addr.sin_port = htons(atoi(argv[3]));

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = 0;
    client_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(fd, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_in));
    check_error(rc, "bind()");

    /* Initial registration */

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    sprintf(buf_out, "PKT %d REG %s", pkt_count, nick);
    send_packet(fd, buf_out, strlen(buf_out), 0, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));

    rc = select(FD_SETSIZE, &fds, 0, 0, &tv);
    check_error(rc, "select()");

    if (!FD_ISSET(fd, &fds)) {
        fprintf(stderr, "Could not connect to server. Exitting..\n");
        return EXIT_FAILURE;
    }

    rc = recv(fd, buf_in, BUFSIZE - 1, 0);
    check_error(rc, "recv()");
    buf_in[rc] = 0;

    rc = parse_pkt(buf_in, &pkt, nick);
    if (rc != ACK_OK) {
        fprintf(stderr, "Could not connect to server. Exitting..\n");
        return EXIT_FAILURE;
    }

    printf("\nWelcome to UPush Chat!\n\nEnter @username <msg> to message someone!\n"
            "You can also use BLOCK or UNBLOCK <username> to block someone. Enter QUIT to quit.\n\n");


    /* Main loop */

    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(STDIN_FILENO, &fds);

        select(FD_SETSIZE, &fds, 0, 0, &hb);

        // User input
        if (FD_ISSET(STDIN_FILENO, &fds)) {
            fgets(buf_in, MSGSIZE, stdin);
            strip(buf_in);

            if (strcmp(buf_in, "QUIT") == 0) {
                upush_shutdown(&clients, &blocked_nicks);
                return 0;
            }

            if (buf_in[0] == '@' && strlen(buf_in) > 1) {
                send_message(buf_in, nick, &clients, &server_addr, fd, &pkt_count, timeout, &blocked_nicks);

            } else {
                tok = strtok(buf_in, " ");
                if (tok == NULL || (strcmp(tok, "BLOCK") && strcmp(tok, "UNBLOCK"))) {
                    printf("Invalid input!\n");

                } else if (strcmp(tok, "BLOCK") == 0) {
                    tok = strtok(NULL, "\0");
                    if (tok) {
                        add_nick(&blocked_nicks, tok);
                        printf("\nBlocked %s\n", tok);
                    }

                } else {
                    tok = strtok(NULL, "\0");
                    if (tok) {
                        remove_nick(&blocked_nicks, tok);
                        printf("\nUnblocked %s\n", tok);
                    }
                }
            }
        }

        // Socket input
        if (FD_ISSET(fd, &fds)) {
            src_addr_len = sizeof(struct sockaddr_in);
            rc = recvfrom(fd, buf_in, MSGSIZE - 1, 0, (struct sockaddr*) &src_addr, &src_addr_len);
            check_error(rc, "recvfrom()");
            buf_in[rc] = 0;

            rc = parse_pkt(buf_in, &pkt, nick);
            if (rc == MSG) {
                if (!is_duplicate(last_msgs, &pkt)) {
                    
                    add_msg(last_msgs, &pkt);
                    print_msg(&pkt, &blocked_nicks);   
                }
                sprintf(buf_out, "ACK %d OK", pkt.pkt_n);
                send_packet(fd, buf_out, strlen(buf_out), 0, (struct sockaddr*) &src_addr, sizeof(struct sockaddr_in));
            }
        }

        // Sending heartbeat
        if (hb.tv_sec == 0) {
            sprintf(buf_out, "PKT %d REG %s", pkt_count, nick);
            pkt_count = (pkt_count + 1) % 10;
            send_packet(fd, buf_out, strlen(buf_out), 0, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));
            hb.tv_sec = 10;
        }
    }
}

int parse_pkt(char *pkt_string, struct pkt *pkt, char *client_nick) {
    char *tok;
    pkt->pkt_type = INVALID;

    tok = strtok(pkt_string, " ");
    if (tok == NULL) {
        return INVALID;
    }

    if (strcmp(tok, "PKT") == 0) {
        pkt->pkt_type = MSG;
    } else if (strcmp(tok, "ACK")) {
        return INVALID;
    }

    tok = strtok(NULL, " ");
    if (tok == NULL) {
        return INVALID;
    }

    pkt->pkt_n = atoi(tok);

    if (pkt->pkt_type == MSG) {
        tok = strtok(NULL, " ");
        if (tok == NULL || strcmp(tok, "FROM")) {
            return WRONG_FORMAT;
        }

        tok = strtok(NULL, " ");
        if (tok == NULL) {
            return WRONG_FORMAT;
        }
        strcpy(pkt->nick, tok);

        tok = strtok(NULL, " ");
        if (tok == NULL || strcmp(tok, "TO")) {
            return WRONG_FORMAT;
        }

        tok = strtok(NULL, " ");
        if (tok == NULL) {
            return WRONG_FORMAT;
        }
        if (strcmp(tok, client_nick)) {
            pkt->pkt_type = WRONG_NAME;
            return WRONG_NAME;
        }

        tok = strtok(NULL, " ");
        if (tok == NULL || strcmp(tok, "MSG")) {
            return WRONG_FORMAT;
        }

        tok = strtok(NULL, "\0");
        if (tok == NULL) {
            return WRONG_FORMAT;
        }
        strcpy(pkt->msg, tok);
        return MSG;
    }

    tok = strtok(NULL, " ");
    if (tok == NULL) {
        return INVALID;
    }

    if (strcmp(tok, "OK") == 0) {
        pkt->pkt_type = ACK_OK;
        return ACK_OK;
    }

    if (strcmp(tok, "NOT") == 0) {
        tok = strtok(NULL, "\0");
        if (tok == NULL || strcmp(tok, "FOUND")) {
            return INVALID;
        }

        pkt->pkt_type = LOOKUP_NOTFOUND;
        return LOOKUP_NOTFOUND;
    }

    if (strcmp(tok, "WRONG") == 0) {
        tok = strtok(NULL, "\0");
        if (tok == NULL) {
            return INVALID;
        }
        if (strcmp(tok, "NAME") == 0) {
            return MSG_WRONG_NAME;
        }
        if (strcmp(tok, "FORMAT") == 0) {
            return MSG_WRONG_FORMAT;
        }
    }

    if (strcmp(tok, "NICK") == 0) {
        tok = strtok(NULL, " ");
        if (tok == NULL || strlen(tok) > 20) {
            return INVALID;
        }

        strcpy(pkt->nick, tok);

        tok = strtok(NULL, " ");
        if (tok == NULL) {
            return INVALID;
        }

        strcpy(pkt->msg, tok);

        tok = strtok(NULL, " ");
        if (tok == NULL || strcmp(tok, "PORT")) {
            return INVALID;
        }

        tok = strtok(NULL, "\0");
        if (tok == NULL) {
            return INVALID;
        }
        strcpy(&pkt->msg[16], tok);
        pkt->pkt_type = LOOKUP_SUCCESS;
        return LOOKUP_SUCCESS;
    }

    return INVALID;
}

void print_msg(struct pkt *pkt, struct nick_list *blocked_nicks) {
    if (!has_nick(blocked_nicks, pkt->nick)) {
        printf("[%s] %s\n", pkt->nick, pkt->msg);
    }
}

void add_nick(struct nick_list *nicks, char *nick) {
    struct nick_node *current_node = nicks->first;

    if (strlen(nick) > 20) { return; }

    if (current_node == NULL) {
        nicks->first = malloc(sizeof(struct nick_node));
        nicks->first->next = NULL;
        strcpy(nicks->first->nick, nick);
        return;
    }

    while (current_node->next) {
        if (strcmp(current_node->nick, nick) == 0) {
            return;
        }
        current_node = current_node->next;
    }

    if (strcmp(current_node->nick, nick) == 0) {
        return;
    }

    current_node->next = malloc(sizeof(struct nick_node));
    strcpy(current_node->next->nick, nick);
    current_node->next->next = NULL;
}

void remove_nick(struct nick_list *nicks, char *nick) {
    struct nick_node *current_node, *prev_node;

    if (strlen(nick) > 20) { return; }

    if (nicks->first == NULL) {
        return;
    }

    current_node = nicks->first;
    prev_node = NULL;

    while (current_node) {
        if (strcmp(current_node->nick, nick) == 0) {
            if (prev_node == NULL) {
                nicks->first = current_node->next;
                free(current_node);
                return;
            }

            prev_node->next = current_node->next;
            free(current_node);
            return;
        }

        prev_node = current_node;
        current_node = current_node->next;
    }
}

int has_nick(struct nick_list *nicks, char *nick) {
    struct nick_node *current_node = nicks->first;

    while (current_node) {
        if (strcmp(current_node->nick, nick) == 0) {
            return 1;
        }

        current_node = current_node->next;
    }

    return 0;
}

void free_nicks(struct nick_list *nicks) {
    struct nick_node *current_node, *prev_node;

    prev_node = NULL;
    current_node = nicks->first;

    while (current_node) {
        prev_node = current_node;
        current_node = current_node->next;

        free(prev_node);
    }
}

void debug_nick_list(struct nick_list *nicks) {
    struct nick_node *current_node = nicks->first;

    printf("\nDebugging:\n");

    while (current_node) {
        printf("%s\n", current_node->nick);

        current_node = current_node->next;
    }
}

void send_message(char *inp,
                  char *client_nick,
                  struct client_list *clients,
                  struct sockaddr_in *server_addr,
                  int fd,
                  int *pkt_count,
                  int timeout,
                  struct nick_list *blocked_nicks) {

    char buf_out[MSGSIZE], buf_in[MSGSIZE], *tok, rec_nick[21], ack_out[BUFSIZE];
    struct client_node *rec_client;
    struct sockaddr_in src_addr;
    struct pkt pkt;
    struct timeval tv;
    socklen_t src_addr_len = sizeof(struct sockaddr_in);
    int rc, i, j, k;
    fd_set fds;


    strcpy(buf_in, &inp[1]);
    tok = strtok(buf_in, " ");
    if (tok == NULL) { return; }

    strcpy(rec_nick, tok);
    if (has_nick(blocked_nicks, rec_nick)) {
        printf("The nick %s is blocked and needs to be unblocked before receiving messages!\n", rec_nick);
        return;
    }

    tok = strtok(NULL, "\0");
    if (tok == NULL) { return; }

    sprintf(buf_out, "PKT x FROM %s TO %s MSG %s", client_nick, rec_nick, tok);

    for (i = 0; i < 2; i++) {
        rec_client = find_client(clients, rec_nick);

        /* Client not stored locally */
        if (rec_client == NULL) {
            rc = lookup(rec_nick,
                        clients,
                        server_addr,
                        fd,
                        pkt_count,
                        client_nick,
                        timeout,
                        blocked_nicks,
                        &rec_client);

            if (rc != LOOKUP_SUCCESS) { return; }
        }

        buf_out[4] = *pkt_count + 48;
        *pkt_count = (*pkt_count + 1) % 10;
        for (j = 0; j < 3; j++) {
            send_packet(fd,
                        buf_out,
                        strlen(buf_out),
                        0,
                        (struct sockaddr*) &rec_client->client_addr,
                        sizeof(struct sockaddr_in));
            
            /* This only loops if the packet received is a new message, in which case k is decremented, and it waits
               for another packet without sending more than 3 message packets */
            for (k = 0; k < 1; k++) {
                FD_ZERO(&fds);
                FD_SET(fd, &fds);

                tv.tv_sec = timeout;
                tv.tv_usec = 0;

                rc = select(FD_SETSIZE, &fds, 0, 0, &tv);
                check_error(rc, "select()");

                if (FD_ISSET(fd, &fds)) {
                    rc = recvfrom(fd, buf_in, MSGSIZE - 1, 0, (struct sockaddr*) &src_addr, &src_addr_len);
                    check_error(rc, "recvfrom()");

                    buf_in[rc] = 0;

                    rc = parse_pkt(buf_in, &pkt, client_nick);


                    switch (rc) {
                        case MSG:
                            print_msg(&pkt, blocked_nicks);
                            sprintf(ack_out, "ACK %d OK", pkt.pkt_n);
                            send_packet(fd,
                                        ack_out,
                                        strlen(ack_out),
                                        0,
                                        (struct sockaddr*) &src_addr,
                                        sizeof(struct sockaddr_in));
                            k--;
                            break;

                        case ACK_OK:
                            if (pkt.pkt_n == (*pkt_count - 1) % 10) { return; }
                            break;

                        case WRONG_FORMAT:
                            fprintf(stderr, "Packet format error! Exitting..\n");
                            upush_shutdown(clients, blocked_nicks);
                            break;

                        case WRONG_NAME:
                            /* If the client receives a WRONG NAME packet, it should try looking up the nick from the
                               server once more. Therefore j is set to 3 so that it won't try sending the same packet
                               to the same client with the wrong name */
                            j = 3;
                            break;
                    }
                }
            }
        }

        // Remove client 
        remove_client(clients, rec_nick);
    }

    fprintf(stderr, "NICK %s UNREACHABLE\n", rec_nick);
}

int lookup(char *nick,
           struct client_list *clients,
           struct sockaddr_in *server_addr,
           int fd,
           int *pkt_count,
           char *client_nick,
           int timeout,
           struct nick_list *blocked_nicks,
           struct client_node **res) {

    struct sockaddr_in client_addr, src_addr;
    socklen_t src_addr_len = sizeof(struct sockaddr_in);
    struct pkt pkt;
    struct timeval tv;
    char buf_out[BUFSIZE], ack_out[BUFSIZE], buf_in[MSGSIZE];
    int i, j, rc;
    fd_set fds;

    sprintf(buf_out, "PKT %d LOOKUP %s", *pkt_count, nick);
    *pkt_count = (*pkt_count + 1) % 10;

    for (i = 0; i < 3; i++) {
        send_packet(fd, buf_out, strlen(buf_out), 0, (struct sockaddr*) server_addr, sizeof(struct sockaddr_in));

        // Putting this in a "loop" where j is decremented if select is activated by anything other than the server
        for (j = 0; j < 1; j++) {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            tv.tv_sec = timeout;
            tv.tv_usec = 0;

            select(FD_SETSIZE, &fds, 0, 0, &tv);

            if (FD_ISSET(fd, &fds)) {
                rc = recvfrom(fd, buf_in, MSGSIZE - 1, 0, (struct sockaddr*) &src_addr, &src_addr_len);
                check_error(rc, "recvfrom()");
                buf_in[rc] = 0;

                rc = parse_pkt(buf_in, &pkt, client_nick);

                if (rc == MSG) {
                    print_msg(&pkt, blocked_nicks);
                    sprintf(ack_out, "ACK %d OK", pkt.pkt_n);
                    send_packet(fd,
                                ack_out,
                                strlen(ack_out),
                                0,
                                (struct sockaddr*) &src_addr,
                                sizeof(struct sockaddr_in));
                    j--;

                } else if (rc == LOOKUP_SUCCESS) {
                    client_addr.sin_family = AF_INET;
                    inet_aton(pkt.msg, &client_addr.sin_addr);
                    client_addr.sin_port = htons(atoi(&pkt.msg[16]));

                    *res = make_client_node(nick, &client_addr);
                    add_client(clients, *res);
                    return LOOKUP_SUCCESS;

                } else if (rc == LOOKUP_NOTFOUND) {
                    fprintf(stderr, "NICK %s NOT REGISTERED\n", nick);
                    return LOOKUP_NOTFOUND;

                } else {
                    j--;
                }
            }
        }
    }

    fprintf(stderr, "SERVER NOT RESPONDING! EXITTING..\n");
    upush_shutdown(clients, blocked_nicks);
    return -1;
}

void upush_shutdown(struct client_list *clients, struct nick_list *blocked_nicks) {
    free_clients(clients);
    free_nicks(blocked_nicks);
    exit(EXIT_FAILURE);
}

int is_duplicate(struct pkt msgs[], struct pkt *msg) {
    int i;

    for (i = 0; i < SAVED_MSGS; i++) {
        if (msgs[i].nick[0] == 0) {
            return 0;
        }

        if ((msgs[i].pkt_n == msg->pkt_n || (msgs[i].pkt_n + 2) % 10 == msg->pkt_n)
            && strcmp(msgs[i].nick, msg->nick) == 0
            && strcmp(msgs[i].msg, msg->msg) == 0) {
            
            return 1;
        }
    }
    return 0;
}

void add_msg(struct pkt msgs[], struct pkt *msg) {
    int i = msgs[0].pkt_type;

    msgs[i].pkt_n = msg->pkt_n;
    strcpy(msgs[i].nick, msg->nick);
    strcpy(msgs[i].msg, msg->msg);

    msgs[0].pkt_type = (msgs[0].pkt_type + 1) % SAVED_MSGS;
}
