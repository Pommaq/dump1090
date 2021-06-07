//
// Created by timmy on 2021-06-06.
//

#include <cstring>
#include "../Headers/Decoding.hpp"
#include "../Headers/data_reader.hpp"
#include "../Headers/Networking.hpp"
#include "../Headers/aircraft.hpp"
#include "../Headers/Terminal.hpp"
#include "../Headers/debugging.hpp"

/* ===================== Mode S detection and decoding  ===================== */

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


uint32_t modesChecksum(unsigned char *msg, int bits) {
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


/* Always positive MOD operation, used for CPR decoding. */
int cprModFunction(int a, int b) {
    int res = a % b;
    if (res < 0) res += b;
    return res;
}

/* The NL function uses the precomputed table from 1090-WP-9-14 */
int cprNLFunction(double lat) {
    if (lat < 0) lat = -lat; /* Table is simmetric about the equator. */
    if (lat < 10.47047130) return 59;
    if (lat < 14.82817437) return 58;
    if (lat < 18.18626357) return 57;
    if (lat < 21.02939493) return 56;
    if (lat < 23.54504487) return 55;
    if (lat < 25.82924707) return 54;
    if (lat < 27.93898710) return 53;
    if (lat < 29.91135686) return 52;
    if (lat < 31.77209708) return 51;
    if (lat < 33.53993436) return 50;
    if (lat < 35.22899598) return 49;
    if (lat < 36.85025108) return 48;
    if (lat < 38.41241892) return 47;
    if (lat < 39.92256684) return 46;
    if (lat < 41.38651832) return 45;
    if (lat < 42.80914012) return 44;
    if (lat < 44.19454951) return 43;
    if (lat < 45.54626723) return 42;
    if (lat < 46.86733252) return 41;
    if (lat < 48.16039128) return 40;
    if (lat < 49.42776439) return 39;
    if (lat < 50.67150166) return 38;
    if (lat < 51.89342469) return 37;
    if (lat < 53.09516153) return 36;
    if (lat < 54.27817472) return 35;
    if (lat < 55.44378444) return 34;
    if (lat < 56.59318756) return 33;
    if (lat < 57.72747354) return 32;
    if (lat < 58.84763776) return 31;
    if (lat < 59.95459277) return 30;
    if (lat < 61.04917774) return 29;
    if (lat < 62.13216659) return 28;
    if (lat < 63.20427479) return 27;
    if (lat < 64.26616523) return 26;
    if (lat < 65.31845310) return 25;
    if (lat < 66.36171008) return 24;
    if (lat < 67.39646774) return 23;
    if (lat < 68.42322022) return 22;
    if (lat < 69.44242631) return 21;
    if (lat < 70.45451075) return 20;
    if (lat < 71.45986473) return 19;
    if (lat < 72.45884545) return 18;
    if (lat < 73.45177442) return 17;
    if (lat < 74.43893416) return 16;
    if (lat < 75.42056257) return 15;
    if (lat < 76.39684391) return 14;
    if (lat < 77.36789461) return 13;
    if (lat < 78.33374083) return 12;
    if (lat < 79.29428225) return 11;
    if (lat < 80.24923213) return 10;
    if (lat < 81.19801349) return 9;
    if (lat < 82.13956981) return 8;
    if (lat < 83.07199445) return 7;
    if (lat < 83.99173563) return 6;
    if (lat < 84.89166191) return 5;
    if (lat < 85.75541621) return 4;
    if (lat < 86.53536998) return 3;
    if (lat < 87.00000000) return 2;
    else return 1;
}

int cprNFunction(double lat, int isodd) {
    int nl = cprNLFunction(lat) - isodd;
    if (nl < 1) nl = 1;
    return nl;
}

double cprDlonFunction(double lat, int isodd) {
    return 360.0 / cprNFunction(lat, isodd);
}

/* This algorithm comes from:
 * http://www.lll.lu/~edward/edward/adsb/DecodingADSBposition.html.
 *
 *
 * A few remarks:
 * 1) 131072 is 2^17 since CPR latitude and longitude are encoded in 17 bits.
 * 2) We assume that we always received the odd packet as last packet for
 *    simplicity. This may provide a position that is less fresh of a few
 *    seconds.
 */
void decodeCPR(aircraft *a) {
    const double AirDlat0 = 360.0 / 60;
    const double AirDlat1 = 360.0 / 59;
    double lat0 = a->even_cprlat;
    double lat1 = a->odd_cprlat;
    double lon0 = a->even_cprlon;
    double lon1 = a->odd_cprlon;

    /* Compute the Latitude Index "j" */
    int j = floor(((59 * lat0 - 60 * lat1) / 131072) + 0.5);
    double rlat0 = AirDlat0 * (cprModFunction(j, 60) + lat0 / 131072);
    double rlat1 = AirDlat1 * (cprModFunction(j, 59) + lat1 / 131072);

    if (rlat0 >= 270) rlat0 -= 360;
    if (rlat1 >= 270) rlat1 -= 360;

    /* Check that both are in the same latitude zone, or abort. */
    if (cprNLFunction(rlat0) != cprNLFunction(rlat1)) return;

    /* Compute ni and the longitude index m */
    if (a->even_cprtime > a->odd_cprtime) {
        /* Use even packet. */
        int ni = cprNFunction(rlat0, 0);
        int m = floor((((lon0 * (cprNLFunction(rlat0) - 1)) -
                        (lon1 * cprNLFunction(rlat0))) / 131072) + 0.5);
        a->lon = cprDlonFunction(rlat0, 0) * (cprModFunction(m, ni) + lon0 / 131072);
        a->lat = rlat0;
    } else {
        /* Use odd packet. */
        int ni = cprNFunction(rlat1, 1);
        int m = floor((((lon0 * (cprNLFunction(rlat1) - 1)) -
                        (lon1 * cprNLFunction(rlat1))) / 131072.0) + 0.5);
        a->lon = cprDlonFunction(rlat1, 1) * (cprModFunction(m, ni) + lon1 / 131072);
        a->lat = rlat1;
    }
    if (a->lon > 180) a->lon -= 360;
}


/* Turn I/Q samples pointed by Modes.data into the magnitude vector
 * pointed by Modes.magnitude. */
void computeMagnitudeVector() {
    uint16_t *m = Modes.magnitude;
    unsigned char *p = Modes.data;
    uint32_t j;

    /* Compute the magnitudo vector. It's just SQRT(I^2 + Q^2), but
     * we rescale to the 0-255 range to exploit the full resolution. */
    for (j = 0; j < Modes.data_len; j += 2) {
        int i = p[j] - 127;
        int q = p[j + 1] - 127;

        if (i < 0) i = -i;
        if (q < 0) q = -q;
        m[j / 2] = Modes.maglut[i * 129 + q];
    }
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
                if (i < MODES_SHORT_MSG_BITS * 2) errors++;
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
 * */
int fixTwoBitsErrors(unsigned char *msg, int bits) {
    int j, i;
    unsigned char aux[MODES_LONG_MSG_BITS / 8];

    for (j = 0; j < bits; j++) {
        int byte1 = j / 8;
        int bitmask1 = 1 << (7 - (j % 8));

        /* Don't check the same pairs multiple times, so i starts from j+1 */
        for (i = j + 1; i < bits; i++) {
            int byte2 = i / 8;
            int bitmask2 = 1 << (7 - (i % 8));
            uint32_t crc1, crc2;

            memcpy(aux, msg, bits / 8);

            aux[byte1] ^= bitmask1; /* Flip j-th bit. */
            aux[byte2] ^= bitmask2; /* Flip i-th bit. */

            crc1 = ((uint32_t) aux[(bits / 8) - 3] << 16) |
                   ((uint32_t) aux[(bits / 8) - 2] << 8) |
                   (uint32_t) aux[(bits / 8) - 1];
            crc2 = modesChecksum(aux, bits);

            if (crc1 == crc2) {
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

/* Hash the ICAO address to index our cache of MODES_ICAO_CACHE_LEN
 * elements, that is assumed to be a power of two. */
uint32_t ICAOCacheHashAddress(uint32_t a) {
    /* The following three rounds wil make sure that every bit affects
     * every bitf_output bit with ~ 50% of probability. */
    a = ((a >> 16) ^ a) * 0x45d9f3b;
    a = ((a >> 16) ^ a) * 0x45d9f3b;
    a = ((a >> 16) ^ a);
    return a & (MODES_ICAO_CACHE_LEN - 1);
}

/* Add the specified entry to the cache of recently seen ICAO addresses.
 * Note that we also add a timestamp so that we can make sure that the
 * entry is only valid for MODES_ICAO_CACHE_TTL seconds. */
void addRecentlySeenICAOAddr(uint32_t addr) {
    uint32_t h = ICAOCacheHashAddress(addr);
    Modes.icao_cache[h * 2] = addr;
    Modes.icao_cache[h * 2 + 1] = (uint32_t) time(NULL);
}

/* Returns 1 if the specified ICAO address was seen in a DF format with
 * proper checksum (not xored with address) no more than * MODES_ICAO_CACHE_TTL
 * seconds ago. Otherwise returns 0. */
int ICAOAddressWasRecentlySeen(uint32_t addr) {
    uint32_t h = ICAOCacheHashAddress(addr);
    uint32_t a = Modes.icao_cache[h * 2];
    uint32_t t = Modes.icao_cache[h * 2 + 1];

    return a && a == addr && time(NULL) - t <= MODES_ICAO_CACHE_TTL;
}

/* If the bitf_message type has the checksum xored with the ICAO address, try to
 * brute force it using a list of recently seen ICAO addresses.
 *
 * Do this in a brute-force fashion by xoring the predicted CRC with
 * the address XOR checksum field in the bitf_message. This will recover the
 * address: if we found it in our cache, we can assume the bitf_message is ok.
 *
 * This function expects mm->msgtype and mm->msgbits to be correctly
 * populated by the caller.
 *
 * On success the correct ICAO address is stored in the modesMessage
 * structure in the aa3, aa2, and aa1 fiedls.
 *
 * If the function successfully recovers a bitf_message with a correct checksum
 * it returns 1. Otherwise 0 is returned. */
int bruteForceAP(unsigned char *msg, struct modesMessage *mm) {
    unsigned char aux[MODES_LONG_MSG_BYTES];
    int msgtype = mm->msgtype;
    int msgbits = mm->msgbits;

    if (msgtype == 0 ||         /* Short air surveillance */
        msgtype == 4 ||         /* Surveillance, altitude reply */
        msgtype == 5 ||         /* Surveillance, identity reply */
        msgtype == 16 ||        /* Long Air-Air survillance */
        msgtype == 20 ||        /* Comm-A, altitude request */
        msgtype == 21 ||        /* Comm-A, identity request */
        msgtype == 24)          /* Comm-C ELM */
    {
        uint32_t addr;
        uint32_t crc;
        int lastbyte = (msgbits / 8) - 1;

        /* Work on a copy. */
        memcpy(aux, msg, msgbits / 8);

        /* Compute the CRC of the bitf_message and XOR it with the AP field
         * so that we recover the address, because:
         *
         * (ADDR xor CRC) xor CRC = ADDR. */
        crc = modesChecksum(aux, msgbits);
        aux[lastbyte] ^= crc & 0xff;
        aux[lastbyte - 1] ^= (crc >> 8) & 0xff;
        aux[lastbyte - 2] ^= (crc >> 16) & 0xff;

        /* If the obtained address exists in our cache we consider
         * the bitf_message valid. */
        addr = aux[lastbyte] | (aux[lastbyte - 1] << 8) | (aux[lastbyte - 2] << 16);
        if (ICAOAddressWasRecentlySeen(addr)) {
            mm->aa1 = aux[lastbyte - 2];
            mm->aa2 = aux[lastbyte - 1];
            mm->aa3 = aux[lastbyte];
            return 1;
        }
    }
    return 0;
}

/* Decode a raw Mode S bitf_message demodulated as a stream of bytes by
 * detectModeS(), and split it into fields populating a modesMessage
 * structure. */
modesMessage::modesMessage(unsigned char *msg) {

    uint32_t crc2;   /* Computed CRC, used to verify the bitf_message CRC. */
    const char *ais_charset = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ????? ???????????????0123456789??????";

    /* Work on our local copy */
    memcpy(this->msg, msg, MODES_LONG_MSG_BYTES);
    msg = this->msg;

    /* Get the bitf_message type ASAP as other operations depend on this */
    this->msgtype = msg[0] >> 3;    /* Downlink Format */
    this->msgbits = modesMessageLenByType(this->msgtype);

    /* CRC is always the last three bytes. */
    this->crc = ((uint32_t) msg[(this->msgbits / 8) - 3] << 16) |
                ((uint32_t) msg[(this->msgbits / 8) - 2] << 8) |
                (uint32_t) msg[(this->msgbits / 8) - 1];
    crc2 = modesChecksum(msg, this->msgbits);

    /* Check CRC and fix single bit errors using the CRC when
     * possible (DF 11 and 17). */
    this->errorbit = -1;  /* No error */
    this->crcok = (this->crc == crc2);

    if (!this->crcok && Modes.fix_errors &&
        (this->msgtype == 11 || this->msgtype == 17)) {
        if ((this->errorbit = fixSingleBitErrors(msg, this->msgbits)) != -1) {
            if (this->errorbit == -2) {
                exit(EXIT_FAILURE);
            }
            this->crc = modesChecksum(msg, this->msgbits);
            this->crcok = 1;
        } else if (Modes.aggressive && this->msgtype == 17 &&
                   (this->errorbit = fixTwoBitsErrors(msg, this->msgbits)) != -1) {
            this->crc = modesChecksum(msg, this->msgbits);
            this->crcok = 1;
        }
    }

    /* Note that most of the other computation happens *after* we fix
     * the single bit errors, otherwise we would need to recompute the
     * fields again. */
    this->ca = msg[0] & 7;        /* Responder capabilities. */

    /* ICAO address */
    this->aa1 = msg[1];
    this->aa2 = msg[2];
    this->aa3 = msg[3];

    /* DF 17 type (assuming this is a DF17, otherwise not used) */
    this->metype = msg[4] >> 3;   /* Extended squitter bitf_message type. */
    this->mesub = msg[4] & 7;     /* Extended squitter bitf_message subtype. */

    /* Fields for DF4,5,20,21 */
    this->fs = msg[0] & 7;        /* Flight status for DF4,5,20,21 */
    this->dr = msg[1] >> 3 & 31;  /* Request extraction of downlink request. */
    this->um = ((msg[1] & 7) << 3) | /* Request extraction of downlink request. */
               msg[2] >> 5;

    /* In the squawk (identity) field bits are interleaved like that
     * (bitf_message bit 20 to bit 32):
     *
     * C1-A1-C2-A2-C4-A4-ZERO-B1-D1-B2-D2-B4-D4
     *
     * So every group of three bits A, B, C, D represent an integer
     * from 0 to 7.
     *
     * The actual meaning is just 4 octal numbers, but we convert it
     * into a base ten number tha happens to represent the four
     * octal numbers.
     *
     * For more info: http://en.wikipedia.org/wiki/Gillham_code */
    {
        int a, b, c, d;

        a = ((msg[3] & 0x80) >> 5) |
            ((msg[2] & 0x02) >> 0) |
            ((msg[2] & 0x08) >> 3);
        b = ((msg[3] & 0x02) << 1) |
            ((msg[3] & 0x08) >> 2) |
            ((msg[3] & 0x20) >> 5);
        c = ((msg[2] & 0x01) << 2) |
            ((msg[2] & 0x04) >> 1) |
            ((msg[2] & 0x10) >> 4);
        d = ((msg[3] & 0x01) << 2) |
            ((msg[3] & 0x04) >> 1) |
            ((msg[3] & 0x10) >> 4);
        this->identity = a * 1000 + b * 100 + c * 10 + d;
    }

    /* DF 11 & 17: try to populate our ICAO addresses whitelist.
     * DFs with an AP field (xored addr and crc), try to decode it. */
    if (this->msgtype != 11 && this->msgtype != 17) {
        /* Check if we can check the checksum for the Downlink Formats where
         * the checksum is xored with the aircraft ICAO address. We try to
         * brute force it using a list of recently seen aircraft addresses. */
        if (bruteForceAP(msg, this)) {
            /* We recovered the bitf_message, mark the checksum as valid. */
            this->crcok = 1;
        } else {
            this->crcok = 0;
        }
    } else {
        /* If this is DF 11 or DF 17 and the checksum was ok,
         * we can add this address to the list of recently seen
         * addresses. */
        if (this->crcok && this->errorbit == -1) {
            uint32_t addr = (this->aa1 << 16) | (this->aa2 << 8) | this->aa3;
            addRecentlySeenICAOAddr(addr);
        }
    }

    /* Decode 13 bit altitude for DF0, DF4, DF16, DF20 */
    if (this->msgtype == 0 || this->msgtype == 4 ||
        this->msgtype == 16 || this->msgtype == 20) {
        this->altitude = decodeAC13Field(msg, &this->unit);
    }

    /* Decode extended squitter specific stuff. */
    if (this->msgtype == 17) {
        /* Decode the extended squitter bitf_message. */

        if (this->metype >= 1 && this->metype <= 4) {
            /* Aircraft Identification and Category */
            this->aircraft_type = this->metype - 1;
            this->flight[0] = ais_charset[msg[5] >> 2];
            this->flight[1] = ais_charset[((msg[5] & 3) << 4) | (msg[6] >> 4)];
            this->flight[2] = ais_charset[((msg[6] & 15) << 2) | (msg[7] >> 6)];
            this->flight[3] = ais_charset[msg[7] & 63];
            this->flight[4] = ais_charset[msg[8] >> 2];
            this->flight[5] = ais_charset[((msg[8] & 3) << 4) | (msg[9] >> 4)];
            this->flight[6] = ais_charset[((msg[9] & 15) << 2) | (msg[10] >> 6)];
            this->flight[7] = ais_charset[msg[10] & 63];
            this->flight[8] = '\0';
        } else if (this->metype >= 9 && this->metype <= 18) {
            /* Airborne position Message */
            this->fflag = msg[6] & (1 << 2);
            this->tflag = msg[6] & (1 << 3);
            this->altitude = decodeAC12Field(msg, &this->unit);
            this->raw_latitude = ((msg[6] & 3) << 15) |
                                 (msg[7] << 7) |
                                 (msg[8] >> 1);
            this->raw_longitude = ((msg[8] & 1) << 16) |
                                  (msg[9] << 8) |
                                  msg[10];
        } else if (this->metype == 19 && this->mesub >= 1 && this->mesub <= 4) {
            /* Airborne Velocity Message */
            if (this->mesub == 1 || this->mesub == 2) {
                this->ew_dir = (msg[5] & 4) >> 2;
                this->ew_velocity = ((msg[5] & 3) << 8) | msg[6];
                this->ns_dir = (msg[7] & 0x80) >> 7;
                this->ns_velocity = ((msg[7] & 0x7f) << 3) | ((msg[8] & 0xe0) >> 5);
                this->vert_rate_source = (msg[8] & 0x10) >> 4;
                this->vert_rate_sign = (msg[8] & 0x8) >> 3;
                this->vert_rate = ((msg[8] & 7) << 6) | ((msg[9] & 0xfc) >> 2);
                /* Compute velocity and angle from the two speed
                 * components. */
                this->velocity = sqrt(this->ns_velocity * this->ns_velocity +
                                      this->ew_velocity * this->ew_velocity);
                if (this->velocity) {
                    int ewv = this->ew_velocity;
                    int nsv = this->ns_velocity;
                    double direction;

                    if (this->ew_dir) ewv *= -1;
                    if (this->ns_dir) nsv *= -1;
                    direction = atan2(ewv, nsv);

                    /* Convert to degrees. */
                    this->heading = direction * 360 / (M_PI * 2);
                    /* We don't want negative values but a 0-360 scale. */
                    if (this->heading < 0) this->heading += 360;
                } else {
                    this->heading = 0;
                }
            } else if (this->mesub == 3 || this->mesub == 4) {
                this->heading_is_valid = msg[5] & (1 << 2);
                this->heading = (360.0 / 128) * (((msg[5] & 3) << 5) |
                                                 (msg[6] >> 3));
            }
        }
    }
    this->phase_corrected = 0; /* Set to 1 by the caller if needed. */
}

/* When a new bitf_message is available, because it was decoded from the
 * RTL device, file, or received in the TCP input port, or any other
 * way we can receive a decoded bitf_message, we call this function in order
 * to use the bitf_message.
 *
 * Basically this function passes a raw bitf_message to the upper layers for
 * further processing and visualization. */
void modesMessage::updatePlanes() {
    if (!Modes.stats && (Modes.check_crc == 0 || this->crcok)) {
        /* Track aircrafts in interactive mode or if the HTTP
         * interface is enabled. */
        if (Modes.interactive || Modes.stat_http_requests > 0 || Modes.stat_sbs_connections > 0) {
            struct aircraft *a = interactiveReceiveData(this);
            if (a && Modes.stat_sbs_connections > 0) modesSendSBSOutput(this, a);  /* Feed SBS bitf_output clients. */
        }
        /* In non-interactive way, display messages on standard bitf_output. */
        if (!Modes.interactive) {
            displayModesMessage(this);
            if (!Modes.raw && !Modes.onlyaddr) printf("\n");
        }
        /* Send data to connected clients. */
        if (Modes.net) {
            modesSendRawOutput(this);  /* Feed raw bitf_output clients. */
        }
    }
}
