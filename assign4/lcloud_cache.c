////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_cache.c
//  Description    : This is the cache implementation for the LionCloud
//                   assignment for CMPSC311.
//
//   Author        : Jonathan Mychack
//   Last Modified : 4/30/20
//

// Includes 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmpsc311_log.h>
#include <lcloud_cache.h>

typedef struct {
    int cache_line;
    LcDeviceId device_id;
    uint16_t sector;
    uint16_t block;
    int timestamp;
    char data[256];
} Cache;

Cache *cache;
float hits = 0;
float misses = 0;

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : adjust_timestamps
// Description  : increments all timestamps by one except for the one given by the inputted cache line
//
// Inputs       : cache_line - the cache line number for the data whose timestamp will not be changed
// Outputs      : nothing
void adjust_timestamps(int cache_line) {

    for (int cache_block = 0; cache_block < LC_CACHE_MAXBLOCKS; cache_block++) {

        if ((cache[cache_block].cache_line != cache_line) && (cache[cache_block].timestamp != -1)) {

            cache[cache_block].timestamp += 1;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_getcache
// Description  : Search the cache for a block 
//
// Inputs       : did - device number of block to find
//                sec - sector number of block to find
//                blk - block number of block to find
// Outputs      : cache block if found (pointer), NULL if not or failure

char * lcloud_getcache( LcDeviceId did, uint16_t sec, uint16_t blk ) {

    for (int cache_block = 0; cache_block < LC_CACHE_MAXBLOCKS; cache_block++) {

        int found = 1;  //assume cache location exists for given input
        if (cache[cache_block].device_id != did  || cache[cache_block].sector != sec || cache[cache_block].block != blk) {

            found = 0;  //if the inputted information is not found, set found to 0
        }
        
        if (found == 1) {  //cache hit! fix timestamps and return the cache block's data

            hits += 1;
            cache[cache_block].timestamp = 0;
            adjust_timestamps(cache[cache_block].cache_line);

            logMessage(LOG_INFO_LEVEL, "Found cache item [%d/%d/%d]", did, sec, blk);
            return(cache[cache_block].data);
        }
    }

    misses += 1;  //cache miss, increment miss count and return NULL
    logMessage(LOG_INFO_LEVEL, "Cache item [%d/%d/%d] not found", did, sec, blk);
    /* Return not found */
    return( NULL );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_putcache
// Description  : Put a value in the cache 
//
// Inputs       : did - device number of block to insert
//                sec - sector number of block to insert
//                blk - block number of block to insert
// Outputs      : 0 if succesfully inserted, -1 if failure

int lcloud_putcache( LcDeviceId did, uint16_t sec, uint16_t blk, char *block ) {

    // Error Checks
    for (int cache_block = 0; cache_block < LC_CACHE_MAXBLOCKS; cache_block++) {

        int exists = 1;
        for (int index = 0; index < 256; index++) {

            if (cache[cache_block].data[index] != block[index]) {
                exists = 0;
            }
        }

        if (exists == 1) {
            return(-1);
        }
    }

    if (did == -1 || sec == -1 || blk == -1) {
        return(-1);
    }
    //////

    for (int cache_block = 0; cache_block < LC_CACHE_MAXBLOCKS; cache_block++) {

        if (cache[cache_block].device_id == did && cache[cache_block].sector == sec && cache[cache_block].block == blk) {  //if this location is already in the cache, select its cache line
            break;
        }

        if (cache[cache_block].timestamp == -1) {  //go through all of the cache blocks that haven't been used

            cache[cache_block].timestamp = 0;  //set the values
            cache[cache_block].device_id = did;
            cache[cache_block].sector = sec;
            cache[cache_block].block = blk;
            memcpy(cache[cache_block].data, block, 256);

            /*for (int index = 0; index < 256; index++) {  //insert the data
                cache[cache_block].data[index] = block[index];
            }*/

            adjust_timestamps(cache[cache_block].cache_line);
            logMessage(LOG_INFO_LEVEL, "LionCloud Cache success inserting cache item [%d/%d/%d]", did, sec, blk);
            return(0);
        }
    }

    int least_recent = 0;  //if the code reaches this point, all cache blocks have been used
    int least_recent_line = 0;
    for (int cache_block = 0; cache_block < LC_CACHE_MAXBLOCKS; cache_block++) {

        if (cache[cache_block].device_id == did && cache[cache_block].sector == sec && cache[cache_block].block == blk) {  //if this location is already in the cache, select its cache line

            least_recent = cache[cache_block].timestamp;
            least_recent_line = cache[cache_block].cache_line;
            break;
        }

        if (cache[cache_block].timestamp > least_recent) {  //if the function inputs are not in the cache, select the least recently used cache line to be overwritten

            least_recent = cache[cache_block].timestamp;
            least_recent_line = cache[cache_block].cache_line;
        }   
    }

    if (cache[least_recent_line].device_id == did && cache[least_recent_line].sector == sec && cache[least_recent_line].block == blk) {
        logMessage(LOG_INFO_LEVEL, "Updating cache item [%d/%d/%d]", cache[least_recent_line].device_id, cache[least_recent_line].sector, cache[least_recent_line].block);
    }
    else {
        logMessage(LOG_INFO_LEVEL, "Ejecting cache item [%d/%d/%d]", cache[least_recent_line].device_id, cache[least_recent_line].sector, cache[least_recent_line].block);
    }
    cache[least_recent_line].timestamp = 0;  //set the new values for this line of the cache
    cache[least_recent_line].device_id = did;
    cache[least_recent_line].sector = sec;
    cache[least_recent_line].block = blk;
    memcpy(cache[least_recent_line].data, block, 256);

    /*for (int index = 0; index < 256; index++) {  //insert the data
        cache[least_recent_line].data[index] = block[index];
    }*/
    adjust_timestamps(least_recent_line);
    logMessage(LOG_INFO_LEVEL, "LionCloud Cache success inserting cache item [%d/%d/%d]", did, sec, blk);
    /* Return successfully */
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_initcache
// Description  : Initialze the cache by setting up metadata a cache elements.
//
// Inputs       : maxblocks - the max number number of blocks 
// Outputs      : 0 if successful, -1 if failure

int lcloud_initcache( int maxblocks ) {

    if (maxblocks != LC_CACHE_MAXBLOCKS) {
        return(-1);
    }

    cache = (int*)malloc(maxblocks*sizeof(Cache));

    for (int cache_block = 0; cache_block < maxblocks; cache_block++) {

        cache[cache_block].cache_line = cache_block;
        cache[cache_block].device_id = -1;
        cache[cache_block].sector = -1;
        cache[cache_block].block = -1;
        cache[cache_block].timestamp = -1;

        for (int byte = 0; byte < 256; byte++) {

            cache[cache_block].data[byte] = 0;
        }
    }

    logMessage(LOG_INFO_LEVEL, "init_cmpsc311_cache: initialization complete [%d/%d]", maxblocks, maxblocks*256);
    /* Return successfully */
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_closecache
// Description  : Clean up the cache when program is closing
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int lcloud_closecache( void ) {

    float total_accesses = hits + misses;
    float hit_rate = (hits / total_accesses) * 100;

    logMessage(LOG_INFO_LEVEL, "Closed cmpsc311 cache, deleting 64 items");
    logMessage(LOG_INFO_LEVEL, "Cache hits [%d]", (int)hits);
    logMessage(LOG_INFO_LEVEL, "Cache misses [%d]", (int)misses);
    logMessage(LOG_INFO_LEVEL, "Cache efficiency [%.2f\%]", hit_rate);

    free(cache);

    /* Return successfully */
    return( 0 );
}