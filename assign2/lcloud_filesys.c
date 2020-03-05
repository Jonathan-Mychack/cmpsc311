////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_filesys.c
//  Description    : This is the implementation of the Lion Cloud device 
//                   filesystem interfaces.
//
//   Author        : Jonathan Mychack
//   Last Modified : 3/2/20
//

// Include files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project include files
#include <lcloud_filesys.h>
#include <lcloud_controller.h>

//
// File system interface implementation

LCloudRegisterFrame create_lcloud_registers(uint64_t b0, uint64_t b1, uint64_t c0, uint64_t c1, uint64_t c2, uint64_t d0, uint64_t d1) {
    unsigned int temp1 = b0 << 60;
    unsigned int temp2 = b1 << 56;
    unsigned int temp3 = c0 << 48;
    unsigned int temp4 = c1 << 40;
    unsigned int temp5 = c2 << 32;
    unsigned int temp6 = d0 << 16;
    LCloudRegisterFrame frame = temp1 | temp2 | temp3 | temp4 | temp5 | temp6 | d1;
    return(frame);
}

void extract_lcloud_registers(LCloudRegisterFrame resp, unsigned int *a_b0, unsigned int *a_b1, unsigned int *a_c0, unsigned int *a_c1, unsigned int *a_c2, unsigned int *a_d0, unsigned int *a_d1) {
    unsigned int *temp1, *temp2, *temp3, *temp4, *temp5, *temp6, *temp7;
    temp1 = a_b0;
    temp2 = a_b1;
    temp3 = a_c0;
    temp4 = a_c1;
    temp5 = a_c2;
    temp6 = a_d0;
    temp7 = a_d1;

    unsigned int shift = resp >> 60;  //Case 1
    *temp1 = shift;
    shift = (resp << 4) >> 60;  //Case 2
    *temp2 = shift;
    shift = (resp << 8) >> 56;  //Case 3
    *temp3 = shift;
    shift = (resp << 16) >> 56;  //Case 4
    *temp4 = shift;
    shift = (resp << 24) >> 56;  //Case 5
    *temp5 = shift;
    shift = (resp << 32) >> 48;  //Case 6
    *temp6 = shift;
    shift = (resp << 48) >> 48;  //Case 7
    *temp7 = shift;
}

int power_on(void) {
    unsigned int b0, b1, c0, c1, c2, d0, d1;
    LCloudRegisterFrame frame = create_lcloud_registers(0, 0, LC_POWER_ON, 0, 0, 0, 0);
    LCloudRegisterFrame rframe = lcloud_io_bus(frame, NULL);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_POWER_ON)) {
        return (-1);
    }
    return(0);
}

/*int active_devices[16];
int i = 0;
int device_probe() {
    unsigned int b0, b1, c0, c1, c2, d0, d1;
    LCloudRegisterFrame frame = create_lcloud_registers(0, 0, LC_DEVPROBE, 0, 0, 0, 0);
    LCloudRegisterFrame rframe = lcloud_io_bus(frame, NULL);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_DEVPROBE)) {
        return(-1);
    }
    
    while (d0 > 0) {
        int bit = d0 % 2;
        if (bit == 1) {
            active_devices[i] = i;
        }
    }
}*/

int get_block(int *buffer, int device_id, int sector, int block) {
    unsigned int b0, b1, c0, c1, c2, d0, d1;
    LCloudRegisterFrame frame = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_id, LC_XFER_READ, sector, block);
    LCloudRegisterFrame rframe = lcloud_io_bus(frame, *buffer);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_BLOCK_XFER)) {
        return(-1);
    }
    return(0);
}

int put_block(int *buffer, int device_id, int sector, int block) {
    unsigned int b0, b1, c0, c1, c2, d0, d1;
    LCloudRegisterFrame frame = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_id, LC_XFER_WRITE, sector, block);
    LCloudRegisterFrame rframe = lcloud_io_bus(frame, *buffer);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_BLOCK_XFER)) {
        return(-1);
    }
    return(0);
}

typedef struct {
    char filename;
    int handle;
    int position;
    int length;
    int blocks[40][2];
} File;

//Variables
File file_array[256];
int f_array_index = 0;

int used_data_locations[10][64];
char local_buf[256];

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

