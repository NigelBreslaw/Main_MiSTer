#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "support/mister_magik/launcher_reply.h"

int main()
{
	char path[128];
	snprintf(path, sizeof(path), "/tmp/mister-magik-reply-test-%d", getpid());
	unlink(path);

	int owner = magik_launcher_reply_open(path);
	assert(owner >= 0);
	int reader = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	assert(reader >= 0);

	assert(magik_launcher_reply_write(owner, "rejected LauncherCrashed"));
	char line[128] = {};
	assert(read(reader, line, sizeof(line)) == 25);
	assert(!strcmp(line, "rejected LauncherCrashed\n"));

	char oversized[300];
	memset(oversized, 'x', sizeof(oversized));
	oversized[sizeof(oversized) - 1] = 0;
	assert(!magik_launcher_reply_write(owner, oversized));
	assert(errno == EMSGSIZE);

	close(owner);
	assert(read(reader, line, sizeof(line)) == 0);
	close(reader);
	unlink(path);

	int regular = open(path, O_CREAT | O_WRONLY | O_CLOEXEC, 0600);
	assert(regular >= 0);
	close(regular);
	assert(magik_launcher_reply_open(path) < 0);
	assert(errno == EINVAL);
	unlink(path);
	assert(symlink("/tmp", path) == 0);
	assert(magik_launcher_reply_open(path) < 0);
	assert(errno == EINVAL);
	unlink(path);
	return 0;
}
