/**
 * @author Gar|k <garik.djan@gmail.com>
 * @copyright (c) 2016, http://c0dedgarik.blogspot.ru/
 * @version 1.0
 */

#include "main.h"

static int sigterm;
#define CHECK_TERM() if(0 != sigterm) break;

static void
sig_handler(int signum) {
    sigterm = 1;
}

static long
mtime() {
    struct timeval t;

    gettimeofday(&t, NULL);
    long mt = (long) t.tv_sec * 1000 + t.tv_usec / 1000;
    return mt;
}

static void
print_usage(const char * name) {
    err_quit("Usage: %s [KEY]... DOMAIN-LIST\n\n\
\t-n\tnumber asynchronous requests, default %d\n\
\t-o\toutput file found domains, default stdout\n\
\t-c\tcontinue with the last entry of the output file\n\n", name, MAXPENDING);
}

static char *
parse_dsv(char *line, size_t len, ptrdiff_t * line_size, const char delimiter) {
    char * end = memchr(line, '\n', len);
    *line_size = 0;

    if (NULL != end) {
        char * separator = memchr(line, delimiter, end - line);
        if (NULL != separator) {

            *separator = 0;
            *line_size = separator - line;

        }
        return ++end;
    }

    return NULL;
}

static char *
last_domain(const int fd) {

    assert(OUT_DEFAULT != fd);

    struct stat statbuf;
    char buffer[MAXLINE], *last_domain = NULL;

    if (fstat(fd, &statbuf) < 0 && S_ISREG(statbuf.st_mode)) {
        err_sys("fstat");
    }

    if (lseek(fd, (sizeof (buffer) > statbuf.st_size ? 0 : statbuf.st_size - sizeof (buffer)), SEEK_SET) < 0) {
        err_sys("lseek");
    }

    size_t len = readn(fd, buffer, sizeof (buffer));
    if (len > 0) {
        char * line, *end_line;
        line = buffer;
        ptrdiff_t line_size = 0;

        while (NULL != (end_line = parse_dsv(line, len - (line - buffer), &line_size, ';'))) {
            if (line_size > 0 && (line[line_size - 1] == 'U' || line[line_size - 1] == 'u')) {
                last_domain = line;
            }

            line = end_line;
        }
    }

    lseek(fd, 0, SEEK_END);
    debug("last domain: %s", last_domain);

    return strdup(last_domain);
}

static void *
main_loop(void *vptr_args) {
    options_t *options = (options_t *) vptr_args;

    const size_t page_size = (size_t) sysconf(_SC_PAGESIZE);

    off_t offset = 0;
    size_t pending_requests = 0;
    bool found = (NULL != options->file.last_domain ? false : true);

    while (offset < options->file.len) { // file mapping loop
        char * buffer = (char *) mmap(0, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, options->file.fd, offset);
        if (MAP_FAILED != buffer) {

            char * line, *end_line;
            line = buffer;
            ptrdiff_t line_size = 0;
            
            while (NULL != (end_line = parse_dsv(line, page_size - (line - buffer), &line_size, ';'))) {

                CHECK_TERM();

                if (line_size > 0 && (line[line_size - 1] == 'U' || line[line_size - 1] == 'u')) {

                    if (found) {

                        ev_ares_gethostbyname(options, line);
                        ++pending_requests;

                    } else {
                        found = (0 == strcmp(line, options->file.last_domain) ? true : false);
                    }


                }

                line = end_line;

                if (MAXPENDING == pending_requests) {
                    debug("pending_requests: %zu", pending_requests);
                    ev_run(options->loop, 0);
                    pending_requests = 0;
                }

            }

            munmap(buffer, page_size);

        } else {
            err_ret("mmap");
            break;
        }

        CHECK_TERM();

        offset += page_size;
    }

    if (pending_requests > 0) {
        debug("pending_requests: %zu", pending_requests);
        ev_run(options->loop, 0);
        pending_requests = 0;
    }


    return NULL;
}

static void
init_options(options_t *options) {
    memset(options, 0, sizeof (options_t));

    options->loop = EV_DEFAULT;
    options->file.out = OUT_DEFAULT;
    options->file.last_domain = NULL;
    options->timeout = MAXDNSTIME;
    options->pending_requests = MAXPENDING;
    options->benchmark = false;
}

static void
free_options(options_t *options) {
    if (OUT_DEFAULT != options->file.out) {
        close(options->file.out);
    }

    if (NULL != options->file.last_domain) {
        free(options->file.last_domain);
    }
}

int main(int argc, char** argv) {

    const long time_start = mtime();

    if (1 == argc) {
        print_usage(argv[0]);
    }

    char *opts = "cbn:o:";
    int opt, status = EXIT_SUCCESS;
    bool f_continue = false;

    options_t options;
    init_options(&options);

    sigterm = 0;
    (void) signal(SIGHUP, sig_handler);
    (void) signal(SIGINT, sig_handler);
    (void) signal(SIGTERM, sig_handler);

    while ((opt = getopt(argc, argv, opts)) != -1) {
        switch (opt) {
            case 'b':
                options.benchmark = true;
                break;
            case 'n':
                options.pending_requests = atoi(optarg);
                break;
            case 'o':
                if ((options.file.out = open(optarg, O_APPEND | O_CREAT | O_RDWR,
                        S_IRUSR | S_IWUSR)) < 0) {
                    err_sys("[E] open output file %s", optarg);
                }
                break;
            case 'c':
                f_continue = true;
                break;
            case '?':
                print_usage(argv[0]);
        }
    }

    if (argc == optind) {
        free_options(&options);
        print_usage(argv[0]);
    }

    if (true == f_continue && OUT_DEFAULT != options.file.out) {
        options.file.last_domain = last_domain(options.file.out);
    }

    if (ARES_SUCCESS == (status = ares_library_init(ARES_LIB_INIT_ALL))) {

        if (ARES_SUCCESS == (status = ev_ares_init_options(&options))) {

            if ((options.file.fd = open(argv[optind], O_RDWR)) > 0) {

                struct stat statbuf;

                if (fstat(options.file.fd, &statbuf) < 0 && S_ISREG(statbuf.st_mode)) {
                    err_ret("fstat");
                    status = EXIT_FAILURE;
                } else {
                    options.file.len = statbuf.st_size;

                    // pthread_t threads;
                    // pthread_create(&threads, NULL, (void *(*)(void *)) main_loop, &options);
                    // pthread_join(threads, NULL);

                    main_loop(&options);
                }

                close(options.file.fd);
            } else {
                err_ret("[E] domain list %s", argv[optind]);
                status = EXIT_FAILURE;
            }

        } else {
            err_ret("Ares error: %s", ares_strerror(status));
            status = EXIT_FAILURE;
        }

        ares_library_cleanup();
    } else {
        err_ret("Ares error: %s", ares_strerror(status));
        status = EXIT_FAILURE;
    }

    write_stat(&options, (mtime() - time_start));
    free_options(&options);

    return status;
}
