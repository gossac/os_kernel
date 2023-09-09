/**
 * @file xchange_stub.h
 *
 * @brief The stub for the xchg assembly instruction.
 *
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @bug No known bugs.
 **/

#ifndef __XCHANGE_STUB_H__
#define __XCHANGE_STUB_H__

/**
 * @brief Atomically put val into the lock and
 *        return the old value that was in the
 *        lock.
 *
 * @param lock The lock.
 * @param val The value to be put into the lock.
 *
 * @return Old value in the lock.
 */
int xchange(int *lock, int val);

#endif /* __XCHANGE_STUB_H__ */