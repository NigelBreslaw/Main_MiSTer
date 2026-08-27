#pragma once

void magik_button_overrides_clear(char values[][32], bool unmap[], int count);
int magik_button_overrides_load(const char *path, char values[][32], bool unmap[], int count);
