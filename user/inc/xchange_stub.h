/** @file xchange_stub.h
 *
 * @brief A multi-threaded application which sometimes experiences
 * a thread crash.
 *
 *
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekun)
 **/

#ifndef __XCHANGE_STUB_H__
#define __XCHANGE_STUB_H__

/**
 * @brief Atomomically pushes val into the lock pointer and
 * return the original value that was in lock
 *
 * @param lock A pointer to the int value
 * @param val The value to be put into the pointer
 *
 * @return old value pointed to by lock
 */
int xchange(int *lock, int val);

#endif /* __XCHANGE_STUB_H__ */