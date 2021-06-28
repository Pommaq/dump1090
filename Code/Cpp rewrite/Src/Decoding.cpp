//
// Created by timmy on 2021-06-06.
//

#include <cstring>
#include "../Headers/Decoding.hpp"
#include "../Headers/Networking.hpp"
#include "../Headers/debugging.hpp"
#include "../Headers/Utilities.hpp"
#include "../Headers/modesMessage.h"

/* ===================== Mode S detection and decoding  ===================== */
/* Parity table for MODE S Messages.
 * The table contains 112 elements, every element corresponds to a bit set
 * in the bitf_message, starting from the first bit of actual data after the
 * preamble.
 *
 * For messages of 112 bit, the whole table is used.
 * For messages of 56 bits only the last 56 elements are used.
 *
 * The algorithm is as simple as xoring all the elements in this table
 * for which the corresponding bit on the bitf_message is set to 1.
 *
 * The latest 24 elements in this table are set to 0 as the checksum at the
 * end of the bitf_message should not affect the computation.
 *
 * Note: this function can be used with DF11 and DF17, other modes have
 * the CRC xored with the sender address as they are reply to interrogations,
 * but a casual listener can't split the address from the checksum.
 */
uint32_t modes_checksum_table[112] = {
        0x3935ea, 0x1c9af5, 0xf1b77e, 0x78dbbf, 0xc397db, 0x9e31e9, 0xb0e2f0, 0x587178,
        0x2c38bc, 0x161c5e, 0x0b0e2f, 0xfa7d13, 0x82c48d, 0xbe9842, 0x5f4c21, 0xd05c14,
        0x682e0a, 0x341705, 0xe5f186, 0x72f8c3, 0xc68665, 0x9cb936, 0x4e5c9b, 0xd8d449,
        0x939020, 0x49c810, 0x24e408, 0x127204, 0x093902, 0x049c81, 0xfdb444, 0x7eda22,
        0x3f6d11, 0xe04c8c, 0x702646, 0x381323, 0xe3f395, 0x8e03ce, 0x4701e7, 0xdc7af7,
        0x91c77f, 0xb719bb, 0xa476d9, 0xadc168, 0x56e0b4, 0x2b705a, 0x15b82d, 0xf52612,
        0x7a9309, 0xc2b380, 0x6159c0, 0x30ace0, 0x185670, 0x0c2b38, 0x06159c, 0x030ace,
        0x018567, 0xff38b7, 0x80665f, 0xbfc92b, 0xa01e91, 0xaff54c, 0x57faa6, 0x2bfd53,
        0xea04ad, 0x8af852, 0x457c29, 0xdd4410, 0x6ea208, 0x375104, 0x1ba882, 0x0dd441,
        0xf91024, 0x7c8812, 0x3e4409, 0xe0d800, 0x706c00, 0x383600, 0x1c1b00, 0x0e0d80,
        0x0706c0, 0x038360, 0x01c1b0, 0x00e0d8, 0x00706c, 0x003836, 0x001c1b, 0xfff409,
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000
};


uint32_t modesChecksum(const unsigned char *msg, int bits) {
    uint32_t crc = 0;
    int offset = (bits == 112) ? 0 : (112 - 56);
    int j;

    for (j = 0; j < bits; j++) {
        int byte = j / 8;
        int bit = j % 8;
        int bitmask = 1 << (7 - bit);

        /* If bit is set, xor with corresponding table entry. */
        if (msg[byte] & bitmask)
            crc ^= modes_checksum_table[j + offset];
    }
    return crc; /* 24 bit checksum. */
}

/* Given the Downlink Format (DF) of the bitf_message, return the bitf_message length
 * in bits. */
int modesMessageLenByType(int type) {
    if (type == 16 || type == 17 ||
        type == 19 || type == 20 ||
        type == 21)
        return MODES_LONG_MSG_BITS;
    else
        return MODES_SHORT_MSG_BITS;
}


/* Turn an hex digit into its 4 bit decimal value.
 * Returns -1 if the digit is not in the 0-F range. */
int hexDigitVal(int c) {
    c = tolower(c);
    if (c >= '0' && c <= '9') return c - '0';
    else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    else return -1;
}


/* This function decodes a string representing a Mode S bitf_message in
 * raw hex format like: *8D4B969699155600E87406F5B69F;
 * The string is supposed to be at the start of the client buffer
 * and null-terminated.
 *
 * The bitf_message is passed to the higher level layers, so it feeds
 * the selected screen bitf_output, the network bitf_output and so forth.
 *
 * If the bitf_message looks invalid is silently discarded.
 *
 * The function always returns 0 (success) to the caller as there is
 * no case where we want broken messages here to close the client
 * connection. */
