/* ============================================
 * PAGING SYSTEM
 * Virtual memory management with protection
 * ============================================ */

#include "memory.h"
#include "../kernel.h"

/* Forward declaration */
extern void* kmalloc(u32 size, u8 flags);

/* Page directory and table structures */
#define PAGE_SIZE 4096
#define PAGE_DIRECTORY_INDEX(x) ((x) >> 22)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)
#define PAGE_OFFSET(x) ((x) & 0xFFF)

/* Page flags */
#define PAGE_PRESENT  0x001
#define PAGE_WRITE    0x002
#define PAGE_USER     0x004
#define PAGE_PWT      0x008
#define PAGE_PCD      0x010
#define PAGE_ACCESSED 0x020
#define PAGE_DIRTY    0x040
#define PAGE_SIZE_4M  0x080
#define PAGE_GLOBAL   0x100

/* Page directory and tables */
u32 page_directory[1024] __attribute__((aligned(4096)));
u32 page_table[1024] __attribute__((aligned(4096)));

/* ============================================
 * Initialize Paging
 * ============================================ */
void init_paging(void)
{
    /* Clear page directory */
    for (u32 i = 0; i < 1024; i++) {
        page_directory[i] = 0;
    }
    
    /* Identity map first 4MB */
    for (u32 i = 0; i < 1024; i++) {
        page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    }
    
    /* Set page directory entry */
    page_directory[0] = ((u32)page_table) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    
    /* Fill rest of page directory with 0 */
    for (u32 i = 1; i < 1024; i++) {
        page_directory[i] = 0;
    }
    
    /* Load page directory */
    asm volatile("mov %0, %%cr3" :: "r"(page_directory));
    
    /* Enable paging */
    u32 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  /* Set PG bit */
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}

/* ============================================
 * Map Page
 * ============================================ */
void map_page(u32 virtual_addr, u32 physical_addr, u32 flags)
{
    u32 pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
    u32 pt_index = PAGE_TABLE_INDEX(virtual_addr);
    
    /* Check if page table exists */
    if (!(page_directory[pd_index] & PAGE_PRESENT)) {
        /* Allocate new page table */
        u32* new_table = (u32*)kmalloc(4096, MEM_ZERO);
        page_directory[pd_index] = ((u32)new_table) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }
    
    /* Get page table */
    u32* page_table = (u32*)(page_directory[pd_index] & 0xFFFFF000);
    
    /* Map page */
    page_table[pt_index] = physical_addr | flags;
    
    /* Invalidate TLB */
    asm volatile("invlpg (%0)" :: "r"(virtual_addr));
}

/* ============================================
 * Unmap Page
 * ============================================ */
void unmap_page(u32 virtual_addr)
{
    u32 pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
    u32 pt_index = PAGE_TABLE_INDEX(virtual_addr);
    
    if (page_directory[pd_index] & PAGE_PRESENT) {
        u32* page_table = (u32*)(page_directory[pd_index] & 0xFFFFF000);
        page_table[pt_index] = 0;
        
        /* Invalidate TLB */
        asm volatile("invlpg (%0)" :: "r"(virtual_addr));
    }
}

