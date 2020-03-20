////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_filesys.c
//  Description    : This is the implementation of the Lion Cloud device 
//                   filesystem interfaces.
//
//   Author        : Jonathan Mychack
//   Last Modified : 3/18/20
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

typedef struct {
    char filename[1024];
    int handle;
    int position;
    int length;
    int device_id;
    int blocks[40][2];
} File;

//Variables
File open_files_array[256];
int f_array_index = 0;

int used_data_locations[LC_DEVICE_NUMBER_SECTORS][LC_DEVICE_NUMBER_BLOCKS];
int sector = 0, block = 0;
//

////////////////////////////////////////////////////////////////////////////////
//
// Function     : create_lcloud_registers
// Description  : packs register opcodes for the system to interpret
//
// Inputs       : the various bit sequences of the register, b0 thru d1
// Outputs      : frame - a 64-bit packed register
LCloudRegisterFrame create_lcloud_registers(uint64_t b0, uint64_t b1, uint64_t c0, uint64_t c1, uint64_t c2, uint64_t d0, uint64_t d1) {
    uint64_t temp1 = b0 << 60;
    uint64_t temp2 = b1 << 56;
    uint64_t temp3 = c0 << 48;
    uint64_t temp4 = c1 << 40;
    uint64_t temp5 = c2 << 32;
    uint64_t temp6 = d0 << 16;
    LCloudRegisterFrame frame = temp1 | temp2 | temp3 | temp4 | temp5 | temp6 | d1;
    return(frame);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : extract_lcloud_registers
// Description  : unpacks the opcodes sent back to the driver from the device
//
// Inputs       : resp - the 64-bit packed response from the device
//                the addresses of the various bit sequences of the register, b0 thru d1
//                
// Outputs      : nothing
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

////////////////////////////////////////////////////////////////////////////////
//
// Function     : power_on
// Description  : sends an opcode to devices to turn them on
//
// Inputs       : nothing
//                
//                
// Outputs      : 0 if success, -1 if failure
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

////////////////////////////////////////////////////////////////////////////////
//
// Function     : device_probe
// Description  : determines which devices are active
//
// Inputs       : nothing
//                
//                
// Outputs      : the device id if success, -1 if failure
int device_probe(void) {
    unsigned int b0, b1, c0, c1, c2, d0, d1;
    LCloudRegisterFrame frame = create_lcloud_registers(0, 0, LC_DEVPROBE, 0, 0, 0, 0);
    LCloudRegisterFrame rframe = lcloud_io_bus(frame, NULL);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_DEVPROBE)) {
        return(-1);
    }
    
    int bit = 0;
    while (d0 > 0) {
        int remainder = d0 % 2;
        d0 /= 2;
        if (remainder == 1) {
            return(bit);
        }
        bit += 1;
    }

    return(-1);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_block
// Description  : reads the specified block and puts the contents into a buffer
//
// Inputs       : buffer - a local buffer for the data
//                device_id - the id of the device containing the desired block
//                sector - the sector the data is in
//                block - the block the data is in
// Outputs      : 0 if success, -1 if failure
int get_block(char *buffer, int device_id, int sector, int block) {
    unsigned int b0, b1, c0, c1, c2, d0, d1;
    LCloudRegisterFrame frame = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_id, LC_XFER_READ, sector, block);
    LCloudRegisterFrame rframe = lcloud_io_bus(frame, buffer);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_BLOCK_XFER)) {
        return(-1);
    }
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : put_block
// Description  : writes to the specified block using a buffer
//
// Inputs       : buffer - a local buffer containing the data to write
//                device_id - the id of the device that is written into
//                sector - the sector to be written into
//                block - the block to be written into
// Outputs      : 0 if success, -1 if failure
int put_block(char *buffer, int device_id, int sector, int block) {
    unsigned int b0, b1, c0, c1, c2, d0, d1;
    LCloudRegisterFrame frame = create_lcloud_registers(0, 0, LC_BLOCK_XFER, device_id, LC_XFER_WRITE, sector, block);
    LCloudRegisterFrame rframe = lcloud_io_bus(frame, buffer);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_BLOCK_XFER)) {
        return(-1);
    }
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

