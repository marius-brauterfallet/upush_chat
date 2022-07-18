BIN = upush_client upush_server
CFLAGS = -g -Wall -Wextra -std=gnu11
VFLAGS = --track-origins=yes --malloc-fill=0x40 --free-fill=0x23 --leak-check=full --show-leak-kinds=all

default: $(BIN)

all: upush_client_np upush_server_np

upush_client: upush_client.o common.o common.h send_packet.c
	gcc $(CFLAGS) send_packet.c upush_client.o common.o -o upush_client

upush_server: upush_server.o common.o common.h send_packet.c
	gcc $(CFLAGS) send_packet.c upush_server.o common.o -o upush_server

upush_client.o: upush_client.c
	gcc $(CFLAGS) -c upush_client.c

upush_server.o: upush_server.c
	gcc $(CFLAGS) -c upush_server.c

common.o: common.c
	gcc $(CFLAGS) -c common.c

upush_client_np: upush_client_np.o common_np.o common.h send_packet.c
	gcc send_packet.c upush_client_np.o common_np.o -o upush_client

upush_server_np: upush_server_np.o common_np.o common.h send_packet.c
	gcc send_packet.c upush_server_np.o common_np.o -o upush_server

upush_client_np.o: upush_client.c
	gcc -c upush_client.c -o upush_client_np.o

upush_server_np.o: upush_server.c
	gcc -c upush_server.c -o upush_server_np.o

common_np.o: common.c
	gcc -c common.c -o common_np.o

check_server: upush_server
	valgrind $(VFLAGS) ./upush_server 2000 20

check_client: upush_client
	valgrind $(VFLAGS) ./upush_client Nick 127.0.0.1 2000 2 20
    
clean:
	rm -f $(BIN) *.o
