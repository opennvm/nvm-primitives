//----------------------------------------------------------------------------
//Copyright (c) 2013, Fusion-io, Inc.
//All rights reserved.
//Redistribution and use of this Software in source and binary forms, with or
//without modification, are permitted provided that the following conditions
//are met:
//* Redistributions of source code must retain the above copyright notice,
//  this list of conditions and the following disclaimer.
//* Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//* Neither the name of the Fusion-io, Inc. nor the names of its
//  contributors may be used to endorse or promote products derived from this
//  software without specific prior written permission.
//This Software means only the software included in the file that includes
//this license.
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.
//----------------------------------------------------------------------------

#define __STDC_FORMAT_MACROS
#define  _GNU_SOURCE
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <nvm/nvm_primitives.h>
#include <nvm/nvm_types.h>
#define MAX_DEV_NAME_SIZE 256


/******************************************************************************
 * These macros correspond to the NVM_PRIMITIVES_API_* version macros for
 * the library version for which this code was designed/written/modified.
 * Whenever the code is changed to work with a newer library version, modify
 * the below values too. The values should be taken from the nvm_primitives.h
 * file for the version of the library used when this code was written/modified
 * and compiled.
 ****************************************************************************/
//Major version of NVM library used when compiling this code
#define NVM_MAJOR_VERSION_USED_COMPILATION 1
//Minor version of NVM library used when compiling this code
#define NVM_MINOR_VERSION_USED_COMPILATION 0
//Micro version of NVM library used when compiling this code
#define NVM_MICRO_VERSION_USED_COMPILATION 0

#define CAP_SUP(cap) (cap & NVM_CAP_FEATURE_SUPPORTED)?1:0
#define CAP_ENA(cap) (cap & NVM_CAP_FEATURE_ENABLED)?1:0

//
//Prints the status and value of each capability
//
static void print_cap_status_value(char *msg, uint64_t feature_value,
                                   int32_t retcode, bool is_feature)
{
    if (retcode == NVM_CAP_SUCCESS)
    {
        if (is_feature)
        {
            printf("\t %s:                  Success                  %d"
                   "            %d\n",
                   msg, CAP_SUP(feature_value), CAP_ENA(feature_value));
        }
        else
        {
           printf("\t %s:                         Success              %"PRIu64"\n",
                  msg, feature_value);
        }
    }
    else if (retcode == NVM_CAP_UNKNOWN)
    {
        printf("\t %s:           Failure - Capability id used for querying is"
               " incorrect\n", msg);
    }
    else
    {
        printf("\t %s:           Failure - Is no longer supported by the"
               " version of your driver\n", msg);
    }
}

