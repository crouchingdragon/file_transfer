# file_transfer

Client and server code using UNIX UDP sockets to transfer files connectionlessly.

# Client / deliver.c

Reads data from a file specified by the user and sends it to the server using a UDP socket. File gets segmented into packets if larger than 1000 bytes. Client waits for an ACK after sending a packet within a time period. If the ACK is not received, it resends the packet.

Execution Command:
deliver server_address server_port_number ftp filename

# Server / server.c

Reads file name from first packet and creates a file with the same name on the local system. Data read from subsequent packets gets added to this file until EOF packet is received. Sends an ACK back to the client once it receives a packet successfully.

Execution Command:
server UDP_listen_port