LcFHandle lcopen( const char *path ) {
    power_on();

    for (int i = 0; i < 256; i++) {
        if (file_array[f_array_index].filename == *path) {
            return(-1);
        }
    }

    int handle = 1;
    file_array[f_array_index].handle = handle;
    file_array[f_array_index].filename = *path;
    file_array[f_array_index].position = 0;
    file_array[f_array_index].length = 0;
    for (int i = 0; i < 40; i++) {
        file_array[f_array_index].blocks[i][0] = -1;
        file_array[f_array_index].blocks[i][1] = -1;
    }
    f_array_index += 1;

    return(file_array[f_array_index].handle);
} 

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcread
// Description  : Read data from the file 
//
// Inputs       : fh - file handle for the file to read from
//                buf - place to put the data
//                len - the length of the read
// Outputs      : number of bytes read, -1 if failure
int lcread( LcFHandle fh, char *buf, size_t len ) {

    int open;
    int byte_count = 0;
    int file_array_location;

    // Error Checks //
    for (int i = 0; i < 256; i++) {  //check if file handle exists in open file array
        if (file_array[i].handle == fh) {
            open = 1;
            file_array_location = i;
        }
    }

    if (open != 1) {  //if no file has the handle, as determined in the loop, the function fails
        return(-1);
    }

    if (len > 4096) {  //check if operation size is over the max of 4096 bytes
        return(-1);
    }
    //////////////////

    for (int i = 0; i < 40; i++) {
        int sector = file_array[file_array_location].blocks[i][0];
        int block = file_array[file_array_location].blocks[i][1];
        if ((sector != -1) && (block != -1)) {
            int read = get_block(buf, 5, sector, block);
            if (read == 0) {
                int file_size = file_array[file_array_location].length;
                file_array[file_array_location].position += 256;
            }
            else {
                return(-1);
            }
        }
    }

    return(len);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcwrite
// Description  : write data to the file
//
// Inputs       : fh - file handle for the file to write to
//                buf - pointer to data to write
//                len - the length of the write
// Outputs      : number of bytes written if successful test, -1 if failure

int lcwrite( LcFHandle fh, char *buf, size_t len ) {

    int open;
    int byte_count = 0;
    int file_array_location;
    int sector = 0, block = 0;

    // Error Checks //
    for (int i = 0; i < 256; i++) {  //check if file handle exists in open file array
        if (file_array[i].handle == fh) {
            open = 1;
            file_array_location = i;
        }
    }

    if (open != 1) {  //if no file has the handle, as determined in the loop, the function fails
        return(-1);
    }

    if (len > 4096) {  //check if operation size is over the max of 4096 bytes
        return(-1);
    }
    //////////////////

    char read_buf[256];
    int total_writes = len / 256;
    int byte_remainder = len % 256;
    if (byte_remainder != 0) {
        total_writes += 1;
    }
    int size_remaining = len;
    for (int i = 0; i < total_writes; i++) {

        if (used_data_locations[sector][block] == 0) {

            if (size_remaining >= 256) {
                memcpy(*local_buf, &buf[i*256], 256);
                int write = put_block(*local_buf, 5, sector, block);

                if (write == -1) {
                    return(-1);
                }

                size_remaining -= 256;
                used_data_locations[sector][block] = 1;
                file_array[file_array_location].position += 256;
                file_array[file_array_location].length += 256;
                file_array[file_array_location].blocks[i][0] = sector;
                file_array[file_array_location].blocks[i][1] = block;

                if (block == 63) {  //check if at end of blocks, if so choose new sector
                    sector += 1;
                    block = 0;
                }
                else {  //choose new block otherwise
                    block += 1;
                }
            }
            else {
                memcpy(*local_buf, &buf[i*256], size_remaining);
                int write = put_block(*local_buf, 5, sector, block);

                if (write == -1) {
                    return(-1);
                }

                used_data_locations[sector][block] = 1;
                file_array[file_array_location].position += size_remaining;
                file_array[file_array_location].length += size_remaining;
                file_array[file_array_location].blocks[i][0] = sector;
                file_array[file_array_location].blocks[i][1] = block;
                size_remaining -= size_remaining;
            }
        }
        else{
            int read_bytes = lcread(fh, *read_buf, file_array[file_array_location].length);
            
            if (read_bytes < (file_array[file_array_location].length + ))
        }
    }
    return(len);
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcseek
// Description  : Seek to a specific place in the file
//
// Inputs       : fh - the file handle of the file to seek in
//                off - offset within the file to seek to
// Outputs      : 0 if successful test, -1 if failure

int lcseek( LcFHandle fh, size_t off ) {
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcclose
// Description  : Close the file
//
// Inputs       : fh - the file handle of the file to close
// Outputs      : 0 if successful test, -1 if failure

int lcclose( LcFHandle fh ) {
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcshutdown
// Description  : Shut down the filesystem
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int lcshutdown( void ) {

    return( 0 );
}
