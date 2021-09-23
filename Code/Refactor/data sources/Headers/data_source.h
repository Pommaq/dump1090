#ifndef COOLBEANS_DATA_SOURCE_H
#define COOLBEANS_DATA_SOURCE_H


#include <cstddef>
#include <array>
#include <vector>
#include <iostream>
#include <exception>

struct SourceException : public std::exception {
    /* Catch-all exception for data_source structures. */
    const char *msg = nullptr;

    explicit SourceException(const char *message);

    [[nodiscard]] const char *what() const noexcept override;

    [[nodiscard]] const char *message() const;
};

class data_source {
    /* Provides an interface to read data in a stream-like fashion from a custom source */

public:
    virtual void run() = 0;
    constexpr virtual std::vector<unsigned char>  get_data() = 0;

    /* TODO: Decide if this should use an iterator or stream for an interface. or something else.
     *  We want it to block if we are waiting for data, else die if we are out of data.
     *  Having a stream would lead to troubles if we cannot fill the buffer completely.
     *  Null-termination isnt an option.
     *  --We could create a class with states similar to Rust Optional.
     *  Having a one-way iterator might work well. We can let the user be responsible
     *  for saving the bytes. Two-way wont work since then we will have to keep the buffer
     *  forever since we wont know when it will back up.
     *  I like iterator.
     *  */
};


#endif //COOLBEANS_DATA_SOURCE_H
