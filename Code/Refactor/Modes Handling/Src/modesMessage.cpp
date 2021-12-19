#include "modesMessage.h"
#include "decoding.h"


modesMessage::modesMessage(const std::string &msg, int msg_type) : message(msg), message_type(msg_type) {
    /*
     This class expects the caller to have already detected the preamble and the message type,
     passing the length of our data to it. It also assumes the caller knows the buffer is large enough for us to copy.
     */

    // CRC is always the last three bytes.
    unsigned long int message_bits = this->message.length();
    this->expected_crc = ((uint32_t) msg[message_bits - 3] << 16) |
                         ((uint32_t) msg[message_bits - 2] << 8) |
                         (uint32_t) msg[message_bits - 1];
}

uint32_t modesMessage::checksum() const {
    return modesChecksum(this->message);
}

bool modesMessage::is_broken() const {
    return this->expected_crc != this->checksum();
}
