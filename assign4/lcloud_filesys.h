#ifndef LCLOUD_FILESYS_INCLUDED
#define LCLOUD_FILESYS_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_filesys.h
//  Description    : This is the declaration of interface of the Lion
//                   Cloud device filesystem interface.
//
//   Author        : Patrick McDaniel
//   Last Modified : Sat Jan 25 09:30:06 PST 2020
//

// Includes
#include <stddef.h>
#include <stdint.h>
#include <lcloud_controller.h>

// Defines 

// Type definitions
typedef int32_t LcFHandle;

// File system interface definitions

LCloudRegisterFrame create_lcloud_registers(uint64_t b0, uint64_t b1, uint64_t c0, uint64_t c1, uint64_t c2, uint64_t d0, uint64_t d1);
    // Pack 64 bit registers for the system

void extract_lcloud_registers(LCloudRegisterFrame resp, unsigned int *a_b0, unsigned int *a_b1, unsigned int *a_c0, unsigned int *a_c1, unsigned int *a_c2, unsigned int *a_d0, unsigned int *a_d1);
    // Unpack 64 bit registers

LcFHandle lcopen( const char *path );
    // Open the file for for reading and writing

int lcread( LcFHandle fh, char *buf, size_t len );
    // Read data from the file hande

int lcwrite( LcFHandle fh, char *buf, size_t len );
    // Write data to the file

int lcseek( LcFHandle fh, size_t off );
    // Seek to a specific place in the file

int lcclose( LcFHandle fh );
    // Close the file

int lcshutdown( void );
    // Shut down the filesystem

#endif
