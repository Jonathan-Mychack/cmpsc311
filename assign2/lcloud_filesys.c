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

void extract_lcloud_registers(LCloudRegisterFrame resp, unsigned int b0, unsigned int b1, unsigned int c0, unsigned int c1, unsigned int c2, unsigned int d0, unsigned int d1) {
    unsigned int *temp1, *temp2, *temp3, *temp4, *temp5, *temp6, *temp7;
    temp1 = b0;
    temp2 = b1;
    temp3 = c0;
    temp4 = c1;
    temp5 = c2;
    temp6 = d0;
    temp7 = d1;
    //Case 1
    unsigned int shift = resp >> 60;
    *temp1 = shift;
    //Case 2
    shift = (resp << 4) >> 60;
    *temp2 = shift;
    //Case 3
    shift = (resp << 8) >> 56;
    *temp3 = shift;
    //Case 4
    shift = (resp << 16) >> 56;
    *temp4 = shift;
    //Case 5
    shift = (resp << 24) >> 56;
    *temp5 = shift;
    //Case 6
    shift = (resp << 32) >> 48;
    *temp6 = shift;
    //Case 7
    shift = (resp << 48) >> 48;
    *temp7 = shift;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

LcFHandle lcopen( const char *path ) {
    return( 0 ); // Likely wrong
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
    return( 0 );
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
    return( 0 );
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
