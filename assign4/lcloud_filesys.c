////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_filesys.c
//  Description    : This is the implementation of the Lion Cloud device 
//                   filesystem interfaces.
//
//   Author        : Jonathan Mychack
//   Last Modified : 4/30/20
//

// Include files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project include files
#include <lcloud_filesys.h>
#include <lcloud_controller.h>
#include <lcloud_cache.h>
#include <lcloud_support.h>
#include <lcloud_network.h>

//
// File system interface implementation

typedef struct {
    char filename[1024];
    int handle;
    int position;
    int length;
    int blocks[2048][3];  //order of indices is sector, block, device array index
} File;

typedef struct {
    int num_sectors;
    int num_blocks;
    int id;
    int **used_locations;
} Device;

//Variables
File open_files_array[256];
Device active_devices_array[16];
int chosen_location[3];
int active_devices[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int f_array_index = 0;
int handle = 1;
int powered_on = 0;
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
    LCloudRegisterFrame rframe = client_lcloud_bus_request(frame, NULL);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_POWER_ON)) {
        return(-1);
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
// Outputs      : 0 if success, -1 if failure
int device_probe(void) {
    unsigned int b0, b1, c0, c1, c2, d0, d1;
    LCloudRegisterFrame frame = create_lcloud_registers(0, 0, LC_DEVPROBE, 0, 0, 0, 0);
    LCloudRegisterFrame rframe = client_lcloud_bus_request(frame, NULL);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_DEVPROBE)) {
        return(-1);
    }
    
    int bit = 0;
    int active_devices_index = 0;
    while (d0 > 0) {
        int remainder = d0 % 2;
        d0 /= 2;
        if (remainder == 1) {
            active_devices[active_devices_index] = bit;
            active_devices_index += 1;
        }
        bit += 1;
    }

    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : device_init
