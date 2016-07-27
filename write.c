/**
 * @author Gar|k <garik.djan@gmail.com>
 * @copyright (c) 2016, http://c0dedgarik.blogspot.ru/
 * @version 1.0
 */

#include "main.h"

size_t /* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n) {
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (errno == EINTR)
                nwritten = 0; /* and call write() again */
            else
                return (-1); /* error */
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n);
}

size_t /* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n) {
    size_t nleft;
    ssize_t nread;
    char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0; /* and call read() again */
            else
                return (-1);
        } else if (nread == 0)
            break; /* EOF */

        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft); /* return >= 0 */
}

size_t
write_out(const options_t * options, const struct hostent *host) {
    
    if(true == options->benchmark) return 0;
    
    char buf[MAXLINE];
    size_t len;
    
    len = snprintf(buf, sizeof (buf), "%s;%s\n", host->h_name, inet_ntoa(*((struct in_addr *) host->h_addr_list[0])));

    if (len != writen(options->file.out, buf, len)) {
        err_ret("[E] write_out");
    }

    return len;
}

int
write_stat(options_t *options, const long time) {
    char buf[MAXLINE];
    size_t len;
    
    if(false == options->benchmark) {
        len = snprintf(buf, sizeof (buf),
            "DNS resolved: %zu; found: %zu; not found: %zu (%zu%%); \
pending requests: %d; time: %ld milliseconds\n",
            options->counters.domains,
            options->counters.dnsfound,
            options->counters.dnsnotfound,
            (options->counters.dnsnotfound > 0 ? ((options->counters.dnsnotfound * 100) / options->counters.domains) : 0),
            options->pending_requests,
            time
            );
    }
    else {
        len = snprintf(buf, sizeof (buf), "%d;%ld;\n", options->pending_requests, time);
    }

    return writen(OUT_DEFAULT, buf, len);
}