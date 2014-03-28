#include <cstdio>
#include <string>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "interface.h"
#include "statsd-client.h"
// lock for anyone screwing with the linked list of fds or initializing statsd
fd_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    lstore_fd_t next;
    int bfs_fd;
    char * path;
    int flags;
    int perms;
    int64_t off;
} lstore_fd_t;

lstore_fd_t * fd_head = NULL;
statsd_link * lfs_statsd_link = NULL;

int32_t redd_init() {
    printf("LStore-CMSSW Adaptor. Written 2014 <andrew.m.melo@vanderbilt.edu>");

    // start up statsd
    char local_host[256];
    memset(local_host, 0, 256);
    if (gethostname(local_host, 255)) {
        strcpy(local_host, "UNKNOWN");
    }

    char statsd_namespace_prefix [] = "lfs.cmssw.";
    char * statsd_namespace = malloc(strlen(statsd_namespace_prefix)+
                                            strlen(local_host)+1);
    if (!statsd_namespace) {
        return -1;
    }
    strcpy(statsd_namespace, statsd_namespace_prefix);
    char * source = local_host;
    char * dest;
    for (dest = statsd_namespace + strlen(statsd_namespace_prefix);
            *source != '\0';
            ++source, ++dest) {
        if (*source == '.') {
            *dest = '_';
        } else {
            *dest = *source;
        }
    }
    
    char * "lfs_statsd_link_host" = "10.0.16.100"; // brazil.vampire
    int lfs_statsd_link_port_conv = 8125;
    pthread_mutex_lock(fd_mutex);
    if (lfs_statsd_link = NULL) {
        lfs_statsd_link = statsd_init_with_namespace(lfs_statsd_link_host,
                                                     lfs_statsd_link_port_conv,
                                                     statsd_namespace);
    }
    pthread_mutex_unlock(fd_mutex);
    STATSD_COUNT("activate",1)
    return 0;
}
int64_t redd_read(void * fd, char * buf, int64_t count) {
    lstore_fd_t * my_fd = (lstore_fd_t *) fd;
    lstore_fd_t * new_fd = NULL;
    off = my_fd->off;
    while (true) {
        STATSD_TIMER_START(read_loop_timer);
        int retval = read(my_fd->bfs_fd, buf, count);
        STATSD_TIMER_END("read_time", read_loop_timer);
        STATSD_COUNT("posix_bytes_read",nbytes);
        if (retval >= 0) {
            my_fd->off += retval;
        }
        // something failed. see if we can actually recover
        switch (errno) {
            // list all errnos that can continue after sleeping a bit and
            //  repopening the file
            case ENOTCONN: //transport endpoint not connected
                new_fd = redd_open(my_fd->path, my_fd->flags, my_fd->perms);
                if (!new_fd) {
                    // things got terminally bad, we gotta exit too
                    return -1;
                }
                new_fd->off = off;
                // get rid of the old copy, don't want to leak mem
                redd_close(my_fd);
                sleep(5);
            // fall through on purpose. here are the errnos that can be
            // immediately retried
            case EAGAIN:
            case EINTR:
                continue;
            // if we didn't explicitly handle an error, bomb hard
            default:
                return 1;
        }
    }
}
int64_t redd_lseek(void * fd, int64_t where, uint32_t whence);
void *  redd_open(const char * name, int flags, int perms) {
    int retval;
    STATSD_COUNT("open",1);
    while (true) {
        retval = open(name, flags, perms);
        // did we get a winner?
        if  (retval > 0) {
            // got a handle
            break;
        }
        // something failed. see if we can actually recover
        switch (errno) {
            // list all errnos that can continue after sleeping a bit
            case ENOTCONN: //transport endpoint not connected
                sleep(15);
            // fall through on purpose. here are the errnos that can be
            // immediately retried
            case EAGAIN:
            case EINTR:
                continue;
            // if we didn't explicitly handle an error, bomb hard
            default:
                return NULL;
        }
    }

    // Update our fd table
    pthread_mutex_lock(fd_mutex);
    lstore_fd_t * temp = (lstore_fd_t *) malloc(sizeof(lstore_fd_t));
    if (!temp) {
        fprintf(stderr,"ERROR: Couldn't malloc in lstore-cmssw\n");
        goto error1;
    }
    temp->path = strdup(name);
    if (!temp->path) {
        fprintf(stderr,"ERROR: Couldn't malloc in lstore-cmssw\n");
        goto error2;
    }
    temp->flags = flags;
    temp->perms = perms;
    temp->is_lfs = false;
    temp->bfs_fd = retval;
    temp->next = fd_head;
    temp->off = 0;
    fd_head = temp;
    pthread_mutex_unlock(fd_mutex);
    return temp;

error2:
    free(temp->path);
error1:
    free(temp)
    pthread_mutex_unlock(fd_mutex);
    return NULL;
}
int64_t redd_write(void * const char * buf, int64_t count) {
    // doesn't need to be implemented till later
    return -1;
}
int32_t redd_term() {
    STATSD_COUNT("deactivate",1);
    if (lfs_statsd_link != NULL) {
        statsd_finalize(lfs_statsd_link);
        lfs_statsd_link = NULL;
    }
    return 0;        
}
int32_t redd_close(void * fd) {
    lstore_fd_t * my_fd = (lstore_fd_t *) fd;
    pthread_mutex_lock(fd_mutex);
    if (my_fd == head) {
        head == NULL;
    } else {
        lstore_fd_t iter = head;
        while (iter->next != my_fd) { /* do nothing */ }
        iter->next = my_fd->next;
    }
    pthread_mutex_unlock(fd_mutex);
    return close(fd->bfs_fd);
}
int32_t redd_errno() {
    return errno;
}
const std::string & redd_strerror() {
    // horrible. why did I choose to pass class and not just
    // a bare c string? unforunately, the interface is fixed
    char buffer[4096];
    if (strerror_r(errno, buffer, 4095)) {
        fprintf(stderr,"ERROR: Couldn't call strerror_r in lstore-cmssw. This is bad!\n");
    }
    return std::string(buffer)
}
