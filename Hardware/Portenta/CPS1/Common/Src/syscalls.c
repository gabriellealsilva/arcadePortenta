/*
 * Minimal newlib syscall stubs for bare-metal STM32H747.
 *
 * Overrides libnosys.a so the linker does not emit "not implemented"
 * warnings. None of these are ever expected to succeed at runtime;
 * real I/O goes through HAL / RTT, not through the C stdio layer.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>

/* Linker-script symbols used by _sbrk */
extern uint8_t _end;
extern uint8_t _estack;
extern uint32_t _Min_Stack_Size;

int _close(int fd);
int _fstat(int fd, struct stat *st);
int _isatty(int fd);
off_t _lseek(int fd, off_t offset, int whence);
int _open(const char *path, int flags, ...);
int _read(int fd, char *buf, int len);
int _write(int fd, const char *buf, int len);
void _exit(int status);
int _kill(int pid, int sig);
int _getpid(void);
void *_sbrk(ptrdiff_t incr);

int _close(int fd)
{
    (void)fd;
    errno = EBADF;
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd)
{
    (void)fd;
    return 1;
}

off_t _lseek(int fd, off_t offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;
    errno = ESPIPE;
    return (off_t)-1;
}

int _open(const char *path, int flags, ...)
{
    (void)path;
    (void)flags;
    errno = ENOSYS;
    return -1;
}

int _read(int fd, char *buf, int len)
{
    (void)fd;
    (void)buf;
    (void)len;
    errno = ENOSYS;
    return -1;
}

__attribute__((weak)) int _write(int fd, const char *buf, int len)
{
    (void)fd;
    (void)buf;
    return len;   /* silent discard — override with RTT/UART implementation */
}

void _exit(int status)
{
    (void)status;
    while (1) {}
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}

void *_sbrk(ptrdiff_t incr)
{
    static uint8_t *heap_end = NULL;
    const uint32_t stack_limit = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;

    if (heap_end == NULL)
        heap_end = &_end;

    if ((uint32_t)(heap_end + incr) > stack_limit)
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    void *prev = heap_end;
    heap_end += incr;
    return prev;
}