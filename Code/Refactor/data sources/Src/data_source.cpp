#include "data_source.h"

const char *DataSource::SourceException::message() const {
    if (this->msg) {
        return this->msg;
    }
    return nullptr;
}

DataSource::SourceException::SourceException(const char *message) {
    this->msg = message;
}

const char *DataSource::SourceException::what() const noexcept {
    return "An exception occured during handling of datasource";
}


