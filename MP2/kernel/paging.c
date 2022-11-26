#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "proc.h"

/* NTU OS 2022 */
/* Page fault handler */
int handle_pgfault() {
  printf("======trigger page fault======\n");
  uint64 va = r_stval();// va that cause the page fault
  struct proc *p = myproc();
  if((va>=MAXVA) || (va>=p->sz) || (va<p->trapframe->sp)) return -1;
  //pte_t *new_pte = walk(p->pagetable, va, 1);
  char* pa = kalloc();
  if(pa == 0)
    return -1;
  memset(pa, 0, PGSIZE);
  if(mappages(p->pagetable, va, PGSIZE, (uint64)pa, PTE_U|PTE_R|PTE_W|PTE_X) != 0){
    kfree(pa);
    return -1;
  }
  printf("page handler: %s\n", p->name);
  return 0;
}