int decodeHexMessage(struct client *c) {
    char *hex = c->buf;
    int l = strlen(hex), j;
    unsigned char msg[MODES_LONG_MSG_BYTES];

    /* Remove spaces on the left and on the right. */
    while (l && isspace(hex[l - 1])) {
        hex[l - 1] = '\0';
        l--;
    }
    while (isspace(*hex)) {
        hex++;
        l--;
    }

    /* Turn the bitf_message into binary. */
    if (l < 2 || hex[0] != '*' || hex[l - 1] != ';') return 0;
    hex++;
    l -= 2; /* Skip * and ; */
    if (l > MODES_LONG_MSG_BYTES * 2) return 0; /* Too long bitf_message... broken. */
    for (j = 0; j < l; j += 2) {
        int high = hexDigitVal(hex[j]);
        int low = hexDigitVal(hex[j + 1]);

        if (high == -1 || low == -1) return 0;
        msg[j / 2] = (high << 4) | low;
    }
    modesMessage mm(msg);
    mm.updatePlanes();
    return 0;
}


/* Turn I/Q samples pointed by Modes.data into the magnitude vector
 * pointed by Modes.magnitude. */
std::shared_ptr<uint16_t> computeMagnitudeVector() {
    std::shared_ptr<uint16_t> m(new uint16_t[Modes.data_len]);
    unsigned char *p = Modes.data;
    uint32_t j;
    /* Compute the magnitudo vector. It's just SQRT(I^2 + Q^2), but
     * we rescale to the 0-255 range to exploit the full resolution. */
    for (j = 0; j < Modes.data_len; j += 2) {
        int i = p[j] - 127;
        int q = p[j + 1] - 127;

        if (i < 0) i = -i;
        if (q < 0) q = -q;
        m.get()[j / 2] = Modes.maglut[i * 129 + q];
    }
    return m;
}

/* Return -1 if the bitf_message is out of fase left-side
 * Return  1 if the bitf_message is out of fase right-size
 * Return  0 if the bitf_message is not particularly out of phase.
 *
 * Note: this function will access m[-1], so the caller should make sure to
 * call it only if we are not at the start of the current buffer. */
int detectOutOfPhase(uint16_t const *m) {
    if (m[3] > m[2] / 3) return 1;
    if (m[10] > m[9] / 3) return 1;
    if (m[6] > m[7] / 3) return -1;
    if (m[-1] > m[1] / 3) return -1;
    return 0;
}

/* This function does not really correct the phase of the bitf_message, it just
 * applies a transformation to the first sample representing a given bit:
 *
 * If the previous bit was one, we amplify it a bit.
 * If the previous bit was zero, we decrease it a bit.
 *
 * This simple transformation makes the bitf_message a bit more likely to be
 * correctly decoded for out of phase messages:
 *
 * When messages are out of phase there is more uncertainty in
 * sequences of the same bit multiple times, since 11111 will be
 * transmitted as continuously altering magnitude (high, low, high, low...)
 *
 * However because the bitf_message is out of phase some part of the high
 * is mixed in the low part, so that it is hard to distinguish if it is
 * a zero or a one.
 *
 * However when the bitf_message is out of phase passing from 0 to 1 or from
 * 1 to 0 happens in a very recognizable way, for instance in the 0 -> 1
 * transition, magnitude goes low, high, high, low, and one of of the
 * two middle samples the high will be *very* high as part of the previous
 * or next high signal will be mixed there.
 *
 * Applying our simple transformation we make more likely if the current
 * bit is a zero, to detect another zero. Symmetrically if it is a one
 * it will be more likely to detect a one because of the transformation.
 * In this way similar levels will be interpreted more likely in the
 * correct way. */
void applyPhaseCorrection(uint16_t *m) {
    int j;

    m += 16; /* Skip preamble. */
    for (j = 0; j < (MODES_LONG_MSG_BITS - 1) * 2; j += 2) {
        if (m[j] > m[j + 1]) {
            /* One */
            m[j + 2] = (m[j + 2] * 5) / 4;
        } else {
            /* Zero */
            m[j + 2] = (m[j + 2] * 4) / 5;
        }
    }
}

/* Detect a Mode S messages inside the magnitude buffer pointed by 'm' and of
 * size 'mlen' bytes. Every detected Mode S bitf_message is convert it into a
 * stream of bits and passed to the function to display it. */
