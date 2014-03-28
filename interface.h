#ifndef __LSTORE_CMSSW_INTERFACE_H
#define __LSTORE_CMSSW_INTERFACE_H
#include <string>
#include "statsd-client.h"
extern statsd_link * lfs_statsd_link;
#define STATSD_COUNT(name, count) if (lfs_statsd_link) { statsd_count(lfs_statsd_link, name, count, 1.0); }                                                                 
#define STATSD_TIMER_START(variable) time_t variable; time(& variable );
#define STATSD_TIMER_END(name, variable) time_t variable ## _end; if (lfs_statsd_link) { time(& variable ## _end); statsd_timing(lfs_statsd_link, name, (int) (difftime(variable ## _end, variable) * 1000.0)); }
#include <string>
int32_t redd_init();
int64_t redd_read(void * fd, char * buf, int64_t count);
int64_t redd_lseek(void * fd, int64_t where, uint32_t whence);
void *  redd_open(const char * name, int flags, int perms);
int64_t redd_write(void * const char * buf, int64_t count);
int32_t redd_term();
int32_t redd_errno();
const std::string & redd_strerror();
#endif //__LSTORE_CMSSW_INTERFACE_H
