#include "DataSource.h"

const char *datasource::SourceException::message() const {
    if (this->msg) {
        return this->msg;
    }
    return nullptr;
}

datasource::SourceException::SourceException(const char *message) {
    this->msg = message;
}

const char *datasource::SourceException::what() const noexcept {
    return "An exception occured during handling of datasource";
}


datasource::DataSource::Iterator::Iterator(datasource::DataSource &src) : src(&src) {
    this->buf = src.get_data();
    this->bufiter = this->buf.begin();
}

unsigned char &datasource::DataSource::Iterator::operator*() {
    check_buffer();

    return *this->bufiter;
}

datasource::DataSource::Iterator &datasource::DataSource::Iterator::operator++() {
    check_buffer();
    this->bufiter++;
    return *this;
}

datasource::DataSource::Iterator datasource::DataSource::Iterator::operator++(int) {
    check_buffer();
    Iterator tmp = *this;
    ++(*this);
    return tmp; }

bool datasource::operator==(const datasource::DataSource::Iterator &a, const datasource::DataSource::Iterator &b) { return a.bufiter == b.bufiter; }

bool datasource::operator!=(const datasource::DataSource::Iterator &a, const datasource::DataSource::Iterator &b) { return a.bufiter != b.bufiter; }

void datasource::DataSource::Iterator::check_buffer() {
    if (bufiter == buf.end()) {
        buf = src->get_data();
    }
}
