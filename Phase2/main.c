// SPEDE inclues
#include "k-include.h"
// Entries to kernal (TimerEntry, etc)
#include "k-entry.h"
// Kernel data types
#include "k-type.h"
// small handy functions
#include "k-lib.h"
// kernal service routines
#include "k-sr.h"
// all user process code here
#include "proc.h"
// all sys calls
#include "sys-call.h"

// kernal data are all here:
// current running PID; if -1, then none selected
int run_pid;
q_t pid_q, ready_q, sleep_q;
// Process Control Blocks
pcb_t pcb[PROC_SIZE];
// process runtime stacks
char proc_stack[PROC_SIZE][PROC_STACK_SIZE];
// intr table's DRAM location
struct i386_gate * intr_table;
// system time in centi-sec, intialize to 0
int sys_centi_sec = 0;

// init kernal data
void InitKernelData(void) {
    int i;

    // get intr table location
    intr_table = get_idt_base();

    // clear 2 queues
    Bzero((char *)&pid_q, sizeof(q_t));
    Bzero((char *)&ready_q, sizeof(q_t));
    Bzero((char *)&sleep_q, sizeof(q_t));

    // put all PID's to pid queue
    for(i = 0; i < 20; i++) {
        EnQ(i, &pid_q);
    }

    // set run_pid to NONE
    run_pid = NONE;
}

// init kernal control
void InitKernelControl(void) {
    // fill out intr table for timer
    // fill_gate();
    fill_gate(&intr_table[TIMER_INTR], 
            (int)TimerEntry, get_cs(), 
            ACC_INTR_GATE, 
            0);

    fill_gate(&intr_table[GETPID_CALL], 
            (int)GetPidEntry, get_cs(), 
            ACC_INTR_GATE, 
            0);

    fill_gate(&intr_table[SHOWCHAR_CALL], 
            (int)ShowCharEntry, get_cs(), 
            ACC_INTR_GATE, 
            0);

    fill_gate(&intr_table[SLEEP_CALL], 
            (int)SleepEntry, get_cs(), 
            ACC_INTR_GATE, 
            0);
    // mask out PIC for timer
    // outportb();

    outportb(PIC_MASK, MASK);
}

// Choose run_pid
void Scheduler(void) {
    // Ok/picked
    if(run_pid > 0) {
        return;
    }

    // if ready_q is empty:
    // pick 0 as run_pid; pick InitProc
    // else
    // change state of PID 0 to ready
    // dequeue ready_q to set run_pid
    if(QisEmpty(&ready_q)) {
        // Process 0 to ready queue?
        run_pid = 0;
    } else {
        // Not sure if this 
        // pcb[0].state = READY;
        pcb[0].state = READY;
        run_pid = DeQ(&ready_q);
        // cons_printf("Scheduler: %d\n", run_pid);
    }

    // reset run_count of selected proc
    // upgrade its state to run
    pcb[run_pid].run_count = 0;
    pcb[run_pid].state = RUN;
}

int main(void) {
    // call to initialize kernal data
    InitKernelData();
    // call to initialize kernal control
    InitKernelControl();

    // call NewProcSR(InitProc) to create it 
    // create InitProc
    NewProcSR(InitProc);
    // call Scheduler()
    Scheduler();
    // call Loader(pcb[run_pid].trapframe_p); 
    // load/run it
    Loader(pcb[run_pid].trapframe_p);

    // Statement never reached, compiler asks it to for syntax
    return 0;
}

void Kernel(trapframe_t * trapframe_p) {
    char ch;

    // save it
    pcb[run_pid].trapframe_p = trapframe_p; 


    // handle timer intr
    switch (trapframe_p->entry_id) {
        case TIMER_INTR:
            TimerSR();
            break;
        case GETPID_CALL:
            // trapframe_p->reg[7] = GetPidSR();
            trapframe_p->reg[7] = run_pid;
            break;
        case SHOWCHAR_CALL:
            ShowCharSR(trapframe_p->reg[7], trapframe_p->reg[4], trapframe_p->reg[6]);
            break;
        case SLEEP_CALL:
            SleepSR(trapframe_p->reg[7]);
            break;
        default:
            cons_printf("Panic!: Should never be here\n");
    }

    // check if keyboard pressed
    if(cons_kbhit()) {
        ch = cons_getchar();
        // 'b' for breakpoint
        if(ch == 'b') {
            //let's g o GDB
            breakpoint();
        } 
        // 'n' for new process
        else if(ch == 'n') {
            // create a UserProc
            NewProcSR(UserProc);
        }
    }

    // May need t pick another proc
    Scheduler();
    Loader(pcb[run_pid].trapframe_p);
}
