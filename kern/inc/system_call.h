/**
 * @file system_call.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief system call handlers and initializer
 */

#ifndef SYSTEM_CALL_H_SEEN
#define SYSTEM_CALL_H_SEEN

#include <ureg.h> // ureg_t
#include <mutex.h> // mutex_t
#include <stdint.h> // uint32_t
#include <stdbool.h> // bool

mutex_t output_lock;

int initialize_system_call(void);

void handle_gettid(ureg_t *ureg_ptr);
void handle_fork(ureg_t *ureg_ptr);
void handle_exec(ureg_t *ureg_ptr);
void handle_halt(ureg_t *ureg_ptr);
void handle_wait(ureg_t *ureg_ptr);
void handle_vanish(ureg_t *ureg_ptr);
void handle_task_vanish(ureg_t *ureg_ptr);
void handle_set_status(ureg_t *ureg_ptr);
void handle_yield(ureg_t *ureg_ptr);
void handle_deschedule(ureg_t *ureg_ptr);
void handle_make_runnable(ureg_t *ureg_ptr);
void handle_print(ureg_t *ureg_ptr);
void handle_readline(ureg_t *ureg_ptr);
void handle_getchar(ureg_t *ureg_ptr);
void handle_new_pages(ureg_t *ureg_ptr);
void handle_remove_pages(ureg_t *ureg_ptr);
void handle_swexn(ureg_t *ureg_ptr);
void handle_sleep(ureg_t *ureg_ptr);
void handle_readfile(ureg_t *ureg_ptr);
void handle_thread_fork(ureg_t *ureg_ptr);
void handle_get_ticks(ureg_t *ureg_ptr);
void handle_set_term_color(ureg_t *ureg_ptr);
void handle_set_cursor_pos(ureg_t *ureg_ptr);
void handle_get_cursor_pos(ureg_t *ureg_ptr);
void handle_new_console(ureg_t *ureg_ptr);

bool check_eflags(uint32_t old_eflags, uint32_t new_eflags);

#endif /* SYSTEM_CALL_H_SEEN */
