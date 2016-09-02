/*
 *   Copyright (c) 2015-2016, Andrew Romanenko <melanhit@gmail.com>
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of the project nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 *   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 *   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

#include <rndpassw.h>

#include "table.h"

static void memzero(volatile void *p, size_t len)
{
    volatile uint8_t *_p = p;

    while(len--)
        *_p++=0;
}

static void print_usage(void)
{
    fprintf(stderr,"Usage: rndpassw [-dulpsvh] <passlen> <passcnt>\n");
}

static void print_version(void)
{
    printf("%s\n", VERSION);
}

int main(int argc, char **argv)
{
    flags_t flags = { 0 };
    size_t passlen, passcnt;
    size_t baselen, mixlen, entlen;
    int i, j, opt, fd, ret;
    char t, *mixoff, *mixbuf = NULL;
    unsigned char *entoff, *entbuf = NULL, *passbuf = NULL;

    ret = EXIT_SUCCESS;

    /* parse command options */
    while((opt = getopt(argc, argv, "dlupshv")) != -1) {
        switch(opt) {
            case 'd':
                flags.d = 1;
                break;

            case 'l':
                flags.l = 1;
                break;

            case 'u':
                flags.u = 1;
                break;

            case 'p':
                flags.p = 1;
                break;

            case 's':
                flags.d = 1;
                flags.l = 1;
                flags.u = 1;
                flags.p = 1;
                break;

            case 'h':
                print_usage();
                return EXIT_SUCCESS;

            case 'v':
                print_version();
                return EXIT_SUCCESS;

            default:
                print_usage();
                return EXIT_FAILURE;
        }
    }

    /* check for passlen and passcnt */
    if((argc - optind) > 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    passlen = 0;
    if(optind < argc) {
        passlen = strtol(argv[optind], NULL, 10);
        if((passlen > MAX_PASSLEN) || (passlen < MIN_PASSLEN)) {
            fprintf(stderr, "Invalid password length\n");
            return EXIT_FAILURE;
        }
        optind++;
    }

    passcnt = 0;
    if(optind < argc) {
        passcnt = strtol(argv[optind], NULL, 10);
        if((passcnt > MAX_PASSCNT) || (passcnt <= 0)) {
            fprintf(stderr, "Invalid passwords count\n");
            return EXIT_FAILURE;
        }
        optind++;
    }

    if(!passlen)
        passlen = DEF_PASSLEN;

    if(!passcnt)
        passcnt = DEF_PASSCNT;

    /* if no options - set default */
    if(!(flags.d + flags.l + flags.u + flags.p)) {
        flags.d = 1; flags.l = 1; flags.u = 1;
    }

    /* prepear parts for calc dirctionary length */
    baselen = 0;
    if(flags.p) {
        baselen = TBL_PUNCT_LEN;
    }

    if(flags.l) {
        if(baselen < TBL_LALPHA_LEN)
            baselen = TBL_LALPHA_LEN;
    }

    if(flags.u) {
        if(baselen < TBL_UALPHA_LEN)
            baselen = TBL_UALPHA_LEN;
    }

    if(flags.d) {
        if(baselen < TBL_DIGIT_LEN)
            baselen = TBL_DIGIT_LEN;
    }

    /* calculate needed pool length and dictionary len */
    mixlen = baselen * (flags.d + flags.l + flags.u + flags.p);

    entlen = 2 * mixlen + (passcnt * passlen);
    entlen += (flags.d * (baselen - TBL_DIGIT_LEN));
    entlen += (flags.l * (baselen - TBL_LALPHA_LEN));
    entlen += (flags.u * (baselen - TBL_UALPHA_LEN));
    entlen += (flags.p * (baselen - TBL_PUNCT_LEN));

    /* read random data into pool */
    fd = open("/dev/urandom", O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "Could not open /dev/urandom\n");
        return EXIT_FAILURE;
    }

    entoff = entbuf = malloc(entlen);
    if(entbuf == NULL) {
        fprintf(stderr, "Could not allocate %zd bytes\n", entlen);
        ret = EXIT_FAILURE;
        goto out;
    }

    if(read(fd, entbuf, entlen) != entlen) {
        fprintf(stderr, "Could not read %zd bytes from /dev/urandom\n", entlen);
        ret = EXIT_FAILURE;
        goto out;
    }

    /* create and fill dictionary */
    mixbuf = malloc(mixlen);
    if(mixbuf == NULL) {
        fprintf(stderr, "Could not allocate %zd bytes of memory\n", mixlen);
        ret = EXIT_FAILURE;
        goto out;
    }

    /* digit */
    memcpy(mixbuf, tbl_digit, TBL_DIGIT_LEN * flags.d);
    mixoff = mixbuf + (TBL_DIGIT_LEN * flags.d);
    for(i = 0; i < ((flags.d * baselen) - (TBL_DIGIT_LEN * flags.d)); i++) {
        mixoff[0] = tbl_digit[entoff[0] % TBL_DIGIT_LEN];
        mixoff++; entoff++;
    }

    /* lower alpha */
    memcpy(mixoff, tbl_lalpha, TBL_LALPHA_LEN * flags.l);
    mixoff += TBL_LALPHA_LEN * flags.l;
    for(i = 0; i < ((flags.l * baselen) - (TBL_LALPHA_LEN * flags.l)); i++) {
        mixoff[0] = tbl_lalpha[entoff[0] % TBL_LALPHA_LEN];
        mixoff++; entoff++;
    }

    /* upper alpha */
    memcpy(mixoff, tbl_ualpha, TBL_UALPHA_LEN * flags.u);
    mixoff += TBL_UALPHA_LEN * flags.u;
    for(i = 0; i < ((flags.u * baselen) - (TBL_UALPHA_LEN * flags.u)); i++) {
        mixoff[0] = tbl_ualpha[entoff[0] % TBL_UALPHA_LEN];
        mixoff++; entoff++;
    }

    /* punctuation */
    memcpy(mixoff, tbl_punct, TBL_PUNCT_LEN * flags.p);

    /* double randomize dictionary */
    for(i = 0, mixoff = mixbuf; i < (mixlen * 2); i++) {
        if(i == mixlen)
            mixoff = mixbuf;

        t = mixoff[0];
        mixoff[0] = mixbuf[entoff[0] % mixlen];
        mixbuf[entoff[0] % mixlen] = t;
        mixoff++; entoff++;
    }

    /* generate and print passwords */
    passbuf = calloc(passlen + 1, 1);
    if(passbuf == NULL) {
        fprintf(stderr, "Could not allocate %zd bytes\n", passlen + 1);
        ret = EXIT_FAILURE;
        goto out;
    }

    for(i = 0; i < passcnt; entoff += passlen, i++) {
        for(j = 0; j < passlen; j++)
            passbuf[j] = mixbuf[entoff[j] % mixlen];

        printf("%s\n", passbuf);
    }

out:
    /* close /dev/urandom */
    if(fd != -1)
        close(fd);

    /* clean allocated buffers */
    if(mixbuf != NULL) {
        memzero(mixbuf, mixlen);
        free(mixbuf);
    }

    if(entbuf != NULL) {
        memzero(entbuf, entlen);
        free(entbuf);
    }

    if(passbuf != NULL) {
        memzero(passbuf, passlen + 1);
        free(passbuf);
    }

    return ret;
}