LcFHandle lcopen( const char *path ) {
    power_on();
    int device_id = device_probe();



    for (int i = 0; i < 256; i++) {
        if (open_files_array[f_array_index].filename == path) {
            return(-1);
        }
    }



    int handle = 1;
    strcpy(open_files_array[f_array_index].filename, path);
    open_files_array[f_array_index].handle = handle;
    open_files_array[f_array_index].position = 0;
    open_files_array[f_array_index].length = 0;
    open_files_array[f_array_index].device_id = device_id;
    for (int i = 0; i < 40; i++) {
        open_files_array[f_array_index].blocks[i][0] = -1;
        open_files_array[f_array_index].blocks[i][1] = -1;
    }
    f_array_index += 1;



    return(open_files_array[f_array_index-1].handle);
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
    int location;
    char buffer[256];



    /* Error Checks */
    for (int i = 0; i < 256; i++) {  //check if file handle exists in open file array
        if (open_files_array[i].handle == fh) {
            open = 1;
            location = i;
        }
    }

    if (open != 1) {  //if no file has the handle, as determined in the loop, the function fails
        return(-1);
    }

    if (len > open_files_array[location].length) {  //check if operation size is over the size of the file
        return(-1);
    }

    
    
    /* Reading */
    if (open_files_array[location].length == 0) {
        return(0);
    }

    

    int count = 0;
    int total_possible_reads = (LC_MAX_OPERATION_SIZE / 2) + 2;

    for (int read = 0; read < total_possible_reads; read++) {

        int section = open_files_array[location].position / 256;  //use the markers in the file struct to determine which sector and block the current position is in
        int temp_sector = open_files_array[location].blocks[section][0];
        int temp_block = open_files_array[location].blocks[section][1];
        int index = open_files_array[location].position % 256;  //starting index to be used for the buffer that reads the data
        int block_space_remaining = 256 - index;

        get_block(buffer, open_files_array[location].device_id, temp_sector, temp_block);
        logMessage(LcDriverLLevel, "Success reading blkc [%d/%d/%d].", open_files_array[location].device_id, temp_sector, temp_block);

        if ((len - count) <= block_space_remaining) {  //if the current read can be done without exceeding the block space

            for (int i = 0; i < (len - count); i++) {

                buf[count++] = buffer[index + i];
                open_files_array[location].position += 1;
            }
        }
        else {

            for (int i = 0; i < block_space_remaining; i++) {

                buf[count++] = buffer[index + i];
                open_files_array[location].position += 1;
            }
        }

        if (count == len) {

            return(count);
        }
    }

    return(-1);
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
    int location;
    char buffer[256];



    /* Error Checks */
    for (int i = 0; i < 256; i++) {  //check if file handle exists in open file array
        if (open_files_array[i].handle == fh) {
            open = 1;
            location = i;
        }
    }

    if (open != 1) {  //if no file has the handle, as determined in the loop, the function fails
        return(-1);
    }

    if (len > 4096) {  //check if operation size is over the max of 4096 bytes
        return(-1);
    }



    /* Writing */
    int count = 0;
    int total_possible_writes = (LC_MAX_OPERATION_SIZE / 256) + 2;
    int bytes_in_block = 0;
    int write_size;
    int overwrite = 0;  //marker for whether an overwrite is desired

    if (open_files_array[location].position != open_files_array[location].length) {

        overwrite = 1;
    }

    for (int write = 0; write < total_possible_writes; write++) {

        sector = open_files_array[location].position / 2560;  //changes sectors every 10 blocks in order to mimic the sample output
        block = (open_files_array[location].position / 256) - (sector * 10);  //uses 10 sectors then resets to 0

        if ((open_files_array[location].position % 256) == 0) {  //if a new block is selected, give a notification about its allocation
            
            logMessage(LcDriverLLevel, "Allocated block for data [%d/%d/%d]", open_files_array[location].device_id, sector, block);
        }
    
        if (((open_files_array[location].position % 256) != 0) && (overwrite == 0)) {  //if the position is in the middle of a block

            int temp_pos = open_files_array[location].position;
            int read_pos = open_files_array[location].position - (open_files_array[location].position % 256);  //the position of the beginning of the current block
            lcseek(fh, read_pos);
            bytes_in_block = lcread(fh, buffer, temp_pos % 256);  //read up until the latest byte
            int block_space_remaining = 256 - bytes_in_block;

            if ((len - count) <= block_space_remaining) {  //if the current write can fit in the rest of the block

                write_size = len - count;
            }
            else {

                write_size = block_space_remaining;
            }
        }
        else {  //if the position is at the beginning of a block

            bytes_in_block = 0;

            if ((len - count) <= 256) {  //if the current write can fit in the entire block

                write_size = len - count;
            }
            else {

                write_size = 256;
            }
        }

        memcpy(&buffer[bytes_in_block], &buf[count], write_size);
        put_block(buffer, open_files_array[location].device_id, sector, block);

        int section = open_files_array[location].length / 256;  //make note of which sector and block was used for this part of the file
        open_files_array[location].blocks[section][0] = sector;
        open_files_array[location].blocks[section][1] = block;
        if (used_data_locations[sector][block] == 0) {  //mark this sector and block as used if it hasn't been already
            used_data_locations[sector][block] = 1;
        }

        count += write_size;
        if (open_files_array[location].position == open_files_array[location].length) {  //only change the length if writing after current length of the file
            open_files_array[location].length += write_size;
        }
        open_files_array[location].position += write_size;

        logMessage(LcDriverLLevel, "LC success writing blkc [%d/%d/%d].", open_files_array[location].device_id, sector, block);

        if (count == len) {

            logMessage(LcDriverLLevel, "Driver wrote %d bytes to file %s (now %d bytes)", count, open_files_array[location].filename, open_files_array[location].length);
            return(count);
        }
    }

    return(-1);
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
    
    int location;
    int open;
    for (int i = 0; i < 256; i++) {  //check if file handle exists in open file array
        if (open_files_array[i].handle == fh) {
            location = i;
            open = 1;
        }
    }

    if (open != 1) {
        return(-1);
    }

    if (off > open_files_array[location].length) {
        return(-1);
    }

    logMessage(LcDriverLLevel, "Seeking to position %d in file handle %d [%s]", off, open_files_array[location].handle, open_files_array[location].filename);
    open_files_array[location].position = off;
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
    
    int location;
    int open;
    for (int i = 0; i < 256; i++) {  //check if file handle exists in open file array
        if (open_files_array[i].handle == fh) {
            location = i;
            open = 1;
        }
    }

    if (open != 1) {
        return(-1);
    }

    open_files_array[location].position = 0;
    for (int i = 0; i < 40; i++) {
        if (open_files_array[location].blocks[i][0] != -1) {
            used_data_locations[open_files_array[location].blocks[i][0]][open_files_array[location].blocks[i][1]] = 0;
            logMessage(LcDriverLLevel, "Deallocated block for data [%d/%d/%d]", open_files_array[location].device_id, open_files_array[location].blocks[i][0], open_files_array[location].blocks[i][1]);
        }
        open_files_array[location].blocks[i][0] = -1;
        open_files_array[location].blocks[i][1] = -1;
    }
    open_files_array[location].handle = -1;

    logMessage(LcDriverLLevel, "Closed file handle %d [%s]", fh, open_files_array[location].filename);
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcshutdown
// Description  : Shut down the filesystem
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int lcshutdown( void ) {
    unsigned int b0, b1, c0, c1, c2, d0, d1;
    LCloudRegisterFrame frame = create_lcloud_registers(0, 0, LC_POWER_OFF, 0, 0, 0, 0);
    LCloudRegisterFrame rframe = lcloud_io_bus(frame, NULL);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_POWER_OFF)) {
        return (-1);
    }
    logMessage(LcDriverLLevel, "Powered off the LionCloud system.");
    return(0);
}
