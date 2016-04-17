#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pthread.h"


//free in kernel
typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};
typedef union header Header;

static Header *freep;

void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}
//end of free in kernel

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }







  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;



  int j =0;
  initlock(&(np->mt.lock),proc->name);
  for(;j<NUM_MUTEX;j++){
    char alphaName[2] = {(char) (j + 65), '\0'};
    initlock(&(np->mt.mutex_arr[j].lock),alphaName);
  }
  np->family = pid;
  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{

  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;


    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);
  // kill threads and clean them up

  struct proc *t;

  for(t=ptable.proc; t<&ptable.proc[NPROC];t++){
    if(t->family == proc->pid&&t->pid!=proc->pid){
      t->killed=1;
      if(t->state == SLEEPING){
        t->state=RUNNABLE;
      }
      /*
      if(t->wemalloc){

        free(t->stack);
      }*/
    }
  }

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");


}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  //cprintf("YIELD BEFORE: %s\n", proc->name);
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
  //cprintf("YIELD AFTER: %s\n", proc->name);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    initlog();
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1

    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      struct proc *t;
      for(t=ptable.proc; t<&ptable.proc[NPROC];t++){
        if(t->family == p->pid&& t->pid!=p->pid){
          t->killed=1;
          if(t->state == SLEEPING){
            t->state=RUNNABLE;
          }
          /*
          if(t->wemalloc){
            free(t->stack);
          }*/
        }
      }


      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int clone(void*(*func) (void*), void* arg, void* stack)
{
  //procdump();
  struct proc *t;
  int pid, i;

  //allocated process element
  if((t = allocproc()) == 0)
    return -1;

  //share vm
  t->pgdir = proc->pgdir;
  /*if((t->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(t->kstack);
    t->kstack = 0;
    t->state = UNUSED;
    return -1;
  }*/
  t->sz = proc->sz;
  *t->tf = *proc->tf;

  // Increment file reference count
  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      t->ofile[i] = filedup(proc->ofile[i]);
  t->cwd = idup(proc->cwd);

  //set the parent
  t->parent = proc;
  if(proc->family==0){
    proc->family=proc->pid;
  }
  t->family = proc->family;

  t->stack = stack;
  //free(t->stack); // copy stack
  //cprintf("FREED!!\n");
  //make the stack
  t->tf->esp = (uint)(stack+STACK_SIZE);
  t->tf->ebp = t->tf->esp;
  t->tf->esp -= sizeof(uint);
  *((uint*)(t->tf->esp)) = (uint)arg;
  t->tf->esp -= sizeof(uint);
  *((uint*)(t->tf->esp)) = 0xFFFFFFFF;
  t->tf->eip = (uint)func;
  pid = t->pid;

  char alphaName[2] = {(char) (t->pid % 26 + 65), '\0'};
  safestrcpy(t->name, alphaName, sizeof(alphaName));

  acquire(&ptable.lock);
  t->state = RUNNABLE;
  //t->state = SLEEPING;
  release(&ptable.lock);


  return pid;
}

void wemalloc(int tid){
  acquire(&ptable.lock);
  struct proc *t;
  for(t = ptable.proc; t < &ptable.proc[NPROC]; t++) {
      if(t->pid==tid){
        t->wemalloc=1;
        break;
      }
  }
  release(&ptable.lock);
}


int join(int pid, void** stack, void**retval)
{
  struct proc *p;
  int havekid;

  if(pid <= 0 || proc->pid == pid) // invalid pid
    return -1;

  acquire(&ptable.lock);
  for(;;) {
    havekid = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if(p->parent != proc)
        continue;
      if(p->pid != pid) // not the requested thread
        continue;


      havekid = 1;
      if(p->state == ZOMBIE) {
        // Found one.
        *stack = (void *)p->stack;
        *retval = (void *)p->retval;
        /*
        if(p->wemalloc){
          cprintf("WE MALLOC FREE");
          free(p->stack);
        }*/
        p->stack = 0;
        p->retval = 0;
        kfree(p->kstack);
        p->kstack = 0;
        p->pgdir = 0; // just make pointer NULL
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return 0;
      }
    }

    // thread requested doesn't exist
    if(!havekid || proc->killed) {
      release(&ptable.lock);
      return -1;
    }

    // Waiting for children
    sleep(proc, &ptable.lock);
  }
}
void texit(void* retval)
{

  proc->retval = retval;
  exit();
}


