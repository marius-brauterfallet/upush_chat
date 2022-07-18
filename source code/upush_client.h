#ifndef UPUSH_CLIENT
#define UPUSH_CLIENT

#include "common.h"

#define MSGSIZE 1461
#define BUFSIZE 256
#define SAVED_MSGS 3    /* The amount of messages to store for checking for duplicated messages */

enum pkt_type { MSG,
                MSG_WRONG_NAME,
                MSG_WRONG_FORMAT,
                LOOKUP_SUCCESS,
                LOOKUP_NOTFOUND,
                ACK_OK,
                WRONG_NAME,
                WRONG_FORMAT,
                INVALID };


struct pkt {
    int pkt_type, pkt_n;
    char msg[1401], nick[21];
};

struct nick_node {
    struct nick_node *next;
    char nick[21];
};

struct nick_list {
    struct nick_node *first;
};


/**
 * @brief Parses a packet and stores the information in a struct pkt.
 * 
 * @param pkt_string The packet in string form
 * @param pkt A struct to store the information in
 * @param client_nick The nick of the client calling the function
 * @return int An integer describing the packet type according to the enum pkt_type
 */
int parse_pkt(char *pkt_string, struct pkt *pkt, char *client_nick);


/**
 * @brief Prints a message in a user friendly format, unless the message was sent from someone in the blocked list
 * 
 * @param pkt The struct pkt containing the message
 * @param blocked_nicks A list containing all blocked nicks
 */
void print_msg(struct pkt *pkt, struct nick_list *blocked_nicks);


/**
 * @brief Adds a nick to a linked list of nicks, if the list does not alread contain it
 * 
 * @param nicks The list of nicks
 * @param nick The nick to add
 */
void add_nick(struct nick_list *nicks, char *nick);


/**
 * @brief Removes a nick from a linked list of nicks, if the list contains the nick
 * 
 * @param nicks The list of nicks
 * @param nick The nick to remove
 */
void remove_nick(struct nick_list *nicks, char *nick);


/**
 * @brief Checks if a linked list of nicks contains a certain nick
 * 
 * @param nicks The list of nicks
 * @param nick The nick to check
 * @return int 1 if the nick exists, else 0
 */
int has_nick(struct nick_list *nicks, char *nick);


/**
 * @brief Frees all stored nicks in a list of nicks
 * 
 * @param nicks The list of nicks
 */
void free_nicks(struct nick_list *nicks);


/**
 * @brief Prints all nicks in a linked list
 * 
 * @param nicks The list of nicks
 */
void debug_nick_list(struct nick_list *nicks);


/**
 * @brief Attempts to send a message to another client
 * 
 * @param inp The user input, as it was typed into the terminal
 * @param client_nick The nick of the client sending the message
 * @param clients The list of stored client information
 * @param server_addr The address of the Upush server
 * @param fd The file descriptor for the client's socket
 * @param pkt_count Pointer to the client's packet count, mod 10
 * @param timeout The amount of seconds to wait before timing out
 * @param blocked_nicks A linked list containing all blocked nicks
 */
void send_message(char *inp,
                  char *client_nick,
                  struct client_list *clients,
                  struct sockaddr_in *server_addr,
                  int fd,
                  int *pkt_count,
                  int timeout,
                  struct nick_list *blocked_nicks);


/**
 * @brief Looks up a client's information from the Upush server. 
 * 
 * @param nick The nick of the client to look up
 * @param clients The list of stored client information
 * @param server_addr The address of the Upush server
 * @param fd The file descriptor for the client's socket
 * @param pkt_count Pointer to the client's packet count, mod 10
 * @param client_nick The nick of the client sending the request
 * @param timeout The amount of seconds to wait before timing out
 * @param blocked_nicks A linkeed list containing all blocked nicks
 * @param res A pointer to a client_node to store the information at
 * @return int An integer describing the outcome of the operation accordinig to enum pkt_type
 */
int lookup(char *nick,
           struct client_list *clients,
           struct sockaddr_in *server_addr,
           int fd,
           int *pkt_count,
           char *client_nick,
           int timeout,
           struct nick_list *blocked_nicks,
           struct client_node **res);


/**
 * @brief Frees all heap allocated memory and shuts down the client
 * 
 * @param clients A list containing all client information
 * @param blocked_nicks A list containing all blocked nicks
 */
void upush_shutdown(struct client_list *clients, struct nick_list *blocked_nicks);


/**
 * @brief Checks if a msg is a duplicate of a recently received message
 * 
 * @param msgs An array containing the latest received messages
 * @param msg The message to check
 * @return int 
 */
int is_duplicate(struct pkt msgs[], struct pkt *msg);


/**
 * @brief Adds a message to the most recently received messages
 * 
 * @param msgs An array containing the latest received messages
 * @param msg The message to add
 */
void add_msg(struct pkt msgs[], struct pkt *msg);

#endif