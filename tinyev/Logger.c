//
// Created by frank on 17-2-12.
//

#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include <tinyev/Logger.h>

#define MAXLINE     256

#ifndef NDEBUG
int logLevel = LOG_LEVEL_DEBUG;
#else
int logLevel = LOG_LEVEL_INFO;
#endif

static const char *log_level_str[] = {
        "[TRACE]",
        "[DEBUG]",
        "[INFO] ",
        "[WARN] ",
        "[ERROR]",
        "[FATAL]"
};

static int log_fd = STDOUT_FILENO;

static int timestamp(char *data, size_t len);

void setLogLevel(int level)
{
    if (level < LOG_LEVEL_TRACE)
        level = LOG_LEVEL_TRACE;
    if (level > LOG_LEVEL_FATAL)
        level = LOG_LEVEL_FATAL;

    logLevel = level;
}

void setLogFd(int fd)
{
    if (fd <= 0)
        fd = STDOUT_FILENO;
    log_fd = fd;
}

void log_base(const char *file,
              int line,
              int level,
              int to_abort,
              const char *fmt, ...)
{
    char        data[MAXLINE];
    size_t      i = 0;
    va_list     ap;

    i += timestamp(data, 32);
    i += sprintf(data + i, " [%d]", getpid());
    i += sprintf(data + i, " %s ", log_level_str[level]);

    va_start(ap, fmt);
    vsnprintf(data + i, MAXLINE - i, fmt, ap);
    va_end(ap);

    int err = dprintf(log_fd, "%s - %s:%d\n",
                      data, strrchr(file, '/') + 1, line);
    if (err == -1) {
        fprintf(stderr, "log failed");
    }
    if (to_abort) {
        abort();
    }
}

void log_sys(const char *file,
             int line,
             int to_abort,
             const char *fmt, ...)
{
    char        data[MAXLINE];
    size_t      i = 0;
    va_list     ap;

    i += timestamp(data, 32);
    i += sprintf(data + i, " [%d]", getpid());
    i += sprintf(data + i, " %s ", to_abort ? "[SYSFA]":"[SYSER]");

    va_start(ap, fmt);
    vsnprintf(data + i, MAXLINE - i, fmt, ap);
    va_end(ap);

    dprintf(log_fd, "%s: %s - %s:%d\n",
            data, strerror(errno), strrchr(file, '/') + 1, line);

    if (to_abort) {
        abort();
    }
}

static int timestamp(char *data, size_t len)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    time_t seconds = tv.tv_sec;

    struct tm tm_time;

    gmtime_r(&seconds, &tm_time);

    return snprintf(data, len, "%4d%02d%02d %02d:%02d:%02d.%06ld",
                    tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                    tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, tv.tv_usec);
}