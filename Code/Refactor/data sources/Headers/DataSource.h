#ifndef COOLBEANS_DATA_SOURCE_H
#define COOLBEANS_DATA_SOURCE_H


#include <cstddef>
#include <iterator>
#include <array>
#include <vector>
#include <iostream>
#include <exception>
// HOWTO iterator https://stackoverflow.com/questions/37031805/preparation-for-stditerator-being-deprecated/38103394
//      https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp

namespace datasource {

    struct SourceException : public std::exception {
        /* Catch-all exception for data_source structures. */
        const char *msg = nullptr;

        explicit SourceException(const char *message);

        [[nodiscard]] const char *what() const noexcept override;

        [[nodiscard]] const char *message() const;
    };

    class DataSource {
        /* Provides an interface to read data in a stream-like fashion from a custom source */

    public:
        virtual void run() = 0;
        constexpr virtual std::vector<unsigned char>  get_data() = 0;

        struct Iterator {
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = unsigned char;
            using pointer = unsigned char*;
            using reference = unsigned char&;
            /*
             * DataSource has to be valid during lifetime of iterator or else undefined behavior
             * will occur.
             *
             */
            Iterator(DataSource& src);

            reference operator*();



            Iterator& operator++();

            Iterator operator++(int);

            friend bool operator== (const Iterator& a, const Iterator& b);;
            friend bool operator!= (const Iterator& a, const Iterator& b);;

        private:
            DataSource* src;
            std::vector<unsigned char> buf;
            std::vector<unsigned char>::iterator bufiter;

            // Makes sure the internal buffer contains valid data.
            void check_buffer();
        };
    };


}


#endif //COOLBEANS_DATA_SOURCE_H