int
mutex_init(void){

  //acquire(&(ptable.lock));
  struct proc* parent = &ptable.proc[proc->family-1];

  acquire(&(parent->mt.lock));
  int i=0;
  for(;i<NUM_MUTEX; i++){
    if(!parent->mt.mutex_arr[i].valid){

      parent->mt.mutex_arr[i].valid = 1;
      parent->mt.mutex_arr[i].status =0;
      parent->mt.mutex_arr[i].holder = -1;
      release(&(parent->mt.lock));
      //release(&(ptable.lock));
      return i;
    }
  }
  release(&(parent->mt.lock));
  //release(&(ptable.lock));
  return -1;
}

int
mutex_lock(int mutid){

    struct proc* parent = &ptable.proc[proc->family-1];
    //cprintf("ST%d\n",parent->mt.mutex_arr[mutid].status);
    //pushcli();
    //acquire(&(parent->mt.lock));
    if(parent->mt.mutex_arr[mutid].valid){
      //acquire(&parent->mt.lock);

      //if(!parent->mt.mutex_arr[mutid].status){
        //  cprintf("ATTEMPT\n");
        //  acquire(&parent->mt.mutex_arr[mutid].lock);
          //cprintf("ACQ\n");m
      //}
      //else{
          cprintf("SLEEP\n");
          //release(&parent->mt.lock);
          acquire(&parent->mt.mutex_arr[mutid].lock);
          while(parent->mt.mutex_arr[mutid].status){
            //cprintf("SLEEPING\n");
            //popcli();
            sleep(&(parent->mt.mutex_arr[mutid]),&(parent->mt.mutex_arr[mutid].lock));
          }
          //cprintf("MADE IT\n");

//s      }
        acquire(&parent->mt.lock);
        parent->mt.mutex_arr[mutid].status =1;
        parent->mt.mutex_arr[mutid].holder = proc->pid;
        release(&(parent->mt.lock));

      //popcli();
      return 0;
    }
    else{
      //popcli();
      //release(&(parent->mt.lock));
      return -1;
    }

}

int
mutex_destroy(int mutid){
  cprintf("DESTROY\n");
  struct proc* parent = &ptable.proc[proc->family-1];

  //acquire(&(parent->mt.lock));
  if(mutid<0 || mutid>=NUM_MUTEX || parent->mt.mutex_arr[mutid].status || !parent->mt.mutex_arr[mutid].valid){

    return -1;
  }
  acquire(&(parent->mt.lock));
  parent->mt.mutex_arr[mutid].status =0;
  parent->mt.mutex_arr[mutid].valid = 0;
  parent->mt.mutex_arr[mutid].holder= -1;
  release(&(parent->mt.lock));


  return 0;
}

int
mutex_unlock(int mutid){

  struct proc* parent = &ptable.proc[proc->family-1];
  cprintf("WWWWWWTTTFFF\n");
  //acquire(&(parent->mt.lock));;
  //if mutex is valid and this thread is holding it and the mutex is actually locked
  if(parent->mt.mutex_arr[mutid].valid==1 && (parent->mt.mutex_arr[mutid].holder == proc->pid)&&(parent->mt.mutex_arr[mutid].status==1)){
    release(&parent->mt.mutex_arr[mutid].lock);
    acquire(&(parent->mt.lock));
    parent->mt.mutex_arr[mutid].status =0;
    parent->mt.mutex_arr[mutid].holder = -1;
    release(&(parent->mt.lock));

    wakeup(&parent->mt.mutex_arr[mutid]);
    cprintf("RELEASED\n");


    //cprintf("RELEASED!\n");

    return 0;
  }
  else{
    //release(&(parent->mt.lock));
    return -1;
  }
}
