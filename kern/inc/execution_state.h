/**
 * @file execution_state.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief ureg loader
 */

#ifndef EXECUTABLE_H_SEEN
#define EXECUTABLE_H_SEEN

#include <ureg.h> // ureg_t

/**
 * @brief load ureg into registers
 * 
 * @param ureg_ptr pointer to a ureg variable
 */
void load_ureg(ureg_t *ureg_ptr);

#endif // EXECUTABLE_H_SEEN
