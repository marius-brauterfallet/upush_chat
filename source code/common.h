#ifndef UPUSH_COMMON
#define UPUSH_COMMON

#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "send_packet.h"

struct client_node {
    struct client_node *next;
    char nick[21];
    struct sockaddr_in client_addr;
    time_t last_update;
};

struct client_list {
    struct client_node *first;
};


/**
 * @brief Checks for an error and calls perror() if rc equals -1
 * 
 * @param rc The integer to check
 * @param msg The message used in perror()
 */
void check_error(int rc, char *msg);


/**
 * @brief Strips the beginning and end of a string of blank spaces
 * 
 * @param str The string to strip
 */
void strip(char *str);


/**
 * @brief Allocates memory for and returns a pointer to a newly created struct client_node
 * 
 * @param nick The nick of the client
 * @param client_addr The sockaddr_in of the client
 * @return The newly created client_node pointer
 */
struct client_node* make_client_node(char *nick, struct sockaddr_in *client_addr);


/**
 * @brief Adds a client node to the client list. If the list already contains a client node with the same nick, the old
 *  client_node is replaced.
 * 
 * @param clients The client list
 * @param client The client to add
 */
void add_client(struct client_list *clients, struct client_node *client);


/**
 * @brief Removes the client with the specified nick if such a client exists
 * 
 * @param clients The client list
 * @param nick The nick of the client to remove
 */
void remove_client(struct client_list *clients, char *nick);


/**
 * @brief Finds and returns the client with the specified nick.
 * 
 * @param clients The client list
 * @param nick The nick of the client to find
 * @return A pointer to the client node, or NULL if no client with the nick exists
 */
struct client_node* find_client(struct client_list *clients, char *nick);


/**
 * @brief Frees all the clients of a client list.
 * 
 * @param clients The client list
 */
void free_clients(struct client_list *clients);


/**
 * @brief Prints all the clients of a client list in readable format.
 * 
 * @param clients The client list
 */
void debug_clients(struct client_list *clients);

#endif