// Description  : finds the number or sectors and blocks for each device and 
//                dynamically allocates a space to record which locations are available for use
// Inputs       : nothing
//                
//                
// Outputs      : 0 if success, -1 if failure
int device_init(void) {
    unsigned int b0, b1, c0, c1, c2, d0, d1;
    int index = 0;
    for (int id = 0; id < 16; id++) {

        if (active_devices[id] != -1) {

            LCloudRegisterFrame frame = create_lcloud_registers(0, 0, LC_DEVINIT, active_devices[id], 0, 0, 0);
            LCloudRegisterFrame rframe = client_lcloud_bus_request(frame, NULL);
            extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
            if ((b0 != 1) || (b1 != 1) || (c0 != LC_DEVINIT)) {
                return(-1);
            }

            active_devices_array[index].used_locations = (int**)calloc(d0, sizeof(int*));
            for (int i = 0; i < d0; i++) {
                active_devices_array[index].used_locations[i] = (int*)calloc(d1, sizeof(int));
            }

            active_devices_array[index].num_sectors = d0;
            active_devices_array[index].num_blocks = d1;
            active_devices_array[index].id = active_devices[id];
            index += 1;

        }
    }

    return(0);
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
    LCloudRegisterFrame rframe = client_lcloud_bus_request(frame, buffer);
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
    LCloudRegisterFrame rframe = client_lcloud_bus_request(frame, buffer);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_BLOCK_XFER)) {
        return(-1);
    }
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : choose_location
// Description  : selects a sector and block from a device to be used for writing
//
// Inputs       : nothing
//                
//                
// Outputs      : 0 if success, -1 if failure
int choose_location(void) {

    for (int device = 0; device < 15; device++) {

        if (active_devices_array[device].id != NULL) {

            for (int sector = 0; sector < active_devices_array[device].num_sectors; sector++) {

                for (int block = 0; block < active_devices_array[device].num_blocks; block++) {

                    if (active_devices_array[device].used_locations[sector][block] == 0) {

                        chosen_location[0] = sector;
                        chosen_location[1] = block;
                        chosen_location[2] = device;
                        return(0);
                    }
                }
            }
        }
    }

    return(-1);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

LcFHandle lcopen( const char *path ) {

    if (powered_on == 0) {
        power_on();
        device_probe();
        device_init();
        lcloud_initcache(LC_CACHE_MAXBLOCKS);
        powered_on = 1;
    }



    for (int i = 0; i < 256; i++) {
        if (open_files_array[f_array_index].filename == path) {
            return(-1);
        }
    }



    strcpy(open_files_array[f_array_index].filename, path);
    open_files_array[f_array_index].handle = handle;
    open_files_array[f_array_index].position = 0;
    open_files_array[f_array_index].length = 0;
    for (int i = 0; i < 2048; i++) {
        open_files_array[f_array_index].blocks[i][0] = -1;
        open_files_array[f_array_index].blocks[i][1] = -1;
        open_files_array[f_array_index].blocks[i][2] = -1;
    }
    f_array_index += 1;
    handle += 1;



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



    /* Error Checks */
    for (int i = 0; i < 256; i++) {  //check if file handle exists in open file array
        if (open_files_array[i].handle == fh) {
            open = 1;
            location = i;
            break;
        }
    }

    if (open != 1) {  //if no file has the handle, as determined in the loop, the function fails
        return(-1);
    }

    if (len > open_files_array[location].length) {  //check if operation size is over the size of the file
        len = open_files_array[location].length;
    }

    
    
    /* Reading */
    if (open_files_array[location].length == 0) {
        return(0);
    }

    

    int count = 0;
    int total_possible_reads = (LC_MAX_OPERATION_SIZE / 2) + 2;

    for (int read = 0; read < total_possible_reads; read++) {

        char buffer[256] = {0};
        char *cache_buffer;

        int section = open_files_array[location].position / 256;  //use the markers in the file struct to determine which sector and block the current position is in
        int temp_sector = open_files_array[location].blocks[section][0];
        int temp_block = open_files_array[location].blocks[section][1];
        int device_index = open_files_array[location].blocks[section][2];
        int index = open_files_array[location].position % 256;  //starting index to be used for the buffer that reads the data
        int block_space_remaining = 256 - index;

        cache_buffer = lcloud_getcache(active_devices_array[device_index].id, temp_sector, temp_block);  //check if the desired block is in the cache

        if (cache_buffer == NULL) {  //if there was a cache miss, make an io call
            get_block(buffer, active_devices_array[device_index].id, temp_sector, temp_block);
            lcloud_putcache(active_devices_array[device_index].id, temp_sector, temp_block, buffer);
        }
        else {  //if there was cache hit, fill up the local buffer with the block info from the cache

            memcpy(buffer, cache_buffer, 256);
            /*for (int i = 0; i < 256; i++) {

                buffer[i] = cache_buffer[i];
            }*/
        }
        logMessage(LcDriverLLevel, "Success reading blkc [%d/%d/%d].", active_devices_array[device_index].id, temp_sector, temp_block);

        if ((len - count) <= block_space_remaining) {  //if the current read can be done without exceeding the block space
            int temp = len - count;
            for (int i = 0; i < temp; i++) {

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



    /* Error Checks */
    for (int i = 0; i < 256; i++) {  //check if file handle exists in open file array
        if (open_files_array[i].handle == fh) {
            open = 1;
            location = i;
            break;
        }
    }

    if (open != 1) {  //if no file has the handle, as determined in the loop, the function fails
        return(-1);
    }



    /* Writing */
    int count = 0;
    int total_possible_writes = (LC_MAX_OPERATION_SIZE / 256) + 2;
    int bytes_in_block = 0;
    int write_size;
    int overwrite = 0;  //marker for whether an overwrite is desired
    int device_index;

    if (open_files_array[location].position != open_files_array[location].length) {

        overwrite = 1;
    }

    for (int write = 0; write < total_possible_writes; write++) {

        char buffer[256] = {0};

        if (((open_files_array[location].position % 256) == 0) && (overwrite == 0)) {  //select a new location to write into if the current block has been filled
            
            choose_location();
            sector = chosen_location[0];
            block = chosen_location[1];
            device_index = chosen_location[2];

            logMessage(LcDriverLLevel, "Allocated block for data [%d/%d/%d]", active_devices_array[device_index].id, sector, block);
        }
        else {//if (overwrite == 1) { //if an overwrite is necessary, go to the location where the overwrite is occurring

            int section = open_files_array[location].position / 256;
            sector = open_files_array[location].blocks[section][0];
            block = open_files_array[location].blocks[section][1];
            device_index = open_files_array[location].blocks[section][2];
        }
    
        if (((open_files_array[location].position % 256) != 0)) {  //if the position is in the middle of a block

            int temp_pos = open_files_array[location].position;
            int read_pos = open_files_array[location].position - (open_files_array[location].position % 256);  //the position of the beginning of the current block
            lcseek(fh, read_pos);

            bytes_in_block = lcread(fh, buffer, temp_pos % 256);  //read up until the latest byte
            int block_space_remaining = 256 - bytes_in_block;
            if (overwrite == 1) {
                open_files_array[location].position = read_pos;
                lcread(fh, buffer, 256);
                open_files_array[location].position = temp_pos;
            }

            if ((len - count) <= block_space_remaining) {  //if the current write can fit in the rest of the block

                write_size = len - count;
            }
            else {

                write_size = block_space_remaining;
            }
        }
        else if (((open_files_array[location].position % 256) == 0)) {  //if the position is at the beginning of a block

            bytes_in_block = 0;
            if (overwrite == 1) {
                int temp_pos = open_files_array[location].position;
                lcread(fh, buffer, 256);
                open_files_array[location].position = temp_pos;
            }

            if ((len - count) <= 256) {  //if the current write can fit in the entire block

                write_size = len - count;
            }
            else {

                write_size = 256;
            }
        }

        memcpy(&buffer[bytes_in_block], &buf[count], write_size);
        lcloud_putcache(active_devices_array[device_index].id, sector, block, buffer);  //update the cache with new information
        put_block(buffer, active_devices_array[device_index].id, sector, block);

        int section = open_files_array[location].length / 256;  //make note of which sector, block, and device was used for this part of the file
        if (open_files_array[location].blocks[section][0] == -1) {
            open_files_array[location].blocks[section][0] = sector;
            open_files_array[location].blocks[section][1] = block;
            open_files_array[location].blocks[section][2] = device_index;
        }
        if (active_devices_array[device_index].used_locations[sector][block] == 0) {  //mark this sector and block as used if it hasn't been already
            active_devices_array[device_index].used_locations[sector][block] = 1;
        }

        count += write_size;
        if (open_files_array[location].position == open_files_array[location].length) {  //only change the length if writing after current length of the file
            open_files_array[location].length += write_size;
        }
        else if ((overwrite == 1) && ((open_files_array[location].position + write_size) > open_files_array[location].length)) {
            int difference = open_files_array[location].length - open_files_array[location].position;
            open_files_array[location].length += write_size - difference;
        }
        open_files_array[location].position += write_size;

        logMessage(LcDriverLLevel, "LC success writing blkc [%d/%d/%d].", active_devices_array[device_index].id, sector, block);

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
// Outputs      : the file position if successful, -1 if failure

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
    return(open_files_array[location].position);
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
    for (int i = 0; i < 2048; i++) {
        if (open_files_array[location].blocks[i][0] != -1) {
            int device_id = active_devices_array[open_files_array[location].blocks[i][2]].id;
            for (int device = 0; device < 16; device++) {
                if (active_devices_array[device].id == device_id) {
                    active_devices_array[device].used_locations[open_files_array[location].blocks[i][0]][open_files_array[location].blocks[i][1]] = 0;
                    logMessage(LcDriverLLevel, "Deallocated block for data [%d/%d/%d]", device_id, open_files_array[location].blocks[i][0], open_files_array[location].blocks[i][1]);
                }
            }
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
    LCloudRegisterFrame rframe = client_lcloud_bus_request(frame, NULL);
    extract_lcloud_registers(rframe, &b0, &b1, &c0, &c1, &c2, &d0, &d1);
    if ((b0 != 1) || (b1 != 1) || (c0 != LC_POWER_OFF)) {
        return (-1);
    }



    for (int device = 0; device < 16; device++) {
        for (int sector = 0; sector < active_devices_array[device].num_sectors; sector++) {
            free(active_devices_array[device].used_locations[sector]);
        }
        free(active_devices_array[device].used_locations);
    }



    lcloud_closecache();
    logMessage(LcDriverLLevel, "Powered off the LionCloud system.");
    return(0);
}
