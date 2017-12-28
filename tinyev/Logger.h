//
// Created by frank on 17-2-12.
//

#ifndef FANCY_LOGGER_H
#define FANCY_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_LEVEL_TRACE     0
#define LOG_LEVEL_DEBUG     1
#define LOG_LEVEL_INFO      2
#define LOG_LEVEL_WARN      3
#define LOG_LEVEL_ERROR     4
#define LOG_LEVEL_FATAL     5

extern int logLevel;

void setLogLevel(int level);

void setLogFd(int fd);

/* private, do not use  */
void log_base(const char *file,
              int line,
              int level,
              int to_abort,
              const char *fmt, ...);

void log_sys(const char *file,
             int line,
             int to_abort,
             const char *fmt, ...);

/* private, do not use  */
#define LOG_BASE(level, to_abort, fmt, ...) \
log_base(__FILE__, __LINE__, level, to_abort, fmt, ##__VA_ARGS__)
#define LOG_SYS(to_abort, fmt, ...) \
log_sys(__FILE__, __LINE__, to_abort, fmt, ##__VA_ARGS__)

/* public  */
#define TRACE(fmt, ...)     if(logLevel <= LOG_LEVEL_TRACE) \
LOG_BASE(LOG_LEVEL_TRACE, 0, fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...)     if(logLevel <= LOG_LEVEL_DEBUG) \
LOG_BASE(LOG_LEVEL_DEBUG, 0, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...)      if(logLevel <= LOG_LEVEL_INFO) \
LOG_BASE(LOG_LEVEL_INFO, 0, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...)      if(logLevel <= LOG_LEVEL_WARN) \
LOG_BASE(LOG_LEVEL_WARN, 0, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...)     LOG_BASE(LOG_LEVEL_ERROR, 0, fmt, ##__VA_ARGS__)
#define FATAL(fmt, ...)     LOG_BASE(LOG_LEVEL_FATAL, 1, fmt, ##__VA_ARGS__)
#define SYSERR(fmt, ...)    LOG_SYS(0, fmt, ##__VA_ARGS__)
#define SYSFATAL(fmt, ...)  LOG_SYS(1, fmt, ##__VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif //FANCY_LOGGER_H
