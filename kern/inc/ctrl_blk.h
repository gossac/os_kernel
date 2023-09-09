/**
 * @file ctrl_blk.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief data structures of TCB and PCB and operations on them
 */

#ifndef CTRL_BLK_H_SEEN
#define CTRL_BLK_H_SEEN

#include <stdint.h>
#include <list.h>
#include <vm.h>
#include <ureg.h>
#include <mutex.h>
#include <cr.h>
#include <stdbool.h>
#include <hvcall.h>

// number of entries in a virtual IDT
#define VIRTUAL_IDT_LEN (HV_KEYBOARD + 1)

// Do not change KERNEL_STACK_LEN. The offset
// is hard coded into context.S
#define KERNEL_STACK_LEN (0x1000)

// states of threads
#define NEW_STATE (0)
#define READY_STATE (NEW_STATE + 1)
#define RUNNING_STATE (READY_STATE + 1)
#define WAITING_STATE (RUNNING_STATE + 1)
#define TERMINATED_STATE (WAITING_STATE + 1)
// blocking reasons
#define SLEEP (TERMINATED_STATE + 1)
#define READLINE (SLEEP + 1)
#define DESCHEDULE (READLINE + 1)
#define VANISH_WAIT (DESCHEDULE + 1)

#define THREAD_LIST_COUNT (VANISH_WAIT + 1)

struct pcb_t;
typedef struct blocking_detail_t {
    int reason;
    union {
        unsigned int wakeup_time;
        bool first_reader;
    };
} blocking_detail_t;

typedef struct page_allocation_t {
    void *base;
    int len;
} page_allocation_t;

// Used only if the pcb is a guest
typedef struct guest_resource_t {
    bool interrupt_enable_flag;
    uint32_t esp0;
    uint32_t virtual_idt[VIRTUAL_IDT_LEN];
} guest_resource_t;

struct pcb_t;
typedef struct tcb_t {
    // the process owning this thread
    struct pcb_t *pcb_ptr;
    int tid;
    int state;

    // Do not move esp and kernel_stack. The offsets of them are
    // hard coded into context.S.
    uint32_t esp;
    uint32_t kernel_stack[KERNEL_STACK_LEN];

    // (exception != NULL) means swexn has registered a handler,
    // only in which case the three fields below are ready for use.
    void *exception_stack;
    void (*handler)(void *arg, ureg_t *ureg_ptr);
    void *arg;

    blocking_detail_t blocking_detail;
} tcb_t;

DEFINE_NODE_T(tcb_node_t, tcb_t);
DEFINE_NODE_T(page_allocation_node_t, page_allocation_t);
struct pcb_node_t;
typedef struct pcb_t {
    // parent process, NULL for root process
    struct pcb_t *parent_pcb_ptr;
    // child processes
    struct pcb_node_t *child_pcb_list;
    // all the threads of this process
    tcb_node_t *tcb_list;
    int status;

    // Do not move page_directory. The offset of it is
    // hard coded into context.S.
    pde_t *page_directory;
    page_allocation_node_t *page_allocation_list;

    mutex_t lock;

    // boolean indecates if the current process is a guest
    bool guest;
    guest_resource_t guest_resource;
} pcb_t;
DEFINE_NODE_T(pcb_node_t, pcb_t);

DEFINE_NODE_T(tcb_ptr_node_t, tcb_t *);

tcb_ptr_node_t *thread_lists[THREAD_LIST_COUNT];
pcb_node_t *root_pcb_node_ptr;

int init_ctrl_blk(void);
int fork_ctrl_blk(ureg_t *ureg_ptr);
int thread_fork_ctrl_blk(ureg_t *ureg_ptr);
int alter_state(
    tcb_t *tcb_ptr,
    int state,
    blocking_detail_t *blocking_detail_ptr
);
int get_thread_alive_count(pcb_t *pcb_ptr, int *thread_alive_count_ptr);
tcb_t *find_tcb_in_list(void *value, int state);
pcb_node_t *find_exited_child(pcb_t *parent_pcb);
int unlock_children(pcb_t *pcb, pcb_node_t *node);

#endif // CTRL_BLK_H_SEEN