void detectModeS(uint16_t *m, uint32_t mlen) {
    unsigned char bits[MODES_LONG_MSG_BITS];
    unsigned char msg[MODES_LONG_MSG_BITS / 2];
    uint16_t aux[MODES_LONG_MSG_BITS * 2];
    uint32_t j;
    int use_correction = 0;

    /* The Mode S preamble is made of impulses of 0.5 microseconds at
     * the following time offsets:
     *
     * 0   - 0.5 usec: first impulse.
     * 1.0 - 1.5 usec: second impulse.
     * 3.5 - 4   usec: third impulse.
     * 4.5 - 5   usec: last impulse.
     *
     * Since we are sampling at 2 Mhz every sample in our magnitude vector
     * is 0.5 usec, so the preamble will look like this, assuming there is
     * an impulse at offset 0 in the array:
     *
     * 0   -----------------
     * 1   -
     * 2   ------------------
     * 3   --
     * 4   -
     * 5   --
     * 6   -
     * 7   ------------------
     * 8   --
     * 9   -------------------
     */
    for (j = 0; j < mlen - MODES_FULL_LEN * 2; j++) {
        int low, high, delta, i, errors;
        int good_message = 0;

        if (!use_correction) { /* We already checked it. */
            /* First check of relations between the first 10 samples
             * representing a valid preamble. We don't even investigate further
             * if this simple test is not passed. */
            if (!(m[j] > m[j + 1] &&
                  m[j + 1] < m[j + 2] &&
                  m[j + 2] > m[j + 3] &&
                  m[j + 3] < m[j] &&
                  m[j + 4] < m[j] &&
                  m[j + 5] < m[j] &&
                  m[j + 6] < m[j] &&
                  m[j + 7] > m[j + 8] &&
                  m[j + 8] < m[j + 9] &&
                  m[j + 9] > m[j + 6])) {
                if (Modes.debug & MODES_DEBUG_NOPREAMBLE &&
                    m[j] > MODES_DEBUG_NOPREAMBLE_LEVEL)
                    dumpRawMessage("Unexpected ratio among first 10 samples",
                                   msg, m, j);
                continue;
            }

            /* The samples between the two spikes must be < than the average
             * of the high spikes level. We don't test bits too near to
             * the high levels as signals can be out of phase so part of the
             * energy can be in the near samples. */
            high = (m[j] + m[j + 2] + m[j + 7] + m[j + 9]) / 6;
            if (m[j + 4] >= high ||
                m[j + 5] >= high) {
                if (Modes.debug & MODES_DEBUG_NOPREAMBLE &&
                    m[j] > MODES_DEBUG_NOPREAMBLE_LEVEL)
                    dumpRawMessage(
                            "Too high level in samples between 3 and 6",
                            msg, m, j);
                continue;
            }

            /* Similarly samples in the range 11-14 must be low, as it is the
             * space between the preamble and real data. Again we don't test
             * bits too near to high levels, see above. */
            if (m[j + 11] >= high ||
                m[j + 12] >= high ||
                m[j + 13] >= high ||
                m[j + 14] >= high) {
                if (Modes.debug & MODES_DEBUG_NOPREAMBLE &&
                    m[j] > MODES_DEBUG_NOPREAMBLE_LEVEL)
                    dumpRawMessage(
                            "Too high level in samples between 10 and 15",
                            msg, m, j);
                continue;
            }
            Modes.stat_valid_preamble++;
        }
        /* If the previous attempt with this bitf_message failed, retry using
         * magnitude correction. */
        if (use_correction) {
            memcpy(aux, m + j + MODES_PREAMBLE_US * 2, sizeof(aux));
            if (j && detectOutOfPhase(m + j)) {
                applyPhaseCorrection(m + j);
                Modes.stat_out_of_phase++;
            }
            /* TODO ... apply other kind of corrections. */
        }

        /* Decode all the next 112 bits, regardless of the actual bitf_message
         * size. We'll check the actual bitf_message type later. */
        errors = 0;
        for (i = 0; i < MODES_LONG_MSG_BITS * 2; i += 2) {
            low = m[j + i + MODES_PREAMBLE_US * 2];
            high = m[j + i + MODES_PREAMBLE_US * 2 + 1];
            delta = low - high;
            if (delta < 0) delta = -delta;

            if (i > 0 && delta < 256) {
                bits[i / 2] = bits[i / 2 - 1];
            } else if (low == high) {
                /* Checking if two adiacent samples have the same magnitude
                 * is an effective way to detect if it's just random noise
                 * that was detected as a valid preamble. */
                bits[i / 2] = 2; /* error */
                if (i < MODES_SHORT_MSG_BITS * 2) {
                    errors++;
                };
            } else if (low > high) {
                bits[i / 2] = 1;
            } else {
                /* (low < high) for exclusion  */
                bits[i / 2] = 0;
            }
        }

        /* Restore the original bitf_message if we used magnitude correction. */
        if (use_correction)
            memcpy(m + j + MODES_PREAMBLE_US * 2, aux, sizeof(aux));

        /* Pack bits into bytes */
        for (i = 0; i < MODES_LONG_MSG_BITS; i += 8) {
            msg[i / 8] =
                    bits[i] << 7 |
                    bits[i + 1] << 6 |
                    bits[i + 2] << 5 |
                    bits[i + 3] << 4 |
                    bits[i + 4] << 3 |
                    bits[i + 5] << 2 |
                    bits[i + 6] << 1 |
                    bits[i + 7];
        }

        int msgtype = msg[0] >> 3;
        int msglen = modesMessageLenByType(msgtype) / 8;

        /* Last check, high and low bits are different enough in magnitude
         * to mark this as real bitf_message and not just noise? */
        delta = 0;
        for (i = 0; i < msglen * 8 * 2; i += 2) {
            delta += abs(m[j + i + MODES_PREAMBLE_US * 2] -
                         m[j + i + MODES_PREAMBLE_US * 2 + 1]);
        }
        delta /= msglen * 4;

        /* Filter for an average delta of three is small enough to let almost
         * every kind of bitf_message to pass, but high enough to filter some
         * random noise. */
        if (delta < 10 * 255) {
            use_correction = 0;
            continue;
        }

        /* If we reached this point, and error is zero, we are very likely
         * with a Mode S bitf_message in our hands, but it may still be broken
         * and CRC may not be correct. This is handled by the next layer. */
        if (errors == 0 || (Modes.aggressive && errors < 3)) {

            /* Decode the received bitf_message and update statistics */
            modesMessage mm(msg);
            // TODO: Move this statistics part to somewhere else so we can do errorfixes on a separate thread.

            /* Update statistics. */
            if (mm.crcok || use_correction) {
                if (errors == 0) Modes.stat_demodulated++;
                if (mm.errorbit == -1) {
                    if (mm.crcok)
                        Modes.stat_goodcrc++;
                    else
                        Modes.stat_badcrc++;
                } else {
                    Modes.stat_badcrc++;
                    Modes.stat_fixed++;
                    if (mm.errorbit < MODES_LONG_MSG_BITS)
                        Modes.stat_single_bit_fix++;
                    else
                        Modes.stat_two_bits_fix++;
                }
            }

            /* Output debug mode info if needed. */
            if (use_correction == 0) {
                if (Modes.debug & MODES_DEBUG_DEMOD)
                    dumpRawMessage("Demodulated with 0 errors", msg, m, j);
                else if (Modes.debug & MODES_DEBUG_BADCRC &&
                         mm.msgtype == 17 &&
                         (!mm.crcok || mm.errorbit != -1))
                    dumpRawMessage("Decoded with bad CRC", msg, m, j);
                else if (Modes.debug & MODES_DEBUG_GOODCRC && mm.crcok &&
                         mm.errorbit == -1)
                    dumpRawMessage("Decoded with good CRC", msg, m, j);
            }

            /* Skip this bitf_message if we are sure it's fine. */
            if (mm.crcok) {
                j += (MODES_PREAMBLE_US + (msglen * 8)) * 2;
                good_message = 1;
                if (use_correction)
                    mm.phase_corrected = 1;
            }

            /* Pass data to the next layer */
            mm.updatePlanes();
        } else {
            if (Modes.debug & MODES_DEBUG_DEMODERR && use_correction) {
                printf("The following bitf_message has %d demod errors\n", errors);
                dumpRawMessage("Demodulated with errors", msg, m, j);
            }
        }

        /* Retry with phase correction if possible. */
        if (!good_message && !use_correction) {
            j--;
            use_correction = 1;
        } else {
            use_correction = 0;
        }
    }
}


