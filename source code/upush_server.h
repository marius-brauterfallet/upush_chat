#ifndef UPUSH_SERVER
#define UPUSH_SERVER

#include "common.h"

#define BUFSIZE 59          /* Based on the size of the longest message expected */

enum pkt_type { REG, LOOKUP, INVALID };

struct pkt {
    int pkt_n;
    char nick[21];
};

/**
 * @brief Parses a packet and extracts the relevant information into a struct pkt.
 * 
 * @param msg The packet to be parsed
 * @param pkt Pointer to the struct where the extracted information is stored
 * @return int An integer describing the packet type according to the enum pkt_type
 */
int parse_msg(char *msg, struct pkt *pkt);

/**
 * @brief Loops through a client list and removes inactive clients
 * 
 * @param clients The client list to check
 */
void clients_cleanup(struct client_list *clients);

#endif