/*******************************************************************************/
/* Permission is hereby granted, free of charge, to any person or organization */
/* obtaining a copy of the software and accompanying documentation covered by  */
/* this license (the "Software") to use, reproduce, display, distribute,       */
/* execute, and transmit the Software, and to prepare derivative works of the  */
/* Software, and to permit third-parties to whom the Software is furnished to  */
/* do so, all subject to the following:                                        */
/*                                                                             */
/* The copyright notices in the Software and this entire statement, including  */
/* the above license grant, this restriction and the following disclaimer,     */
/* must be included in all copies of the Software, in whole or in part, and    */
/* all derivative works of the Software, unless such copies or derivative      */
/* works are solely in the form of machine-executable object code generated by */
/* a source language processor.                                                */
/*                                                                             */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    */
/* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT   */
/* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE   */
/* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, */
/* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER */
/* DEALINGS IN THE SOFTWARE.                                                   */
/*******************************************************************************/

#include <lfp/unistd.h>
#include <lfp/stdlib.h>
#include <lfp/string.h>
#include <lfp/errno.h>

#include <limits.h>
#include <stdio.h>

#include "utils.h"

off_t lfp_lseek(int fd, off_t offset, int whence)
{
    return lseek(fd, offset, whence);
}



int lfp_pipe (int pipefd[2], uint64_t flags)
{
#if defined(HAVE_PIPE2)
    // We assume that if pipe2() is defined then O_CLOEXEC too
    // exists, which means that it's in the lower part of "flags"
    return pipe2(pipefd, (int)flags & 0xFFFFFFFF);
#else
    if (pipe(pipefd) < 0) {
        goto error_return;
    }

    if ((flags & O_CLOEXEC) &&
        (lfp_set_fd_cloexec(pipefd[0], true) < 0 ||
         lfp_set_fd_cloexec(pipefd[1], true) < 0)) {
        goto error_close;
    }
    if ((flags & O_NONBLOCK) &&
        (lfp_set_fd_nonblock(pipefd[0], true) < 0 ||
         lfp_set_fd_nonblock(pipefd[1], true) < 0)) {
        goto error_close;
    }

    return 0;

  error_close:
    close(pipefd[0]);
    close(pipefd[1]);
  error_return:
    return -1;
#endif // HAVE_PIPE2
}



ssize_t lfp_pread(int fd, void *buf, size_t count, off_t offset)
{
    return pread(fd, buf, count, offset);
}

ssize_t lfp_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    return pwrite(fd, buf, count, offset);
}



int lfp_truncate(const char *path, off_t length)
{
    return truncate(path, length);
}

int lfp_ftruncate(int fd, off_t length)
{
    return ftruncate(fd, length);
}



int lfp_stat(const char *path, struct stat *buf)
{
    return stat(path, buf);
}

int lfp_fstat(int fd, struct stat *buf)
{
    return fstat(fd, buf);
}

int lfp_lstat(const char *path, struct stat *buf)
{
    return lstat(path, buf);
}

int lfp_fd_is_open(int fd)
{
    struct stat buf;
    int ret = fstat(fd, &buf);
    if ( ret < 0 ) {
        if ( lfp_errno() == EBADF ) {
            return false;
        } else {
            return -1;
        }
    } else {
        return true;
    }
}

bool lfp_isreg(mode_t mode)
{
    return (bool) S_ISREG(mode);
}

bool lfp_isdir(mode_t mode)
{
    return (bool) S_ISDIR(mode);
}

bool lfp_ischr(mode_t mode)
{
    return (bool) S_ISCHR(mode);
}

bool lfp_isblk(mode_t mode)
{
    return (bool) S_ISBLK(mode);
}

bool lfp_isfifo(mode_t mode)
{
    return (bool) S_ISFIFO(mode);
}

bool lfp_islnk(mode_t mode)
{
    return (bool) S_ISLNK(mode);
}

bool lfp_issock(mode_t mode)
{
    return (bool) S_ISSOCK(mode);
}



int lfp_execve(const char *path, char *const argv[], char *const envp[])
{
    SYSCHECK(EINVAL, path == NULL);
    SYSCHECK(ENOENT, path[0] == '\0');

    return execve(path, argv, envp);
}

int lfp_execvpe(const char *file, char *const argv[], char *const envp[])
{
    SYSCHECK(EINVAL, file == NULL);
    SYSCHECK(ENOENT, file[0] == '\0');

    if (strchr(file, '/'))
        return execve(file, argv, envp);

    size_t filelen = lfp_strnlen(file, NAME_MAX);
    SYSCHECK(ENAMETOOLONG, filelen >= NAME_MAX);

    char path[PATH_MAX], *searchpath, *tmpath, *bindir;

    tmpath = searchpath = lfp_getpath(envp);

    while ((bindir = strsep(&tmpath, ":")) != NULL)
        if ( bindir[0] != '\0' ) {
            size_t dirlen = lfp_strnlen(bindir, PATH_MAX);
            size_t pathlen = dirlen + 1 + filelen + 1;
            SYSCHECK(ENAMETOOLONG, pathlen > PATH_MAX);
            memset(path, 0, PATH_MAX);
            snprintf(path, PATH_MAX, "%s/%s", bindir, file);
            lfp_execve(path, argv, envp);
            if ( errno == E2BIG  || errno == ENOEXEC ||
                 errno == ENOMEM || errno == ETXTBSY )
                break;
        }

    free(searchpath);

    return -1;
}
