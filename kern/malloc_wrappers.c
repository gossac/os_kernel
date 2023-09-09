/**
 * @file malloc_wrappers.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief thread safe malloc wrappers
 */

#include <stddef.h>
#include <malloc.h>
#include <malloc_internal.h>
#include <mem_allocation.h>
#include <mutex.h>

/* safe versions of malloc functions */

void *malloc(size_t size)
{
  mutex_lock(&mem_allocation_lock);
  void *mem = _malloc(size);
  mutex_unlock(&mem_allocation_lock);
  return mem;
}

void *memalign(size_t alignment, size_t size)
{
  mutex_lock(&mem_allocation_lock);
  void *mem = _memalign(alignment, size);
  mutex_unlock(&mem_allocation_lock);
  return mem;
}

void *calloc(size_t nelt, size_t eltsize)
{
  mutex_lock(&mem_allocation_lock);
  void *mem = _calloc(nelt, eltsize);
  mutex_unlock(&mem_allocation_lock);
  return mem;
}

void *realloc(void *buf, size_t new_size)
{
  mutex_lock(&mem_allocation_lock);
  void *mem = _realloc(buf, new_size);
  mutex_unlock(&mem_allocation_lock);
  return mem;
}

void free(void *buf)
{
  mutex_lock(&mem_allocation_lock);
  _free(buf);
  mutex_unlock(&mem_allocation_lock);
}

void *smalloc(size_t size)
{
  mutex_lock(&mem_allocation_lock);
  void *mem = _smalloc(size);
  mutex_unlock(&mem_allocation_lock);
  return mem;
}

void *smemalign(size_t alignment, size_t size)
{
  mutex_lock(&mem_allocation_lock);
  void *mem = _smemalign(alignment, size);
  mutex_unlock(&mem_allocation_lock);
  return mem;
}

void sfree(void *buf, size_t size)
{
  mutex_lock(&mem_allocation_lock);
  _sfree(buf, size);
  mutex_unlock(&mem_allocation_lock);
}
