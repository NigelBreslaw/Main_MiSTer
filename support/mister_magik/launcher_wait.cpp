#include "launcher_wait.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

static int s_sigchld_read_fd = -1;
static volatile sig_atomic_t s_sigchld_write_fd = -1;
static bool s_sigchld_setup_attempted;

static void notify_sigchld(int)
{
	int saved_errno = errno;
	int fd = static_cast<int>(s_sigchld_write_fd);
	if (fd >= 0)
	{
		const unsigned char notification = 1;
		(void)write(fd, &notification, sizeof(notification));
	}
	errno = saved_errno;
}

static bool set_nonblocking_cloexec(int fd)
{
	int status_flags = fcntl(fd, F_GETFL, 0);
	if (status_flags < 0 || fcntl(fd, F_SETFL, status_flags | O_NONBLOCK) < 0)
		return false;
	int descriptor_flags = fcntl(fd, F_GETFD, 0);
	return descriptor_flags >= 0 &&
	       fcntl(fd, F_SETFD, descriptor_flags | FD_CLOEXEC) == 0;
}

static void ensure_sigchld_wait_pipe()
{
	if (s_sigchld_setup_attempted) return;
	s_sigchld_setup_attempted = true;

	int fds[2];
	if (pipe(fds) < 0) return;
	if (!set_nonblocking_cloexec(fds[0]) || !set_nonblocking_cloexec(fds[1]))
	{
		close(fds[0]);
		close(fds[1]);
		return;
	}

	s_sigchld_read_fd = fds[0];
	s_sigchld_write_fd = fds[1];
}

static void drain_sigchld_notifications()
{
	if (s_sigchld_read_fd < 0) return;
	unsigned char notifications[64];
	while (read(s_sigchld_read_fd, notifications, sizeof(notifications)) > 0)
	{
	}
}

bool magik_launcher_wait_for_activity(int command_fd, int timeout_ms)
{
	ensure_sigchld_wait_pipe();
	struct sigaction previous = {};
	bool sigchld_handler_installed = false;
	if (s_sigchld_read_fd >= 0 &&
	    sigaction(SIGCHLD, nullptr, &previous) == 0 &&
	    previous.sa_handler == SIG_DFL)
	{
		struct sigaction action = {};
		action.sa_handler = notify_sigchld;
		sigemptyset(&action.sa_mask);
		action.sa_flags = SA_NOCLDSTOP | SA_RESTART;
		sigchld_handler_installed = sigaction(SIGCHLD, &action, nullptr) == 0;
	}

	struct pollfd fds[2] = {};
	nfds_t count = 0;
	if (command_fd >= 0)
	{
		fds[count].fd = command_fd;
		fds[count].events = POLLIN;
		count++;
	}
	int sigchld_index = -1;
	if (sigchld_handler_installed)
	{
		sigchld_index = static_cast<int>(count);
		fds[count].fd = s_sigchld_read_fd;
		fds[count].events = POLLIN;
		count++;
	}

	int result = poll(fds, count, timeout_ms);
	int poll_errno = errno;
	if (sigchld_handler_installed)
		(void)sigaction(SIGCHLD, &previous, nullptr);
	if (result < 0) return poll_errno == EINTR;
	if (!result) return false;
	if (sigchld_index >= 0 && fds[sigchld_index].revents)
		drain_sigchld_notifications();
	return true;
}
