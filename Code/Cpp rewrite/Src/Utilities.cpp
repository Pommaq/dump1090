//
// Created by timmy on 2021-06-06.
//
#include <sys/time.h>
#include "../Headers/Utilities.hpp"

void showHelp() {
    std::cout <<
              "--device-index <index>   Select RTL device (default: 0).\n"
              "--gain <db>              Set gain (default: max gain. Use -100 for auto-gain).\n"
              "--enable-agc             Enable the Automatic Gain Control (default: off).\n"
              "--freq <hz>              Set frequency (default: 1090 Mhz).\n"
              "--ifile <filename>       Read data from file (use '-' for stdin).\n"
              "--loop                   With --ifile, read the same file in a loop.\n"
              "--interactive            Interactive mode refreshing data on screen.\n"
              "--interactive-rows <num> Max number of rows in interactive mode (default: 15).\n"
              "--interactive-ttl <sec>  Remove from list if idle for <sec> (default: 60).\n"
              "--raw                    Show only messages hex values.\n"
              "--net                    Enable networking.\n"
              "--net-only               Enable just networking, no RTL device or file used.\n"
              "--net-ro-port <port>     TCP listening port for raw bitf_output (default: 30002).\n"
              "--net-ri-port <port>     TCP listening port for raw input (default: 30001).\n"
              "--net-http-port <port>   HTTP server port (default: 8080).\n"
              "--net-sbs-port <port>    TCP listening port for BaseStation format bitf_output (default: 30003).\n"
              "--html_file              With --net, sets path to HTML file we serve clients with\n"
              "--no-fix                 Disable single-bits error correction using CRC.\n"
              "--no-crc-check           Disable messages with broken CRC (discouraged).\n"
              "--aggressive             More CPU for more messages (two bits fixes, ...).\n"
              "--stats                  With --ifile print stats at exit. No other bitf_output.\n"
              "--onlyaddr               Show only ICAO addresses (testing purposes).\n"
              "--metric                 Use metric units (meters, km/h, ...).\n"
              "--snip <level>           Strip IQ file removing samples < level.\n"
              "--debug <flags>          Debug mode (verbose), see README for details.\n"
              "--help                   Show this help.\n"
              "\n"
              "Debug mode flags: d = Log frames decoded with errors\n"
              "                  D = Log frames decoded with zero errors\n"
              "                  c = Log frames with bad CRC\n"
              "                  C = Log frames with good CRC\n"
              "                  p = Log frames with bad preamble\n"
              "                  n = Log network debugging info\n"
              "                  j = Log frames to frames.js, loadable by debug.html.\n"
              << std::endl;
}


/* ============================= Utility functions ========================== */

long long mstime() {
    timeval tv;
    long long mst;

    gettimeofday(&tv, nullptr);
    mst = ((long long) tv.tv_sec) * 1000;
    mst += tv.tv_usec / 1000;
    return mst;
}


