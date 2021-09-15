//
// Created by timmy on 9/14/21.
//

#ifndef COOLBEANS_DATA_SOURCE_H
#define COOLBEANS_DATA_SOURCE_H


#include <cstddef>
#include <array>
#include <iostream>
#include <exception>
#define BUFFER_SIZE 4096*4096

struct SourceException: public std::exception {
    /* Catch-all exception for data_source structures. */
    [[nodiscard]] const char * what () const noexcept override
    {
        return "Unhandled SourceException thrown";
    }
};

struct NoSourceException: public SourceException{
    const char* message = "No sources found";
};

class data_source {
    /* Provides an interface to read data in a stream-like fashion from a custom source */
protected:
    std::array<char, BUFFER_SIZE> buffer{};
public:
    data_source();
    constexpr virtual bool fill_buffer() = 0;

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
