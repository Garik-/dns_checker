/**
 * @author Gar|k <garik.djan@gmail.com>
 * @copyright (c) 2016, http://c0dedgarik.blogspot.ru/
 * @version 1.0
 */

#include "main.h"

#define MAGIC 0xDEADBEEF

#pragma pack(push,1)

typedef struct {
    unsigned int magic;
    unsigned int data_len;
} file_record;

#pragma pack(pop)

static ssize_t /* Write "n" bytes to a descriptor. */
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

size_t
write_out(const options_t * options, const struct hostent *host) {
    char buf[MAXLINE];
    int position;
    unsigned int len;
    if (OUT_DEFAULT == options->file.out) {
        len = snprintf(buf, sizeof (buf), "%s: %s\n", host->h_name, inet_ntoa(*((struct in_addr *) host->h_addr_list[0])));

    } else {
        file_record *rec = (file_record *) buf;
        rec->magic = MAGIC;
        position = sizeof (file_record);


        len = strlen(host->h_name);
        memcpy(&buf[position], &len, sizeof (len));
        position += sizeof (len);
        memcpy(&buf[position], host->h_name, len);
        position += len;

        memcpy(&buf[position], &host->h_length, sizeof (host->h_length));
        position += sizeof (host->h_length);
        memcpy(&buf[position], host->h_addr_list[0], host->h_length);
        position += host->h_length;

        rec->data_len = position - sizeof (file_record);

        len = position;
    }

    if (len != writen(options->file.out, buf, len)) {
        err_ret("[E] write_out");
    }

    return len;
}