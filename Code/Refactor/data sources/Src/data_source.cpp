#include "data_source.h"

const char *SourceException::message() const {
    if (this->msg) {
        return this->msg;
    }
    return nullptr;
}

SourceException::SourceException(const char *message) {
    this->msg = message;
}

const char *SourceException::what() const noexcept {
    return "An exception occured during handling of datasource";
}


