////////////////////////////////////////////////////////////////////////////////
//
//  File          : lcloud_client.c
//  Description   : This is the client side of the Lion Clound network
//                  communication protocol.
//
//  Author        : Jonathan Mychack
//  Last Modified : 4/30/20
//

// Include Files
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

// Project Include Files
#include <lcloud_network.h>
#include <cmpsc311_log.h>
#include <lcloud_filesys.h>
#include <cmpsc311_util.h>
#include <lcloud_controller.h>
#include <lcloud_cache.h>


socket_handle = -1;

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_lcloud_bus_request
// Description  : This the client regstateeration that sends a request to the 
//                lion client server.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : reg - the request reqisters for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed

LCloudRegisterFrame client_lcloud_bus_request( LCloudRegisterFrame reg, void *buf ) {

    if (socket_handle == -1) {  //if there is no valid connection, make one
        
        char *ip = LCLOUD_DEFAULT_IP;  //set the IP
        unsigned short port = LCLOUD_DEFAULT_PORT;  //set the port
        struct sockaddr_in address;  //declare 

        address.sin_family = AF_INET;  //establish the fact that the address is IPv4
        address.sin_port = htons(port);  //convert port to something readable for the server

        socket_handle = socket(AF_INET, SOCK_STREAM, 0);  //create a socket for an IPv4 address using TCP

        int connected = connect(socket_handle, (struct sockaddr *)&address, sizeof(address));  //connect the client to the server
        if (connected == -1) {
            return(-1);
        }
    }

    unsigned int b0, b1, c0, c1, c2, d0, d1;  //extract the inputted register opcode to determine what operation must be done
    extract_lcloud_registers(reg, &b0, &b1, &c0, &c1, &c2, &d0, &d1);

    char local_buffer[256] = {0};
    LCloudRegisterFrame rframe;
    if (c0 == LC_BLOCK_XFER) {  //if the operation is a block transfer

        if (c2 == LC_XFER_WRITE) {  //if the type of block transfer is a write

            memcpy(local_buffer, buf, 256);
            uint64_t network_op = htonll64(reg);  //convert the opcode to something readable for the server

            int op_success = write(socket_handle, &network_op, LCLOUD_NET_HEADER_SIZE);  //give the network the opcode
            if (op_success == -1) {
                return(-1);
            }

            int write_success = write(socket_handle, local_buffer, 256);  //give the network the data to be written
                if (write_success == -1) {
                return(-1);
            }

            int read_success = read(socket_handle, &rframe, LCLOUD_NET_HEADER_SIZE);  //receive the network's return opcode
            if (read_success == -1) {
                return(-1);
            }
            rframe = ntohll64(rframe);

            return(rframe);
        }
        else if (c2 == LC_XFER_READ) {  //if the type of block transfer is a read

            uint64_t network_op = htonll64(reg);

            int op_success = write(socket_handle, &network_op, LCLOUD_NET_HEADER_SIZE);  //give opcode
            if (op_success == -1) {
                return(-1);
            }

            int read_success = read(socket_handle, &rframe, LCLOUD_NET_HEADER_SIZE);  //receive return opcode
            if (read_success == -1) {
                return(-1);
            }
            rframe = ntohll64(rframe);

            int buf_success = read(socket_handle, local_buffer, 256);  //read the data from the server with the local buffer
            if (buf_success == -1) {
                return(-1);
            }

            memcpy(buf, local_buffer, 256);
            return(rframe);
        }
    }
    else if (c0 == LC_POWER_OFF) {  //if the operation is a power off

        uint64_t network_op = htonll64(reg);

        int op_success = write(socket_handle, &network_op, LCLOUD_NET_HEADER_SIZE);  //give opcode
        if (op_success == -1) {
            return(-1);
        }

        int read_success = read(socket_handle, &rframe, LCLOUD_NET_HEADER_SIZE);  //receive return opcode
        if (read_success == -1) {
            return(-1);
        }
        rframe = ntohll64(rframe);

        close(socket_handle);  //close the connection and reset the socket handle
        socket_handle = -1;

        return(rframe);
    }
    else {  //if the operation is a device probe, device init, device shut down, etc.

        uint64_t network_op = htonll64(reg);

        int op_success = write(socket_handle, &network_op, LCLOUD_NET_HEADER_SIZE);  //give opcode
        if (op_success == -1) {
            return(-1);
        }

        int read_success = read(socket_handle, &rframe, LCLOUD_NET_HEADER_SIZE);  //receive return opcode
        if (read_success == -1) {
            return(-1);
        }
        rframe = ntohll64(rframe);

        return(rframe);
    }
}

