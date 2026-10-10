#include "launcher_reply.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int magik_launcher_reply_open(const char *path)
{
	if (!path || !path[0])
	{
		errno = EINVAL;
		return -1;
	}
	struct stat st;
	if (lstat(path, &st) < 0)
	{
		if (errno != ENOENT || mkfifo(path, 0666) < 0)
			return -1;
	}
	else if (!S_ISFIFO(st.st_mode))
	{
		errno = EINVAL;
		return -1;
	}
	return open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
}

bool magik_launcher_reply_write(int fd, const char *result)
{
	if (fd < 0)
	{
		errno = EBADF;
		return false;
	}
	char line[258];
	int len = snprintf(line, sizeof(line), "%s\n", result ? result : "error empty-reply");
	if (len < 0 || len >= (int)sizeof(line))
	{
		errno = EMSGSIZE;
		return false;
	}
	return write(fd, line, len) == len;
}
