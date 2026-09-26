#pragma once

// Only the calling Main input thread is changed. The launcher and all helpers
// keep their own ordinary policies; core handling resumes at the original nice.
struct MagikInputPriorityOps
{
	bool (*read)(int *nice);
	bool (*write)(int nice);
	void (*failed)(const char *operation);
};

class MagikInputPriority
{
public:
	explicit MagikInputPriority(const MagikInputPriorityOps &ops);
	MagikInputPriority();
	~MagikInputPriority();
	MagikInputPriority(const MagikInputPriority &) = delete;
	MagikInputPriority &operator=(const MagikInputPriority &) = delete;

private:
	MagikInputPriorityOps ops_;
	int previous_ = 0;
	bool changed_ = false;
};
