// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "spinlock.h"

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;

  /*
  P5 changes
  */
  uint free_pages; //track free pages
  uint ref_cnt[PHYSTOP / PGSIZE]; //track reference count

} kmem;

extern char end[]; // first address after kernel loaded from ELF file

int 
getFreePages_h(void) 
{
  int free;
  acquire(&kmem.lock);
  free = kmem.free_pages;
  release(&kmem.lock);
  return free;
}

int 
getRefCount_h(uint addr) 
{
  int count;
  acquire(&kmem.lock);
  count = kmem.ref_cnt[addr/PGSIZE];
  release(&kmem.lock);
  return count;
}

// Initialize free list of physical pages.
void
kinit(void)
{
  char *p;

  initlock(&kmem.lock, "kmem");

  p = (char*)PGROUNDUP((uint)end);
  for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE) {
    kfree(p);
  }
  //kmem.free_pages = PHYSTOP / PGSIZE;
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || (uint)v >= PHYSTOP) 
    panic("kfree");

  acquire(&kmem.lock);
  r = (struct run*)v;
  if (kmem.ref_cnt[(uint)r/PGSIZE] <= 1) {
    // Fill with junk to catch dangling refs.
    memset(v, 1, PGSIZE);
    r = (struct run*)v;

    r->next = kmem.freelist;
    kmem.free_pages++;
    kmem.freelist = r;
  }
  else {
    kmem.ref_cnt[(uint)r/PGSIZE]--;
  }

  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    kmem.ref_cnt[(uint)r/PGSIZE] = 1;
    kmem.free_pages--;
  }
  release(&kmem.lock);
  return (char*)r;
}

void
ref_cnt_inc(uint addr){
  acquire(&kmem.lock);
  kmem.ref_cnt[addr/PGSIZE]++;
  release(&kmem.lock);
}

void
ref_cnt_dec(uint addr){
  acquire(&kmem.lock);
  kmem.ref_cnt[addr/PGSIZE]--;
  release(&kmem.lock);
}