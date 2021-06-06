//
// Created by timmy on 2021-05-11.
//

#include <csignal>
#include "../Headers/main.hpp"

g_settings Modes;

int main(int argc, char **argv) {
    int j;

    /* Set sane defaults. */
    modesInitConfig();

    /* Parse the argument line options */
    std::string argument;
    for (j = 1; j < argc; j++) {
        int more = j + 1 < argc; /* There are more arguments. */
        argument = argv[j]; // Using a string entity to make this mess slightly more readable
        if (argument == "--device-index" && more) {
            Modes.dev_index = atoi(argv[++j]);
        } else if (argument == "--gain" && more) {
            Modes.gain = atof(argv[++j]) * 10; /* Gain is in tens of DBs */
        } else if (argument == "--enable-agc") {
            Modes.enable_agc++;
        } else if (argument == "--freq" && more) {
            Modes.freq = strtoll(argv[++j], NULL, 10);
        } else if (argument == "--ifile" && more) {
            Modes.filename = argv[++j];
        } else if (argument == "--loop") {
            Modes.loop = 1;
        } else if (argument == "--no-fix") {
            Modes.fix_errors = 0;
        } else if (argument == "--no-crc-check") {
            Modes.check_crc = 0;
        } else if (argument == "--raw") {
            Modes.raw = 1;
        } else if (argument == "--net") {
            Modes.net = 1;
        } else if (argument == "--net-only") {
            Modes.net = 1;
            Modes.net_only = 1;
        } else if (argument == "--net-ro-port" && more) {
            modesNetServices[MODES_NET_SERVICE_RAWO].port = atoi(argv[++j]);
        } else if (argument == "--net-ri-port" && more) {
            modesNetServices[MODES_NET_SERVICE_RAWI].port = atoi(argv[++j]);
        } else if (argument == "--net-http-port" && more) {
            modesNetServices[MODES_NET_SERVICE_HTTP].port = atoi(argv[++j]);
        } else if (argument == "--net-sbs-port" && more) {
            modesNetServices[MODES_NET_SERVICE_SBS].port = atoi(argv[++j]);
        } else if (argument == "--onlyaddr") {
            Modes.onlyaddr = 1;
        } else if (argument == "--metric") {
            Modes.metric = 1;
        } else if (argument == "--aggressive") {
            Modes.aggressive++;
        } else if (argument == "--interactive") {
            Modes.interactive = 1;
        } else if (argument == "--interactive-rows") {
            Modes.interactive_rows = atoi(argv[++j]);
        } else if (argument == "--interactive-ttl") {
            Modes.interactive_ttl = atoi(argv[++j]);
        } else if (argument == "--debug" && more) {
            char *f = argv[++j];
            while (*f) {
                switch (*f) {
                    case 'D':
                        Modes.debug |= MODES_DEBUG_DEMOD;
                        break;
                    case 'd':
                        Modes.debug |= MODES_DEBUG_DEMODERR;
                        break;
                    case 'C':
                        Modes.debug |= MODES_DEBUG_GOODCRC;
                        break;
                    case 'c':
                        Modes.debug |= MODES_DEBUG_BADCRC;
                        break;
                    case 'p':
                        Modes.debug |= MODES_DEBUG_NOPREAMBLE;
                        break;
                    case 'n':
                        Modes.debug |= MODES_DEBUG_NET;
                        break;
                    case 'j':
                        Modes.debug |= MODES_DEBUG_JS;
                        break;
                    default:
                        fprintf(stderr, "Unknown debugging flag: %c\n", *f);
                        exit(1);
                        break;
                }
                f++;
            }
        } else if (argument == "--stats") {
            Modes.stats = 1;
        } else if (argument == "--html_file" && more) {
            Modes.html_file = argv[++j];
        } else if (argument == "--snip" && more) {
            snipMode(atoi(argv[++j]));
            exit(0);
        } else if (argument == "--help") {
            showHelp();
            exit(0);
        } else {
            std::cerr << "Unknown or not enough arguments for option '" << argv[j] << "'" << std::endl;
            showHelp();
            exit(1);
        }
    }

    /* Setup for SIGWINCH for handling lines */
    if (Modes.interactive == 1) signal(SIGWINCH, sigWinchCallback);

    /* Initialization */

    modesInit();
    if (Modes.net_only) {
        fprintf(stderr, "Net-only mode, no RTL device or file open.\n");
    } else if (Modes.filename.empty()) {
        modesInitRTLSDR();
    } else {
        if (Modes.filename[0] == '-' && Modes.filename[1] == '\0') {
            Modes.fd = STDIN_FILENO;
        } else if ((Modes.fd = open(Modes.filename, O_RDONLY)) == -1) {
            perror("Opening data file");
            exit(1);
        }
    }
    if (Modes.net) modesInitNet();

    /* If the user specifies --net-only, just run in order to serve network
     * clients without reading data from the RTL device. */
    while (Modes.net_only) {
        backgroundTasks();
        modesWaitReadableClients(100);
    }

    /* Create the thread that will read the data from the device. */
    pthread_create(&Modes.reader_thread, NULL, readerThreadEntryPoint, NULL);

    pthread_mutex_lock(&Modes.data_mutex);
    while (1) {
        if (!Modes.data_ready) {
            pthread_cond_wait(&Modes.data_cond, &Modes.data_mutex);
            continue;
        }
        computeMagnitudeVector();

        /* Signal to the other thread that we processed the available data
         * and we want more (useful for --ifile). */
        Modes.data_ready = 0;
        pthread_cond_signal(&Modes.data_cond);

        /* Process data after releasing the lock, so that the capturing
         * thread can read data while we perform computationally expensive
         * stuff * at the same time. (This should only be useful with very
         * slow processors). */
        pthread_mutex_unlock(&Modes.data_mutex);
        detectModeS(Modes.magnitude, Modes.data_len / 2);
        backgroundTasks();
        pthread_mutex_lock(&Modes.data_mutex);
        if (Modes.exit) break;
    }

    /* If --ifile and --stats were given, print statistics. */
    if (Modes.stats && Modes.filename) {
        printf("%lld valid preambles\n", Modes.stat_valid_preamble);
        printf("%lld demodulated again after phase correction\n",
               Modes.stat_out_of_phase);
        printf("%lld demodulated with zero errors\n",
               Modes.stat_demodulated);
        printf("%lld with good crc\n", Modes.stat_goodcrc);
        printf("%lld with bad crc\n", Modes.stat_badcrc);
        printf("%lld errors corrected\n", Modes.stat_fixed);
        printf("%lld single bit errors\n", Modes.stat_single_bit_fix);
        printf("%lld two bits errors\n", Modes.stat_two_bits_fix);
        printf("%lld total usable messages\n",
               Modes.stat_goodcrc + Modes.stat_fixed);
    }

    if (Modes.filename != NULL) { free(Modes.filename); }
    free(Modes.html_file);

    rtlsdr_close(Modes.dev);
    return 0;
}

