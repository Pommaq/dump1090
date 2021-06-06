//
// Created by timmy on 2021-06-06.
//

#ifndef DUMP1090_DEBUGGING_H
#define DUMP1090_DEBUGGING_H

#include <cstdint>

void dumpMagnitudeBar(int index, int magnitude);
void dumpMagnitudeVector(uint16_t *m, uint32_t offset);
void dumpRawMessageJS(char *descr, unsigned char *msg,
                      uint16_t *m, uint32_t offset, int fixable);
void dumpRawMessage(char *descr, unsigned char *msg,
                    uint16_t *m, uint32_t offset);

#endif //DUMP1090_DEBUGGING_H
