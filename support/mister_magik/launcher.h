#pragma once

bool mister_magik_launcher_configured(void);
bool mister_magik_launcher_active(void);
bool mister_magik_launcher_input_proxy_active(void);
int mister_magik_launcher_command_fd(void);
bool mister_magik_launcher_main_framebuffer_suppressed(void);
void mister_magik_launcher_begin_boot_lockdown(void);
void mister_magik_launcher_route_early_black(void);
void mister_magik_launcher_enter_after_menu_init(void);
void mister_magik_launcher_poll(void);
bool mister_magik_launcher_idle_waits(void);
void mister_magik_launcher_wait_for_activity(void);
bool mister_magik_launcher_maybe_load_latch_menu(const char *path);
void mister_magik_status_write(void);
void mister_magik_command_reply(const char *result);
void mister_magik_reply_channel_init(void);
void mister_magik_record_invariant(const char *kind, const char *detail);
