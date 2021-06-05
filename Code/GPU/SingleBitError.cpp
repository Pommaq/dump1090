//
// Created by timmy on 2021-02-23.
//

__constant bitmasks[] = {0b10000000, 0b1000000, 0b100000, 0b10000,
                         0b1000,     0b100,     0b10,     0b1};

__constant int MODES_LONG_MSG_BITS = 112;

/* Try to fix single bit errors using the checksum. On success modifies
 * the original buffer with the fixed version, and saves the position
 * of the error bit. Otherwise if fixing failed nothing is saved leaving
 * returnval as-is. */


__kernel void fixSingleBitErrors(__global unsigned char *msg,
                                 __global int *bits, __global int *returnval) {
  size_t j = get_global_id(0);
  unsigned char
      aux[MODES_LONG_MSG_BITS / 8]; // NOTE: Might have to allocate in main
                                    // memory and give to pointer.
  size_t byte = j / 8;
  int bitmask = 1 << (7 - (j % 8));
  int crc1, crc2;

  for (int i = 0; i < (*bits) / 8; i += 2) {
    aux[i] = msg[i];
    aux[i + 1] = msg[i + 1];
  }

  aux[byte] ^= bitmask; /* Flip the J-th bit*/

  crc1 = ((uint)aux[(*bits / 8) - 3] << 16) |
         ((uint)aux[(*bits / 8) - 2] << 8) | (uint)aux[(*bits / 8) - 1];
  /*
  crc2 = modesChecksum(aux,*bits);
  //barrier(CLK_LOCAL_MEM_FENCE);
  if (crc1 == crc2){*/
  /* The error is fixed. Overwrite the original buffer with
   * the corrected sequence, and returns the error bit
   * position.
   * This should only happen in one work-unit if the number of errors in the
   * bitf_message was few enough. I see no problem if multiple units finds a
   * solution and overwrites each other however.
   * */
  /* for (int i = 0; i < (*bits)/8; i+=2){
       msg[i] = aux[i];
       msg[i+1] = aux[i+1];
   }
   returnval[j] = j;
}*/
  // if we couldn't fix it, dont touch returnval.
}
