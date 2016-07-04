/**
 * @author Gar|k <garik.djan@gmail.com>
 * @copyright (c) 2016, http://c0dedgarik.blogspot.ru/
 * @version 1.0
 */

#include "main.h"

#define MAGIC 0xDEADBEEF

#pragma pack(push,1)

typedef struct {
    ssize_t len;
    char *buf;
} lps;

typedef struct {
    unsigned int magic;
    unsigned int data_len;
    struct sockaddr_in servaddr;
    ssize_t len;
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