/* Decode the 13 bit AC altitude field (in DF 20 and others).
 * Returns the altitude, and set 'unit' to either MODES_UNIT_METERS
 * or MDOES_UNIT_FEETS. */
int decodeAC13Field(unsigned char *msg, int *unit) {
    int m_bit = msg[3] & (1 << 6);
    int q_bit = msg[3] & (1 << 4);

    if (!m_bit) {
        *unit = MODES_UNIT_FEET;
        if (q_bit) {
            /* N is the 11 bit integer resulting from the removal of bit
             * Q and M */
            int n = ((msg[2] & 31) << 6) |
                    ((msg[3] & 0x80) >> 2) |
                    ((msg[3] & 0x20) >> 1) |
                    (msg[3] & 15);
            /* The final altitude is due to the resulting number multiplied
             * by 25, minus 1000. */
            return n * 25 - 1000;
        } else {
            /* TODO: Implement altitude where Q=0 and M=0 */
        }
    } else {
        *unit = MODES_UNIT_METERS;
        /* TODO: Implement altitude when meter unit is selected. */
    }
    return 0;
}

/* Decode the 12 bit AC altitude field (in DF 17 and others).
 * Returns the altitude or 0 if it can't be decoded. */
int decodeAC12Field(unsigned char *msg, int *unit) {
    int q_bit = msg[5] & 1;

    if (q_bit) {
        /* N is the 11 bit integer resulting from the removal of bit
         * Q */
        *unit = MODES_UNIT_FEET;
        int n = ((msg[5] >> 1) << 4) | ((msg[6] & 0xF0) >> 4);
        /* The final altitude is due to the resulting number multiplied
         * by 25, minus 1000. */
        return n * 25 - 1000;
    } else {
        return 0;
    }
}

