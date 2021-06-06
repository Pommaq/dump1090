//
// Created by timmy on 2021-06-06.
//

#include "../Headers/debugging.h"

/* ============================== Debugging ================================= */

/* Helper function for dumpMagnitudeVector().
 * It prints a single bar used to display raw signals.
 *
 * Since every magnitude sample is between 0-255, the function uses
 * up to 63 characters for every bar. Every character represents
 * a length of 4, 3, 2, 1, specifically:
 *
 * "O" is 4
 * "o" is 3
 * "-" is 2
 * "." is 1
 */
void dumpMagnitudeBar(int index, int magnitude) {
    char *set = " .-o";
    char buf[256];
    int div = magnitude / 256 / 4;
    int rem = magnitude / 256 % 4;

    memset(buf, 'O', div);
    buf[div] = set[rem];
    buf[div + 1] = '\0';

    if (index >= 0) {
        int markchar = ']';

        /* preamble peaks are marked with ">" */
        if (index == 0 || index == 2 || index == 7 || index == 9)
            markchar = '>';
        /* Data peaks are marked to distinguish pairs of bits. */
        if (index >= 16) markchar = ((index - 16) / 2 & 1) ? '|' : ')';
        printf("[%.3d%c |%-66s %d\n", index, markchar, buf, magnitude);
    } else {
        printf("[%.2d] |%-66s %d\n", index, buf, magnitude);
    }
}

/* Display an ASCII-art alike graphical representation of the undecoded
 * bitf_message as a magnitude signal.
 *
 * The bitf_message starts at the specified offset in the "m" buffer.
 * The function will display enough data to cover a short 56 bit bitf_message.
 *
 * If possible a few samples before the start of the messsage are included
 * for context. */

void dumpMagnitudeVector(uint16_t *m, uint32_t offset) {
    uint32_t padding = 5; /* Show a few samples before the actual start. */
    uint32_t start = (offset < padding) ? 0 : offset - padding;
    uint32_t end = offset + (MODES_PREAMBLE_US * 2) + (MODES_SHORT_MSG_BITS * 2) - 1;
    uint32_t j;

    for (j = start; j <= end; j++) {
        dumpMagnitudeBar(j - offset, m[j]);
    }
}

/* Produce a raw representation of the bitf_message as a Javascript file
 * loadable by debug.html. */
void dumpRawMessageJS(char *descr, unsigned char *msg,
                      uint16_t *m, uint32_t offset, int fixable) {
    int padding = 5; /* Show a few samples before the actual start. */
    int start = offset - padding;
    int end = offset + (MODES_PREAMBLE_US * 2) + (MODES_LONG_MSG_BITS * 2) - 1;
    FILE *fp;
    int j, fix1 = -1, fix2 = -1;

    if (fixable != -1) {
        fix1 = fixable & 0xff;
        if (fixable > 255) fix2 = fixable >> 8;
    }

    if ((fp = fopen("frames.js", "a")) == NULL) {
        fprintf(stderr, "Error opening frames.js: %s\n", strerror(errno));
        exit(1);
    }

    fprintf(fp, "frames.push({\"descr\": \"%s\", \"mag\": [", descr);
    for (j = start; j <= end; j++) {
        fprintf(fp, "%d", j < 0 ? 0 : m[j]);
        if (j != end) fprintf(fp, ",");
    }
    fprintf(fp, "], \"fix1\": %d, \"fix2\": %d, \"bits\": %d, \"hex\": \"",
            fix1, fix2, modesMessageLenByType(msg[0] >> 3));
    for (j = 0; j < MODES_LONG_MSG_BYTES; j++)
        fprintf(fp, "\\x%02x", msg[j]);
    fprintf(fp, "\"});\n");
    fclose(fp);
}

/* This is a wrapper for dumpMagnitudeVector() that also show the bitf_message
 * in hex format with an additional description.
 *
 * descr  is the additional bitf_message to show to describe the dump.
 * msg    points to the decoded bitf_message
 * m      is the original magnitude vector
 * offset is the offset where the bitf_message starts
 *
 * The function also produces the Javascript file used by debug.html to
 * display packets in a graphical format if the Javascript bitf_output was
 * enabled.
 */
void dumpRawMessage(char *descr, unsigned char *msg,
                    uint16_t *m, uint32_t offset) {

    int j;
    int msgtype = msg[0] >> 3;
    int fixable = -1;

    if (msgtype == 11 || msgtype == 17) {
        int msgbits = (msgtype == 11) ? MODES_SHORT_MSG_BITS :
                      MODES_LONG_MSG_BITS;
        fixable = fixSingleBitErrors(msg, msgbits);
        if (fixable == -1)
            fixable = fixTwoBitsErrors(msg, msgbits);
    }

    if (Modes.debug & MODES_DEBUG_JS) {
        dumpRawMessageJS(descr, msg, m, offset, fixable);
        return;
    }

    printf("\n--- %s\n    ", descr);
    for (j = 0; j < MODES_LONG_MSG_BYTES; j++) {
        printf("%02x", msg[j]);
        if (j == MODES_SHORT_MSG_BYTES - 1) printf(" ... ");
    }
    printf(" (DF %d, Fixable: %d)\n", msgtype, fixable);
    dumpMagnitudeVector(m, offset);
    printf("---\n\n");
}


