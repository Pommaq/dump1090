#ifndef DUMP1090_MODESMESSAGE_H
#define DUMP1090_MODESMESSAGE_H
#include <string>

class modesMessage {
    /*
     Class representing a modes message before being decoded.
     */
private:
protected:
    std::string message;
    int message_type;
    uint32_t expected_crc;

public:
    [[nodiscard]] bool is_broken() const;
    modesMessage(const std::string& msg, int msg_type);
    [[nodiscard]] uint32_t checksum() const;
};

#endif //DUMP1090_MODESMESSAGE_H
