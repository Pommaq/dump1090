//
// Created by timmy on 9/14/21.
//

#include <cstddef>
#include "data_source.h"

data_source::data_source() {
    this->buffer = std::array<char, BUFFER_SIZE>();

}
