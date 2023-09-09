/** @file console.c
 *  @brief A console driver.
 *
 *  This is the implementation of the console driver.
 *
 *  @author Tony Xi (xiaolix)
 *  @author Zekun Ma (zekunm)
 *  @bug No know bugs.
 */

#include <console.h> // putbyte
#include <stdbool.h> // bool
#include <video_defines.h> // CONSOLE_MEM_BASE
#include <asm.h> // outb
#include <limits.h> // CHAR_BIT
#include <simics.h> // lprintf
#include <stddef.h> // NULL

// the so-called byte pair in Pject 0 handout
typedef struct pixel_t {
    char ch;
    char color;
} pixel_t;

// all we need to know in order to place the cursor
typedef struct cursor_t {
    bool visibility;
    int row;
    int col;
} cursor_t;

// terminal settings
typedef struct terminal_t {
    // The initial color of the terminal, which is the default color.
    char initial_color;
    // The current color of the terminal color, which is set by the user.
    char current_color;
} terminal_t;

cursor_t cursor;
terminal_t terminal;

// Helper functions start.
// 1. These helper functions are pretty much enough for
//    implementing APIs exposed by console.
// 2. These helper functions are only for internal use. So they
//    don't check null pointer, position out of bound, etc.
// 3. These helper functions are designed to be stateless. In
//    other words, they should act upon their arguments, rather
//    than global variables.

/**
 * @brief test if a position is within the bound
 * 
 * @param row the row of the position
 * @param col the column of the position
 * @return true if and only if the position is within the bound
 */
bool validate_pos(int row, int col);

/**
 * @brief convert a position to a pixel index
 * 
 * @param row the row of the position
 * @param col the column of the position
 * @return Recall we refer to a byte pair as a pixel.
 *         The index of a byte pair into the byte pair array is a pixel index.
 */
int convert_to_pixel_idx(int row, int col);

/**
 * @brief convert a pixel index to a position
 * 
 * @param pixel_idx the pixel index
 * @param row_ptr where the row of the the position should go
 * @param col_ptr where the column of the position should go
 */
void convert_to_pos(int pixel_idx, int *row_ptr, int *col_ptr);

/**
 * @brief send a pixel index to CRTC
 * 
 * Call this function if you want to change the position of the cursor.
 * 
 * @param pixel_idx the pixel index
 */
void write_to_crtc(int pixel_idx);

/**
 * @brief read a pixel index from CRTC
 * 
 * Call this function if you want to know the position of the cursor.
 * 
 * @return the pixel index
 */
int read_from_crtc(void);

/**
 * @brief place the cursor
 * 
 * @param cursor_ptr a pointer to all we need to know
 *                   in order to place the cursor
 */
void place_cursor(cursor_t *cursor_ptr);

/**
 * @brief read a pixel from the conosle
 * 
 * @param row the row of the position of the pixel
 * @param col the column of the position of the pixel
 * @param pixel_ptr where the pixel should go
 */
void read_pixel(int row, int col, pixel_t *pixel_ptr);

/**
 * @brief write a pixel to the console
 * 
 * @param row the row of the position where the pixel should go
 * @param col the column of the position where the pixel should go
 * @param pixel_ptr a pointer to the pixel
 */
void write_pixel(int row, int col, pixel_t *pixel_ptr);

/**
 * @brief Scroll the console up but leave the cursor unmoved.
 * 
 * @param line_count How many lines the console is to be scrolled up.
 *                   When negative, this value implies scrolling the console down.
 * @param fill_color Which color to be filled into newly added lines
 */
void scroll_up(int line_count, char fill_color);

// Helper functions end.

void install_console(void) {
    int pixel_idx = read_from_crtc();
    convert_to_pos(pixel_idx, &cursor.row, &cursor.col);
    cursor.visibility = true;

    pixel_t pixel;
    read_pixel(cursor.row, cursor.col, &pixel);
    terminal.initial_color = pixel.color;
    terminal.current_color = pixel.color;

    lprintf("Th console has been installed.");
}

int putbyte(char ch) {
    draw_char(cursor.row, cursor.col, ch, terminal.current_color);
    return (unsigned char)ch;
}

void putbytes(const char *s, int len) {
    if (s == NULL || len <= 0) {
        return;
    }

    for (int i = 0; i < len; i++) {
        putbyte(s[i]);
    }
}

int set_term_color(int color) {
    if ((unsigned char)color != color) {
        return -1;
    }
    terminal.current_color = color;
    return 0;
}

void get_term_color(int *color) {
    if (color == NULL) {
        lprintf("color in get_term_color is NULL.");
    }

    *color = (unsigned char)terminal.current_color;
}

int set_cursor(int row, int col) {
    if (!validate_pos(row, col)) {
        return -1;
    }
    
    cursor.row = row;
    cursor.col = col;
    place_cursor(&cursor);
    return 0;
}

void get_cursor(int *row, int *col) {
    if (row == NULL || col == NULL) {
        lprintf("row or col in get_cursor is NULL.");
    }

    *row = cursor.row;
    *col = cursor.col;
}

