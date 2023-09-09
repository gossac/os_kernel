/**
 * @file keyboard.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief the keyboard driver
 */

#ifndef KEYBOARD_H_SEEN
#define KEYBOARD_H_SEEN

// Buffer length of type buf_t, which for now is used as the type
// of both the scancode buffer and the extracted character buffer.
#define BUF_LEN (1024)

/**
 * @brief Char sized element buffer type. It can act as a queue
 *        as well as a stack.
 */
typedef struct buf_t {
    int start_idx;
    int element_count;
    char array[BUF_LEN];
} buf_t;

buf_t scancode_buf;

void install_keyboard(void);
int extract_ch(buf_t *scancode_buf_ptr, char *ch_ptr);
int push_tail(buf_t *buf_ptr, char element);
int pop_head(buf_t *buf_ptr, char *element_ptr);
int pop_tail(buf_t *buf_ptr, char *element_ptr);

#endif /* KEYBOARD_H_SEEN */