/* Try to fix single bit errors using the checksum. On success modifies
 * the original buffer with the fixed version, and returns the position
 * of the error bit. Otherwise if fixing failed -1 is returned. */
int fixSingleBitErrors(unsigned char *msg, int bits) {
    int j;
    unsigned char aux[MODES_LONG_MSG_BITS / 8];

    for (j = 0; j < bits; j++) { // For each bit
        int byte = j / 8;
        int bitmask = 1 << (7 - (j % 8)); // Grab the bit
        uint32_t crc1, crc2;

        memcpy(aux, msg, bits / 8);
        aux[byte] ^= bitmask; /* Flip j-th bit. */

        crc1 = ((uint32_t) aux[(bits / 8) - 3] << 16) |
               ((uint32_t) aux[(bits / 8) - 2] << 8) |
               (uint32_t) aux[(bits / 8) - 1];
        crc2 = modesChecksum(aux, bits);

        if (crc1 == crc2) {
            /* The error is fixed. Overwrite the original buffer with
             * the corrected sequence, and returns the error bit
             * position. */
            memcpy(msg, aux, bits / 8);
            return j;
        }
    }
    return -1;
}


/* Similar to fixSingleBitErrors() but try every possible two bit combination.
 * This is very slow and should be tried only against DF17 messages that
 * don't pass the checksum, and only in Aggressive Mode.
 * fixSingleBitErrors has already been called if this is run. Don't bother just flipping 1 bit.
 * We'd rather run this once than run the first and then this, since they both do the work
 * of flipping the first bits. Lets not waste clock cycles and work.
 * */
int fixTwoBitsErrors(unsigned char *msg, int bits) {
    int j, i;
    unsigned char aux[MODES_LONG_MSG_BITS / 8];
    uint32_t expected_crc, crc2;



    for (j = 0; j < bits; j++) {
        int byte1 = j / 8;
        int bitmask1 = 1 << (7 - (j % 8));
        memcpy(aux, msg, bits / 8); // Grab a buffer
        aux[byte1] ^= bitmask1; /* Flip j-th bit. */

        expected_crc = ((uint32_t) aux[(bits / 8) - 3] << 16) |
                       ((uint32_t) aux[(bits / 8) - 2] << 8) |
                       (uint32_t) aux[(bits / 8) - 1];

        crc2 = modesChecksum(aux, bits);
        if (expected_crc == crc2) [[unlikely]] { // Its fixed
            memcpy(msg, aux, bits / 8);
            return j;
        }

        /* Don't check the same pairs multiple times, so i starts from j+1 */
        for (i = j + 1; i < bits; i++) {
            int byte2 = i / 8;
            int bitmask2 = 1 << (7 - (i % 8));

            aux[byte2] ^= bitmask2; /* Flip i-th bit. */
            expected_crc = ((uint32_t) aux[(bits / 8) - 3] << 16) |
                           ((uint32_t) aux[(bits / 8) - 2] << 8) |
                           (uint32_t) aux[(bits / 8) - 1];


            crc2 = modesChecksum(aux, bits);

            if (expected_crc == crc2) {
                /* The error is fixed. Overwrite the original buffer with
                 * the corrected sequence, and returns the error bit
                 * position. */
                memcpy(msg, aux, bits / 8);
                /* We return the two bits as a 16 bit integer by shifting
                 * 'i' on the left. This is possible since 'i' will always
                 * be non-zero because i starts from j+1. */
                return j | (i << 8);
            }
        }
    }
    return -1;
}