void hide_cursor(void) {
    cursor.visibility = false;
    place_cursor(&cursor);
}

void show_cursor(void) {
    cursor.visibility = true;
    place_cursor(&cursor);
}

void clear_console(void) {
    scroll_up(CONSOLE_HEIGHT, terminal.initial_color);
    cursor.row = 0;
    cursor.col = 0;
    place_cursor(&cursor);
}

void draw_char(int row, int col, int ch, int color) {
    if (!validate_pos(row, col)) {
        return;
    }
    if ((unsigned char)ch != ch || (unsigned char)color != color) {
        return;
    }

    switch (ch) {
        case '\n': {
            cursor.row = row + 1;
            cursor.col = 0;
            if (!validate_pos(cursor.row, cursor.col)) {
                cursor.row--;
                scroll_up(1, terminal.initial_color);
            }
            place_cursor(&cursor);
            break;
        }

        case '\r': {
            cursor.row = row;
            cursor.col = 0;
            place_cursor(&cursor);
            break;
        }

        case '\b': {
            convert_to_pos(
                convert_to_pixel_idx(row, col) - 1,
                &row,
                &col
            );
            if (!validate_pos(row, col)) {
                cursor.row = row + 1;
                cursor.col = 0;
            } else {
                pixel_t pixel = {
                    .ch = ' ',
                    .color = terminal.initial_color
                };
                write_pixel(row, col, &pixel);

                cursor.row = row;
                cursor.col = col;
            }
            place_cursor(&cursor);
            break;
        }

        default: {
            pixel_t pixel = {
                .ch = ch,
                .color = color
            };
            write_pixel(row, col, &pixel);
            
            convert_to_pos(convert_to_pixel_idx(row, col) + 1, &cursor.row, &cursor.col);
            if (!validate_pos(cursor.row, cursor.col)) {
                scroll_up(1, terminal.initial_color);
                cursor.row--;
            }
            place_cursor(&cursor);
            break;
        }
    }
}

char get_char(int row, int col) {
    if (!validate_pos(row, col)) {
        lprintf("row and col in get_char are out of bound.");
        return -1;
    }

    pixel_t pixel;
    read_pixel(row, col, &pixel);
    return pixel.ch;
}

bool validate_pos(int row, int col) {
    return 0 <= row && row < CONSOLE_HEIGHT && 0 <= col && col < CONSOLE_WIDTH;
}

int convert_to_pixel_idx(int row, int col) {
    return row * CONSOLE_WIDTH + col;
}

void convert_to_pos(int pixel_idx, int *row_ptr, int *col_ptr) {
    *row_ptr = pixel_idx / CONSOLE_WIDTH;
    *col_ptr = pixel_idx % CONSOLE_WIDTH;
}

void write_to_crtc(int pixel_idx) {
    unsigned int data = pixel_idx;
    outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
    outb(CRTC_DATA_REG, data);

    data = data >> CHAR_BIT;
    outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
    outb(CRTC_DATA_REG, data);
}

int read_from_crtc(void) {
    unsigned int data = 0;
    outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
    data = data | inb(CRTC_DATA_REG);

    data <<= CHAR_BIT;
    outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
    data = data | inb(CRTC_DATA_REG);

    return data;
}

void place_cursor(cursor_t *cursor_ptr) {
    int pixel_idx = cursor_ptr->visibility ?
        convert_to_pixel_idx(cursor_ptr->row, cursor_ptr->col) :
        convert_to_pixel_idx(CONSOLE_HEIGHT, CONSOLE_WIDTH);
    write_to_crtc(pixel_idx);
}

void read_pixel(int row, int col, pixel_t *pixel_ptr) {
    pixel_t *pixel_array = (pixel_t *)CONSOLE_MEM_BASE;
    int pixel_idx = convert_to_pixel_idx(row, col);
    *pixel_ptr = pixel_array[pixel_idx];
}

void write_pixel(int row, int col, pixel_t *pixel_ptr) {
    pixel_t *pixel_array = (pixel_t *)CONSOLE_MEM_BASE;
    int pixel_idx = convert_to_pixel_idx(row, col);
    pixel_array[pixel_idx] = *pixel_ptr;
}

void scroll_up(int line_count, char fill_color) {
    if (line_count <= 0) {
        return;
    }

    for (int i = 0; i < CONSOLE_HEIGHT - line_count; i++) {
        for (int j = 0; j < CONSOLE_WIDTH; j++) {
            pixel_t pixel;
            read_pixel(i + line_count, j, &pixel);
            write_pixel(i, j, &pixel);
        }
    }

    for (int i = CONSOLE_HEIGHT - line_count; i < CONSOLE_HEIGHT; i++) {
        for (int j = 0; j < CONSOLE_WIDTH; j++) {
            pixel_t pixel = {
                .ch = ' ',
                .color = fill_color,
            };
            write_pixel(i, j, &pixel);
        }
    }
}
