#pragma once

#include <stddef.h>

bool magik_dev_layout();
const char *magik_app_dir();
const char *magik_main_path();
const char *magik_launcher_relative_path();
void magik_app_path(char *out, size_t size, const char *relative);
