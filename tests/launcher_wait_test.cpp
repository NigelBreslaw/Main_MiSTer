#include <assert.h>
#include <chrono>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include "support/mister_magik/launcher_wait.h"

static long elapsed_ms(std::chrono::steady_clock::time_point started)
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
	           std::chrono::steady_clock::now() - started)
	    .count();
}

static void drain_pending_signal_notifications()
{
	while (magik_launcher_wait_for_activity(-1, 0))
	{
	}
}

static void wait_for_child(pid_t child)
{
	pid_t result;
	do
	{
		result = waitpid(child, nullptr, 0);
	}
	while (result < 0 && errno == EINTR);
	assert(result == child);
}

int main()
{
	auto started = std::chrono::steady_clock::now();
	assert(!magik_launcher_wait_for_activity(-1, 30));
	assert(elapsed_ms(started) >= 15);

	int command_pipe[2];
	assert(pipe(command_pipe) == 0);
	pid_t command_writer = fork();
	assert(command_writer >= 0);
	if (!command_writer)
	{
		close(command_pipe[0]);
		usleep(50000);
		const char command = 'x';
		assert(write(command_pipe[1], &command, 1) == 1);
		usleep(100000);
		_exit(0);
	}
	close(command_pipe[1]);
	started = std::chrono::steady_clock::now();
	assert(magik_launcher_wait_for_activity(command_pipe[0], 1000));
	assert(elapsed_ms(started) < 500);
	char command = 0;
	assert(read(command_pipe[0], &command, 1) == 1);
	assert(command == 'x');
	close(command_pipe[0]);
	wait_for_child(command_writer);
	drain_pending_signal_notifications();

	pid_t child = fork();
	assert(child >= 0);
	if (!child)
	{
		usleep(50000);
		_exit(0);
	}
	started = std::chrono::steady_clock::now();
	assert(magik_launcher_wait_for_activity(-1, 1000));
	assert(elapsed_ms(started) < 500);
	wait_for_child(child);

	return 0;
}
