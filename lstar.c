/*
 * Copyright (c) 2023-2024 Ian Marco Moffett and the Osmora Team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#define ALIGN_UP(value, align)        (((value) + (align)-1) & ~((align)-1))

struct tar_hdr {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char link_name[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char dev_major[8];
    char dev_minor[8];
    char prefix[155];
};

/*
 * Convert size from octal to base 10.
 */
static uint32_t
getsize(struct tar_hdr *hdr)
{
    size_t size = 0, count = 1;

    for (size_t j = 11; j > 0; --j, count *= 8) {
        size += (hdr->size[j-1] - '0') * count;
    }

    return size;
}

/*
 * Read the contents of the file.
 */
static char *
read_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    size_t size;
    char *buf = NULL;

    if (fp == NULL) {
        return NULL;
    }

    /* Get the filesize */
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Must be a multiple of 512 bytes */
    if ((size & (512 - 1)) != 0) {
        printf("error: Size of '%s' is not a multiple of 512 bytes!\n", filename);
        goto done;
    }

    /* Allocate the buffer */
    buf = malloc(size + 1);
    if (buf == NULL) {
        goto done;
    }

    fread(buf, sizeof(char), size, fp);
done:
    fclose(fp);
    return buf;
}

/*
 * List the files within the archive
 */
static void
list_files(const char *tarbuf)
{
    uintptr_t addr;
    struct tar_hdr *hdr;

    addr = (uintptr_t)tarbuf;
    hdr = (void *)addr;

    /*
     * If the magic isn't set, we can assume we've reached
     * the end of the archive. TAR archives are made up of
     * 512 byte blocks. Even if the file doesn't completely
     * fill a block, the rest will be padded to the nearest
     * 512 byte boundary.
     */
    while (hdr->magic[0] != 0) {
        printf("%s\n", hdr->filename);
        addr += 512 + ALIGN_UP(getsize(hdr), 512);
        hdr = (void *)addr;
    }
}

int
main(int argc, char **argv)
{
    char *buf;

    if (argc < 2) {
        printf("Usage: %s <tar file>\n", argv[0]);
        return -1;
    }

    for (size_t i = 1; i < argc; ++i) {
        /* Try to see if this file exists */
        if (access(argv[i], F_OK) != 0) {
            printf("lstar: %s: %s\n", argv[i], strerror(errno));
            return -1;
        }

        /* Try to read in the file */
        buf = read_file(argv[i]);
        if (buf == NULL) {
            return -1;
        }

        list_files(buf);
        free(buf);
    }

    return 0;
}