int main(int argc, char *argv[])
{
    char                         device_name[MAX_DEV_NAME_SIZE];
    char                        *buf = NULL;
    int                          i;
    int                          rc = 0;
    int                          bytes_written = 0;
    int                          fd;
    int                          valid_cap_count = 0;
    int                          num_ranges_found;
    nvm_handle_t                 handle = -1;
    nvm_version_t                version_provided_by_lib;
    nvm_version_t                version_used_during_compilation;
    nvm_capability_t             nvm_cap_array[NVM_CAP_MAX];
    nvm_capacity_t               nvm_capacity;
    nvm_block_range_t            *output_ranges = NULL;
    nvm_block_range_t            *mapped_range = NULL;
    nvm_range_op_t               range_exists;
    nvm_logical_range_iter_t     logical_range_iterator;
    nvm_iovec_t                 *iov = NULL;
    uint64_t                     io_size_in_bytes;
    uint64_t                     iov_size_in_bytes;
    uint64_t                     io_size_in_sectors;
    uint64_t                     nvm_cap_atomic_max_iovs;
    uint64_t                     nvm_cap_atomic_write_multiplicity;
    uint64_t                     nvm_cap_atomic_write_max_vector_size;
    uint64_t                     nvm_cap_atomic_write_start_align;
    uint64_t                     nvm_cap_atomic_trim_max_vector_size;
    uint64_t                     nvm_cap_logical_iter_max_num_ranges;
    uint64_t                     sector_size;
    uint64_t                     block_start, block_end;
    uint64_t                     blocks_mapped = 0;
    uint64_t                     blocks_unmapped = 0;
    uint64_t                     block_index = 0;
    uint64_t                     num_of_calls = 0;
    uint32_t                     batch_size = 0;
    bool atomic_write_available = false;
    bool atomic_trim_available = false;
    uint64_t                     nvm_cap_atomic_write_max_total_size;


    if (argc < 2)
    {
        printf("***** Command line param device name"
               "(e.g. /dev/fioa) missing *****\n");
        goto exit;
    }

    /**************************/
    /* Open the block device  */
    /**************************/
    strncpy(device_name, argv[1], MAX_DEV_NAME_SIZE);
    fd = open(device_name, O_RDWR | O_DIRECT);
    if (fd < 0)
    {
        printf("block device %s open failed, errno = %d\n", device_name, errno);
        goto exit;
    }
    printf(" open: success, fd = %d\n", fd);

    /**************************/
    /* Get the version_info   */
    /**************************/
    printf("************************************************************\n");
    rc = nvm_get_version(&version_provided_by_lib);
    if (rc < 0)
    {
        printf("nvm_get_version: failed, errno %d\n", errno);
        goto exit;
    }

    printf("nvm_get_version: success\nVersion provided by NVM primitives"
           "(major, minor, micro) = (%d, %d, %d)\n",
           version_provided_by_lib.major, version_provided_by_lib.minor,
           version_provided_by_lib.micro);
    printf("************************************************************\n");

    //Send the NVM primitives version we used when writing this code,
    //to get the handle for this fd. This version information is checked
    //for compatibility by the library. Note that you cannot use the version
    //you obtained above from nvm_get_version() call, unless your code has been
    //modified to keep up with the new library version.
    version_used_during_compilation.major = NVM_MAJOR_VERSION_USED_COMPILATION;
    version_used_during_compilation.minor = NVM_MINOR_VERSION_USED_COMPILATION;
    version_used_during_compilation.micro = NVM_MICRO_VERSION_USED_COMPILATION;

    /*************************/
    /* Get the file handle   */
    /*************************/
    printf("************************************************************\n");
    handle = nvm_get_handle(fd, &version_used_during_compilation);
    if (handle < 0)
    {
        printf("nvm_get_handle: failed, errno %d\n", errno);
        goto exit;
    }

    printf("nvm_get_handle: success, handle = %d\n", handle);
    printf("************************************************************\n");

    /**********************************************************************
     * Let's query for capabilities now. To do that, either we can set
     * up the capabilities input array ourselves or pass a large enough
     * array and ask the API to give us all capabilities. For now, we
     * are setting the capability id's ourselves (for all NVM_CAP_MAX
     * capabilities).
     *********************************************************************/
    nvm_cap_array[0].cap_id  = NVM_CAP_FEATURE_SPARSE_ADDRESSING_ID;
    nvm_cap_array[1].cap_id  = NVM_CAP_FEATURE_ATOMIC_WRITE_ID;
    nvm_cap_array[2].cap_id  = NVM_CAP_FEATURE_PTRIM_ID;
    nvm_cap_array[3].cap_id  = NVM_CAP_FEATURE_ATOMIC_TRIM_ID;
    nvm_cap_array[4].cap_id  = NVM_CAP_ATOMIC_MAX_IOV_ID;
    nvm_cap_array[5].cap_id  = NVM_CAP_ATOMIC_WRITE_MULTIPLICITY_ID;
    nvm_cap_array[6].cap_id  = NVM_CAP_ATOMIC_WRITE_MAX_VECTOR_SIZE_ID;
    nvm_cap_array[7].cap_id  = NVM_CAP_ATOMIC_WRITE_START_ALIGN_ID;
    nvm_cap_array[8].cap_id  = NVM_CAP_ATOMIC_TRIM_MAX_VECTOR_SIZE_ID;
    nvm_cap_array[9].cap_id  = NVM_CAP_LOGICAL_ITER_MAX_NUM_RANGES_ID;
    nvm_cap_array[10].cap_id = NVM_CAP_SECTOR_SIZE_ID;
    nvm_cap_array[11].cap_id = NVM_CAP_ATOMIC_WRITE_MAX_TOTAL_SIZE_ID;

    //This particular invocation of the API tells the library that we are
    //requesting for a specific set of capabilities
    printf("************************************************************\n");
    valid_cap_count = nvm_get_capabilities(handle, nvm_cap_array, NVM_CAP_MAX,
                                          false);
    //API failed
    if (valid_cap_count < 0)
    {
        printf("nvm_get_capabilities: failed, errno = %d\n", errno);
        goto exit;
    }
    //API succeeded. Note that API might succeed, but all capabilities requested
    //might not be returned because of errors in input or driver mis-match
    else
    {
        printf("nvm_get_capabilities: success, number of capabilities"
               " retrieved = %d\n\n", valid_cap_count);
        printf("\t\tHere are the NVM capabilities of %s:\n\n", device_name);
        printf("\t        Feature                  Query Status     "
               "\t Supported       Enabled\n");
        print_cap_status_value("Sparse Addressing", nvm_cap_array[0].cap_value,
                               nvm_cap_array[0].retcode, true);
        print_cap_status_value("Atomic Writes", nvm_cap_array[1].cap_value,
                               nvm_cap_array[1].retcode, true);
        print_cap_status_value("Persistent Trim", nvm_cap_array[2].cap_value,
                               nvm_cap_array[2].retcode, true);
        print_cap_status_value("Atomic Trim", nvm_cap_array[3].cap_value,
                               nvm_cap_array[3].retcode, true);
        printf("\n");
        printf("\t        Parameter                        \t\tQuery Status      "
               "\t Value\n");
        print_cap_status_value("Max Number of Atomic Vectors", nvm_cap_array[4].cap_value,
                               nvm_cap_array[4].retcode, false);
        print_cap_status_value("Atomic Write Multiplicity(bytes)", nvm_cap_array[5].cap_value,
                               nvm_cap_array[5].retcode, false);
        print_cap_status_value("Atomic Write Max Vector Size", nvm_cap_array[6].cap_value,
                               nvm_cap_array[6].retcode, false);
        printf("\t (units=Atomic Write Multiplicity)\n");
        print_cap_status_value("Atomic Write Memory Address Alignment", nvm_cap_array[7].cap_value,
                               nvm_cap_array[7].retcode, false);
        printf("\t (units=bytes)\n");
        print_cap_status_value("Atomic Trim Max Vector Size", nvm_cap_array[8].cap_value,
                               nvm_cap_array[8].retcode, false);
        printf("\t (units=Atomic Write Multiplicity)\n");
        print_cap_status_value("Logical Iterator Max Number of Ranges", nvm_cap_array[9].cap_value,
                               nvm_cap_array[9].retcode, false);
        print_cap_status_value("Sector Size(bytes)", nvm_cap_array[10].cap_value,
                               nvm_cap_array[10].retcode, false);
        print_cap_status_value("Atomic write max total size in bytes, whether batched or single",
                               nvm_cap_array[11].cap_value, nvm_cap_array[11].retcode, false);
        printf("************************************************************\n");

        //If we couldn't get all the capabilities we wanted, we exit
        if (valid_cap_count != NVM_CAP_MAX)
        {
            printf("nvm_get_capabilities: couldn't get all "
                   "capabilities, exiting\n");
            goto exit;
        }
    }

    /* save some of the capabilities for use below */
    atomic_write_available                = CAP_ENA(nvm_cap_array[1].cap_value);
    atomic_trim_available                 = CAP_ENA(nvm_cap_array[3].cap_value);
    nvm_cap_atomic_max_iovs               = nvm_cap_array[4].cap_value;
    nvm_cap_atomic_write_multiplicity     = nvm_cap_array[5].cap_value;
    nvm_cap_atomic_write_max_vector_size  = nvm_cap_array[6].cap_value;
    nvm_cap_atomic_write_start_align      = nvm_cap_array[7].cap_value;
    nvm_cap_atomic_trim_max_vector_size   = nvm_cap_array[8].cap_value;
    nvm_cap_logical_iter_max_num_ranges   = nvm_cap_array[9].cap_value;
    sector_size                           = nvm_cap_array[10].cap_value;
    nvm_cap_atomic_write_max_total_size   = nvm_cap_array[11].cap_value;


    /**********************/
    /* Get the capacity   */
    /**********************/
    printf("************************************************************\n");
    rc = nvm_get_capacity(handle, &nvm_capacity);
    if (rc < 0)
    {
        printf("nvm_get_capacity: failed, errno %d\n", errno);
        goto exit;
    }

    printf("nvm_get_capacity: success\n\n");
    printf("\t\tCapacity information for %s:\n\n", device_name);
    printf("\t total physical capacity (in blocks)  =  %"PRIu64"\n",
           nvm_capacity.total_phys_capacity);
    printf("\t used  physical capacity (in blocks)  =  %"PRIu64"\n",
           nvm_capacity.used_phys_capacity);
    printf("\t total logical  capacity (in blocks)  =  %"PRIu64"\n",
           nvm_capacity.total_logical_capacity);
    printf("************************************************************\n");


    /****************************************/
    /* Check whether a block(s) is in use   */
    /****************************************/
    block_start = 0;
    //Do _not_ attempt to query existence of each block on device using
    //nvm_block_exists(). It will take a _very very_ long time. You can use
    //nvm_logical_range_iterator() for that purpose. But even that could
    //take a while for the whole drive, if it is mostly full.
    block_end = 10;
    printf("************************************************************\n");
    printf("nvm_block_exists:\n");
    printf("\tblock range: [%"PRIu64" - %"PRIu64")\n", block_start, block_end);
    for (block_index = block_start; block_index < block_end; block_index++)
    {
        rc = nvm_block_exists(handle, block_index);
        if (rc < 0)
        {
            printf("nvm_block_exists: failed, return %d for block %"PRIu64", errno = %d\n",
                    rc, block_index, errno);
            goto exit;
        }
        else if (rc == 1)
        {
            blocks_mapped++;
        }
        else if (rc == 0)
        {
            blocks_unmapped++;
        }
    }
    printf("number of blocks mapped     = %"PRIu64"\n", blocks_mapped);
    printf("number of blocks unmapped   = %"PRIu64"\n", blocks_unmapped);
    printf("************************************************************\n");

    //Do an atomic write if atomic write feature is enabled on the device
    //This is found from the value of the capability
    //NVM_CAP_FEATURE_ATOMIC_WRITE_ID


    /****************************/
    /* Prepare an atomic write  */
    /* of maximum size.         */
    /*                          */
    /* Align the buffer.        */
    /* Fill the buffer with 'z' */
    /****************************/
    //We will write the maximum amount allowed
    io_size_in_bytes   = nvm_cap_atomic_write_max_total_size;

    /****************************/
    /* Do the atomic write of   */
    /* size io_size_in_bytes    */
    /* start block: 0           */
    /****************************/
    block_start = 0;
    if (atomic_write_available)
    {
        //The pointer to the data that should be written, has to be aligned
        //to value of the capability NVM_CAP_ATOMIC_WRITE_START_ALIGN_ID
        if ((posix_memalign((void **) &buf, nvm_cap_atomic_write_start_align,
                            io_size_in_bytes)) != 0)
        {
            printf("posix_memalign: failed, errno = %d\n", errno);
            goto exit;
        }
        memset(buf, 'z', io_size_in_bytes);

        printf("************************************************************\n");
        printf("Doing an atomic write of (max) size: %"PRIu64" bytes\n",
               io_size_in_bytes);
        bytes_written = nvm_atomic_write(handle, (uint64_t)buf, io_size_in_bytes,
                                         block_start);
        if (bytes_written < 0)
        {
            printf("nvm_atomic_write: failed, errno = %d\n", errno);
            goto exit;
        }

        printf("nvm_atomic_write: success\n");
        printf("\tblocks written:  [%"PRIu64" - %"PRIu64")\n", block_start,
               io_size_in_bytes / sector_size);
        printf("\tbytes written:   %d\n", bytes_written);
        printf("************************************************************\n");
    }

    /******************************************/
    /* Find out the first subrange of mapped  */
    /* blocks within a given range using      */
    /* nvm_range_exists(). Note that the      */
    /* entire range is also a valid subrange  */
    /* and will be returned if it has been    */
    /* written to entirely.                   */
    /*                                        */
    /* input block range: entire range we     */
    /*                    wrote to before     */
    /*                                        */
    /* We just wrote to that range, so        */
    /* they should all be mapped.             */
    /******************************************/
    printf("************************************************************\n");
    range_exists.check_range.start_lba    = 0;
    range_exists.check_range.length       = io_size_in_bytes / sector_size;
    printf("nvm_range_exists:\n");
    printf("\trange start block       = %"PRIu64"\n", range_exists.check_range.start_lba);
    printf("\trange length(in blocks) = %"PRIu64"\n", range_exists.check_range.length);
    rc = nvm_range_exists(handle, &range_exists);
    if (rc < 0)
    {
        printf("nvm_range_exists: failed, errno = %d\n", errno);
        goto exit;
    }
    else
    {
        printf("...success\n");
        //If a mapped subrange was found
        if (range_exists.range_found.length != 0)
        {
            printf("\tSubrange of mapped blocks: [%"PRIu64" - %"PRIu64")\n",
                   range_exists.range_found.start_lba,
                   range_exists.range_found.start_lba
                   + range_exists.range_found.length);
        }
        else
        {
            printf("\tNo mapped subrange found\n");
        }
    }
    printf("************************************************************\n");

    /**************************************/
    /* Do the same as above but for an    */
    /* input range that has not been      */
    /* written to entirely                */
    /*                                    */
    /* A subrange not equal to the entire */
    /* input range should be returned     */
    /**************************************/
    printf("************************************************************\n");
    range_exists.check_range.start_lba    = (io_size_in_bytes / sector_size) - 24;
    range_exists.check_range.length       = 48;
    printf("nvm_range_exists:\n");
    printf("\trange start block       = %"PRIu64"\n", range_exists.check_range.start_lba);
    printf("\trange length(in blocks) = %"PRIu64"\n", range_exists.check_range.length);
    rc = nvm_range_exists(handle, &range_exists);
    if (rc < 0)
    {
        printf("nvm_range_exists: failed, errno = %d\n", errno);
        goto exit;
    }
    else
    {
        printf("...success\n");
        //If a mapped subrange was found
        if (range_exists.range_found.length != 0)
        {
            printf("\tSubrange of mapped blocks: [%"PRIu64" - %"PRIu64")\n",
                   range_exists.range_found.start_lba,
                   range_exists.range_found.start_lba
                   + range_exists.range_found.length);
        }
        else
        {
            printf("\tNo mapped subrange found\n");
        }
    }
    printf("************************************************************\n");

    /************************************************************************/
    /* Perform a batch atomic write with a batch size of 30. First IOV will */
    /* trim data (max allowed) written by previous atomic write. Next IOV's */
    /* will write maximum possible data. Note that the sum of the sizes of  */
    /* all write IOVs should not be more than the maximum allowed. The      */
    /* first write IOV starts at sector 65536.                              */
    /************************************************************************/

    //we check for both atomic write and atomic trim features
    //since we have both trim and write IOVs in our batch
    if (atomic_write_available && atomic_trim_available)
    {
        printf("************************************************************\n");
        batch_size = 30;
        //each write IOV in this batch will write maximum allowed for an IOV
        iov_size_in_bytes = nvm_cap_atomic_write_max_vector_size
            * nvm_cap_atomic_write_multiplicity;
        //Calculate total size of the amount of data to be written atomically
        io_size_in_bytes = batch_size * iov_size_in_bytes;
        //Check if we can write a batch of this size
        //by comparing to the value returned for the capability
        //NVM_CAP_ATOMIC_MAX_IOVS_ID
        //If batch_size is greater, then this batch can be split into multiple
        //batches. But for simplicity, we are not doing that here
        //Also check that the sum of sizes of all write IOVs is not more
        //than the max allowed which is returned by the capability
        //NVM_CAP_ATOMIC_WRITE_MAX_TOTAL_SIZE_ID
        if (batch_size <= nvm_cap_atomic_max_iovs &&
            io_size_in_bytes <= nvm_cap_atomic_write_max_total_size)
        {
            if (buf)
            {
                free(buf);
            }
            if ((posix_memalign((void **) &buf, nvm_cap_atomic_write_start_align,
                                iov_size_in_bytes)) != 0)
            {
                printf("posix_memalign: failed, errno = %d\n", errno);
                goto exit;
            }
            memset((void *) buf, 'a', iov_size_in_bytes);

            iov = (nvm_iovec_t *) malloc(sizeof(nvm_iovec_t) * batch_size);
            if (iov == NULL)
            {
                printf("malloc: failed\n");
                goto exit;
            }

            /******************************/
            /* Prepare the trim vector.   */
            /* Trim IOVs should always be */
            /* be before write IOVs.      */
            /* start sector: where last   */
            /* atomic write started (0).  */
            /* size:  max allowed         */
            /******************************/
            iov[0].iov_lba = 0;
            iov[0].iov_len = nvm_cap_atomic_trim_max_vector_size
                             * nvm_cap_atomic_write_multiplicity;
            iov[0].iov_opcode = NVM_IOV_TRIM;

            /******************************/
            /* prepare each write vector  */
            /* size:  max allowed         */
            /* fill: 'a'              */
            /******************************/
            block_start = 65536;
            for (i = 1; i < batch_size; i++)
            {
                iov[i].iov_base   = (uint64_t)((uintptr_t) buf);
                iov[i].iov_len    = iov_size_in_bytes;
                iov[i].iov_opcode = NVM_IOV_WRITE;
                iov[i].iov_lba    = block_start;
                block_start = iov[i].iov_lba + iov_size_in_bytes / sector_size;
            }

            /**************************************/
            /* perform the batch atomic operation */
            /**************************************/

            bytes_written = nvm_batch_atomic_operations(handle, iov, batch_size, 0);
            if (bytes_written < 0)
            {
                printf("nvm_batch_atomic_operations: failed, errno %d\n", errno);
                goto exit;
            }

            printf("nvm_batch_atomic_operations: success, %d bytes written\n",
                   bytes_written);
            printf("\t trimmed all in use blocks in range: [%"PRIu64" - %"PRIu64")\n",
                   iov[0].iov_lba, iov[0].iov_lba + (iov[0].iov_len / sector_size));
            for (i = 1; i < batch_size; i++)
            {
                printf("\t wrote %u blocks in range: [%"PRIu64" - %"PRIu64")\n",
                       (uint32_t) (iov[i].iov_len / sector_size),
                       iov[i].iov_lba, iov[i].iov_lba + (iov[i].iov_len / sector_size));
            }
            printf("************************************************************\n");
        }
        else
        {
            printf("Number of IOVs more than max allowed or amount of data being "
                   "written atomically exceeds 4MiB\n");
        }
    }


    /*********************************/
    /* Scan the entire block device  */
    /* looking for any block ranges  */
    /* that are currently mapped. We */
    /* use the iterator for this.    */
    /* Iterator returns ranges       */
    /* written contiguously but at   */
    /* different times as a single   */
    /* range                         */
    /*********************************/
    printf("************************************************************\n");
    //The max ranges that the iterator returns is the value of the capability
    //NVM_CAP_LOGICAL_ITER_MAX_NUM_RANGES_ID
    logical_range_iterator.max_ranges                 = nvm_cap_logical_iter_max_num_ranges;
    //Allocate a buffer which can contain as many ranges as the max that can be
    //returned
    output_ranges = (nvm_block_range_t *) malloc(sizeof(nvm_block_range_t)
                                                 * nvm_cap_logical_iter_max_num_ranges);
    if (output_ranges == NULL)
    {
        printf("malloc: failed\n");
        goto exit;
    }

    //Set the search range for iteration to be the entire drive
    logical_range_iterator.range_to_iterate.start_lba = 0;
    //Yes, logical capacity should be used because that is what is exposed to
    //users of the drive
    logical_range_iterator.range_to_iterate.length    = nvm_capacity.total_logical_capacity;
    //Set the "ranges" pointer to the buffer we allocated above
    logical_range_iterator.ranges                     = output_ranges;

    //Time to set the filters for iterator
    //Set mask to 0 so that no bits in LBA will be matched and all LBAs will
    //be returned
    logical_range_iterator.filters.filter_mask = 0;
    //Since no bits are matched, pattern need not be set, but set it to a
    //fixed value anyway
    logical_range_iterator.filters.filter_pattern = 0;
    //Since we want all LBAs that have been written to, we have to disable
    //expiry. To do that, we set it to 0
    logical_range_iterator.filters.filter_expiry = 0;

    printf("nvm_logical_range_iterator:\n");
    printf("\tscan %s looking for mapped ranges\n", device_name);
    //iterator should be called continuously until it fails or it returns less
    //than the max number of ranges it can return
    while (true)
    {
        printf("\tCall %"PRIu64"\n", num_of_calls);
        printf("\t\t[Begin of this call] search range start block       = %"PRIu64"\n",
               logical_range_iterator.range_to_iterate.start_lba);
        printf("\t\t[Begin of this call] search range length(in blocks) = %"PRIu64"\n",
               logical_range_iterator.range_to_iterate.length);
        num_ranges_found = nvm_logical_range_iterator(handle,
                                                      &logical_range_iterator);
        if (num_ranges_found < 0)
        {
            printf("nvm_logical_range_iterator: failed, errno = %d\n", errno);
            goto exit;
        }
        //iterator call succeeded, print if atleast one range was found
        else if (num_ranges_found > 0)
        {
            printf("\t\t...success, found %d mapped ranges\n", num_ranges_found);
            mapped_range = logical_range_iterator.ranges;
            for (i = 0; i < num_ranges_found; i++)
            {
                printf("\t\t mapped block range: [%"PRIu64" - %"PRIu64")\n",
                       mapped_range->start_lba,
                       mapped_range->start_lba + mapped_range->length);
                mapped_range++;
            }
        }
        else
        {
            printf("\t\tno ranges found\n");
        }

        //if max number of ranges were not returned, it means iteration
        //has ended
        if (num_ranges_found != nvm_cap_logical_iter_max_num_ranges)
        {
            printf("iteration ended\n");
            break;
        }
        else
        {
            printf("\t\t[End of this call] search range start block = %"PRIu64"\n",
                   logical_range_iterator.range_to_iterate.start_lba);
            printf("\t\t[End of this call] search range length(in blocks) = %"PRIu64"\n",
                   logical_range_iterator.range_to_iterate.length);
        }
        num_of_calls++;
    }
    printf("************************************************************\n");


    /*********************************/
    /* Perform an atomic trim of the */
    /* sectors returned by the last  */
    /* call to iterator above.       */
    /*********************************/
    mapped_range = logical_range_iterator.ranges;
    if (atomic_trim_available)
    {
        printf("************************************************************\n");

        for (i = 0; i < num_ranges_found; i++)
        {
            block_start = mapped_range->start_lba;
            io_size_in_sectors = mapped_range->length;
            io_size_in_bytes = io_size_in_sectors * sector_size;
            //Check if this io size is a multiple of value of capability
            //NVM_CAP_ATOMIC_WRITE_MULTIPLICITY_ID and not more
            //than max allowed. If its more than max allowed, it can be split into
            //multiple calls to nvm_atomic_trim, but we wont be doing that here.
            if (io_size_in_bytes % nvm_cap_atomic_write_multiplicity == 0
                && io_size_in_bytes <= nvm_cap_atomic_trim_max_vector_size
                * nvm_cap_atomic_write_multiplicity
                * nvm_cap_atomic_max_iovs)
            {
                printf("nvm_atomic_trim:\n");
                printf("\t trim block range: [%"PRIu64" - %"PRIu64")",
                       block_start, block_start + io_size_in_sectors);
                rc = nvm_atomic_trim(handle, block_start, io_size_in_bytes);
                if (rc < 0)
                {
                    printf("nvm_atomic_trim: failed, errno = %d\n", errno);
                    goto exit;
                }
                printf(" ... success\n");
            }
            else
            {
                printf("\t Didn't attempt to trim. Size of this block range in bytes"
                       " is not a multiple of atomic write multiplicity OR its too large"
                       " to trim in one call to nvm_atomic_trim()\n");
            }
            mapped_range++;
        }
        printf("************************************************************\n");
    }

    /***********************/
    /* release the handle  */
    /***********************/
    printf("************************************************************\n");
    rc = nvm_release_handle(handle);
    if (rc < 0)
    {
        printf("nvm_release_handle: failed, errno = %d\n", errno);
        goto exit;
    }
    printf("nvm_release_handle: success\n");
    printf("************************************************************\n");
    handle = -1;


exit:
    /*****************************************/
    /* if we have a valid handle, release it */
    /* we may not have a vaid handle if we   */
    /* are exiting before nvm_get_handle()   */
    /*****************************************/
    if (handle != -1)
    {
        rc = nvm_release_handle(handle);
        if (rc < 0)
        {
            printf("nvm_release_handle: failed, errno = %d\n", errno);
        }
    }
    if(buf)
    {
        free(buf);
    }
    if (iov)
    {
        free(iov);
    }
    if (output_ranges)
    {
        free(output_ranges);
    }
    return 0;
}
