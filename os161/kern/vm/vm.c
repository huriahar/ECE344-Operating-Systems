#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <synch.h>
#include <elf.h>
#include <kern/unistd.h>
#include <vfs.h>
#include <vnode.h>
#include <kern/stat.h>
#include <uio.h>
#include <clock.h>

int vm_bootstrap_done = 0;

struct cmap_entry *cmap;
struct lock* access_cmap;
unsigned long cmap_size;
unsigned long page_count;
paddr_t cmap_start_physaddr;

struct vnode *swap_file;
struct smap_entry *smap;
unsigned long smap_page_count;

int kernel_pages, user_pages;

// --------------------- in RAM---------------------------
// [smap_start_physaddr, smap_start_physaddr + smap_size] --> smap
// [cmap_start_physaddr, cmap_start_physaddr + cmap_size) --> coremap
// [cmap_start_physaddr + cmap_size,  last_physaddr] --> system's free memory

// -----------In the coremap-----------------------------
// [0			 , cmapsize) --> fixed  - cmap
// [cmapsize, last_physaddr] --> freed

void vm_bootstrap(void)
{
//	unsigned long i;
	kernel_pages = 0;
	user_pages = 0;
	//--------------------------------------- smap ----------------------------------------------------
	unsigned long smap_size;
	paddr_t smap_start_physaddr;
	unsigned long i;

	char file_name[9];
	strcpy(file_name,"lhd0raw:");
	//strcpy(file_name,"swapfile:");
	
	int ret = vfs_open(file_name, O_RDWR, &swap_file);
	//int ret = vfs_open(file_name, O_RDWR|O_CREAT|O_TRUNC, &swap_file);
	assert(ret == 0);

	struct stat swap_status;
	VOP_STAT(swap_file, &swap_status);

	smap_page_count = swap_status.st_size / PAGE_SIZE;
	smap_size = smap_page_count * sizeof(struct smap_entry);
	smap_size = DIVROUNDUP(smap_size, PAGE_SIZE);
	smap_start_physaddr = ram_stealmem(smap_size);
	smap = (struct smap_entry *) PADDR_TO_KVADDR(smap_start_physaddr);

	for(i = 0; i < smap_page_count; i++){
		smap[i].disk_pa = (i*PAGE_SIZE);
		smap[i].state = empty;
		smap[i].as = NULL;
		smap[i].va = 0;
	}

	smap_pages_avail = smap_page_count;

	//--------------------------------------- coremap ----------------------------------------------------
	paddr_t first_physaddr, last_physaddr;

	ram_getsize(&first_physaddr, &last_physaddr);	

	page_count = (last_physaddr - first_physaddr) / PAGE_SIZE; 			//number of pages the physical memory can store
	cmap_size = page_count * sizeof(struct cmap_entry);					//need to allocate page_count entries in the cmap in order to keep track of each entry
	cmap_size = DIVROUNDUP(cmap_size, PAGE_SIZE);						//round the cmap size to the nearest page
	cmap_start_physaddr = ram_stealmem(cmap_size);						//pass number of pages we want to "steal". stolen pages won't be freed
	cmap = (struct cmap_entry*) PADDR_TO_KVADDR(cmap_start_physaddr);	//allocate the cmap in the first part of the physical memory

	for(i = 0; i < page_count; i++){
		if(i < cmap_size){
			cmap[i].state = fixed;
		}
		else{
			cmap[i].state = freed;
			cmap[i].num_pages = -1;
			cmap[i].first_page = 0;
		}
		cmap[i].pa = cmap_start_physaddr + (i*PAGE_SIZE);
		cmap[i].as = NULL;
		cmap[i].s = 0;
		cmap[i].ns = 0;
	}

	pages_avail = page_count - cmap_size;

	//access_cmap = lock_create("access_cmap");

	ram_reset();

	vm_bootstrap_done = 1;
}

void free_all_pages(struct addrspace *as){
	unsigned long i;
	for(i = 0; i < page_count; i++){
		if(cmap[i].as == as)
			free_kpages(PADDR_TO_KVADDR(cmap[i].pa));
	}
}

/*
	return a pointer to the beginning of the physical address space allocated
	allocating continous pages
*/

paddr_t getppages(unsigned long npages, page_state_t pstate, struct addrspace *as)
{
	paddr_t ret_addr = 0;	//alloc_upages excepts a 0 upon failure
	if(vm_bootstrap_done == 0){
		//if need to kmalloc before coremap is setup, then we're calling ram_stealmem
		int spl = splhigh();
		ret_addr = ram_stealmem(npages);
		splx(spl);
	}
	else{
		//just allocate npages from the pages marked as free in the coremap
		//lock_acquire(access_cmap);
		int spl = splhigh();
		//kprintf("pages avail = %d\n", pages_avail);
	//	assert(npages <= pages_avail);
		//search the coremap for npages free pages
		if(npages <= pages_avail){
			unsigned long i, j, k, nfound = 0;
			for(i = 0; i < page_count; i++){
				if (cmap[i].state == freed) {
					nfound++;

					if (nfound == npages){
						//kprintf("PP:kernel? = %d\n", (pstate==kernel));
						//kprintf("PP:cur_as = %x\nPP:setting as = %x\n",curthread->t_vmspace, as);
						for(j = i, k=0; j > i-npages; j--, k++){
							cmap[j].as = as;
							cmap[j].state = dirty;
							cmap[j].num_pages = npages;
							cmap[j].p_state = pstate;
							time_t s;
							u_int32_t ns;
							
							gettime(&s, &ns);
							cmap[j].s = s;
							cmap[j].ns = ns;

							//if(cmap[j].as == 0x80032f00)
									//kprintf("as = %x writing to page #%d\n", cmap[j].as, i);
							if(k==npages-1){ //first page in block
								cmap[j].first_page = 1;
								ret_addr = cmap[j].pa;
								pages_avail -= npages;
								/*
								if(pstate == kernel){
									kprintf("K: updated cmap ind = %d\n", i);
								}
								else{
									kprintf("U: updated cmap ind = %d\n", i);	
								}*/
								//kprintf("writing to page index = %d\n", i);
							}
						}
						break;
					}
				}
				else{
					nfound = 0;
				}

			}
		}
		splx(spl);
	}
	//if(ret_addr == 237568)
	//	kprintf("hereeeeeeeeeeeeeeeeee! addr = %x\n", as);
	return ret_addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t 
alloc_kpages(int npages)
{
	//kernel_pages++;
	//kprintf("kernel_pages = %d\n", kernel_pages);
	paddr_t pa;
	pa = getppages(npages, kernel, 0);

	if (pa == 0) {
		assert(vm_bootstrap_done == 1);
		int spl = splhigh();
		//kprintf("KERNEL: demanding a page\n");
		// Make space on RAM
		unsigned long lru_index = find_lru();
		pa = cmap[lru_index].pa;
		unsigned long offset;
		int evict = 1;
		if(cmap[lru_index].state == clean) evict = EVICT_CLEAN;
		else evict = EVICT_DIRTY;
		//kprintf("K: %x evicting page %d\n", curthread->t_vmspace, pa);
		struct pte *evicted_pte = update_pte(lru_index);

		//get space on disk must be called before updating the cmap entry. that way
		//we look for the offset where the page to be evicted (using the old offset)
		//was stored on disk (if it were stored on disk before) or we just get a new offset.
		if(evict == 1)
			offset = get_space_on_disk(lru_index, evicted_pte->va, SWAPOUT);
		update_cmap(lru_index, NULL, dirty, kernel);
		if(evict == 1)
			swap_out( offset, PADDR_TO_KVADDR(pa));
		splx(spl);
	}
	return PADDR_TO_KVADDR(pa);
}

vaddr_t 
alloc_upages(int npages, struct addrspace *as)
{
	//user_pages++;
	//kprintf("user_pages = %d\n", user_pages);
	paddr_t pa;
	pa = getppages(npages, user, as);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void 
free_kpages(vaddr_t addr)
{
	int spl = splhigh();
	//lock_acquire(access_cmap);
	paddr_t phy_addr = KVADDR_TO_PADDR(addr);
	unsigned long i, j;
	for(i = 0; i < page_count; i++) {
		if (cmap[i].pa == phy_addr)
			break;
	}

	if (cmap[i].first_page == 1){
		unsigned long length = cmap[i].num_pages;
		for (j = 0; j < length ; j++){
			cmap[i+j].state = freed;
			cmap[i+j].p_state = user;
			cmap[i+j].num_pages = -1;
			cmap[i+j].as = NULL;
		}
		cmap[i].first_page = 0;
		pages_avail += length;
	}
	splx(spl);
}

unsigned long find_lru(){
	unsigned long i;
	unsigned long ret = page_count;

	time_t min_s;
	u_int32_t min_ns;
	
	gettime(&min_s, &min_ns);

	for(i = 0; i < page_count; i++){
		//TODO: shouldn't this assert be true all the time???
		assert(cmap[i].state != freed);	//there cannot be any freed pages at this point. if there are freed pages, a page would have been allocated and no need to swap out
		if(cmap[i].state == fixed) continue;
		if(cmap[i].p_state == kernel) continue;
		if(cmap[i].as == NULL)continue;

		if(cmap[i].s < min_s){
			min_s = cmap[i].s;
			min_ns = cmap[i].ns;
			ret = i;
		}
		else if(cmap[i].s == min_s){
			if(cmap[i].ns < min_ns){
				min_s = cmap[i].s;
				min_ns = cmap[i].ns;
				ret = i;
			}
		}
	}
	assert(ret < page_count);
	//kprintf("entry #%d is the lru\n", ret);
	return ret;
}

unsigned long get_space_on_disk(unsigned long index, vaddr_t faultaddress, swap_type_t swap_type){
	struct addrspace *as = cmap[index].as;
	unsigned long i;

	faultaddress = (faultaddress & PAGE_FRAME);

	for(i = 0; i < smap_page_count; i++){
		if((smap[i].as == as) && smap[i].va == faultaddress){
			//kprintf("loading (%x, %d) from offset %u\n", as, faultaddress, (i*PAGE_SIZE));
			//if we find an old entry on disk. It could be either a swap_in or a swap_out operation
			//swap_in: we're loading a page from disk that we had stored previously
			//swap_out: we are re-evicting a page that used to be stored on disk before
			break;
		}
		if(smap[i].state == empty){
			//kprintf("writing (%x, %d) to offset %u\n", as, faultaddress, (i*PAGE_SIZE));
			//if we are getting a new offset, it must be a swap_out operation
			//if it were a swap_in operation, we should have found it earlier on disk!
			assert(swap_type == SWAPOUT);
			assert( i >= (smap_page_count - smap_pages_avail));
			smap_pages_avail--;
			break;
		}
	}
	assert(i < smap_page_count);

	smap[i].state = occupied;
	smap[i].as = as;
	smap[i].va = faultaddress;

	return (i*PAGE_SIZE);
}

/*
	writes the old page content to disk, invalidates the tlb entry
*/
void swap_out(unsigned long offset,vaddr_t va){
	int spl = splhigh();
	struct uio uio_swap;
	
	mk_kuio(&uio_swap,(void *)(va & PAGE_FRAME), PAGE_SIZE, offset, UIO_WRITE);

	int ret = VOP_WRITE(swap_file, &uio_swap);
	if(ret){
		panic("swap_out: couldn't write to disk");
	}
	//invalidate the evicted tlb entry
	u_int32_t ehi,elo,i;
	paddr_t pa = KVADDR_TO_PADDR(va);

	for (i = 0; i < NUM_TLB; i++) {

		TLB_Read(&ehi, &elo, i);

		if ((elo & PAGE_FRAME) == (pa & PAGE_FRAME))	{
			TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);		
		}
	}
	splx(spl);
	return;
}

/*
	writes the page at an a disk offset (offset)
*/
void swap_in(unsigned long offset, vaddr_t va){
	
	int spl=splhigh();

	struct uio uio_swap;
	
	mk_kuio(&uio_swap,(void *)(va & PAGE_FRAME), PAGE_SIZE, offset, UIO_READ);

	int result=VOP_READ(swap_file, &uio_swap);
	if(result) {
		panic("VM: SWAP in Failed");
	}

	splx(spl);
}


struct pte * update_pte(unsigned long index){
	int spl = splhigh();
	//kprintf("update_pte: the addrspace = %x\nupdate_pte: pa = %d\n", cmap[index].as, cmap[index].pa);
	struct pte *old_entry = cmap[index].as->pages;
	//kprintf("PTE:address space = %x\nPTE:cmap[lru].pa = %d\n", cmap[index].as, cmap[index].pa);
	while(old_entry != NULL){
		/*
		if(old_entry == cmap[index].as->pages)
			kprintf("--------------------------------pages----------------------------------\n");
		else if(old_entry == cmap[index].as->stack)
			kprintf("--------------------------------stack----------------------------------\n");
		else if(old_entry == cmap[index].as->heap)
			kprintf("--------------------------------heap----------------------------------\n");
		if(old_entry->pa != 0)
			kprintf("old entry->pa = %d\n",  old_entry->pa);
			*/
		if(old_entry->pa == cmap[index].pa)
			break;
		old_entry  = old_entry->next;
	}
	//kprintf("----------------------------------------------------------------------\n");
	assert(old_entry != NULL);
	old_entry->on_mem = 0;
	old_entry->on_disk = 1;
	old_entry->pa = 0;
	splx(spl);
	return old_entry;
}

paddr_t demand_page(struct pte *entry, struct addrspace *as){
	//kprintf("demand page: entry = %x as = %x\n", entry, as);
	assert (entry != NULL);
	int spl = splhigh();
	assert(vm_bootstrap_done == 1);
	//lock_acquire(access_cmap);
	paddr_t pa = 0;

	if(pages_avail > 0){
		pa = KVADDR_TO_PADDR(alloc_upages(1, as));	//allocate one page
		if(pa == 0){
			splx(spl);
			//lock_release(access_cmap);
			return ENOMEM;	//oops ran out of memory!!! :'(
		}
		entry->pa = pa;
		entry->on_mem = 1;
		//kprintf("U: %x alloc_u'ed a page %d\n", as, pa);
	}
	else{
		//TODO: entry's va is the same as the faultaddress???
		//TODO: we're not freeing all the pages

		// Make space on RAM
		unsigned long lru_index = find_lru();
		unsigned long offset;
		pa = cmap[lru_index].pa;
		int evict = 1;
		if(cmap[lru_index].state == clean) evict = EVICT_CLEAN;
		else evict = EVICT_DIRTY;

		struct pte *evicted_pte = update_pte(lru_index);

		// get_space_on_disk must be called before updating cmap. That way, get_space_on_disk finds on of these:
		//	1) the offset where this page used to be stored, if it happened to be stored on disk before
		//	2) a new offset on disk where the page can be evicted to, if this page was never stored on disk before
		// to do so, it must use the old addrspace that used to own that cmap entry
		if(evict == 1)
			offset = get_space_on_disk(lru_index, evicted_pte->va, SWAPOUT);
		update_cmap(lru_index, as, dirty, user);	//updates the cmap with the new as
		//update_cmap must be called before swap_out
		if(evict == 1)
			swap_out( offset, PADDR_TO_KVADDR(pa));
		entry->pa = pa;
		entry->on_mem = 1;
		//kprintf("U: evicting %d\n", lru_index);
	}
	assert((pa & PAGE_FRAME) == pa);
	splx(spl);
	//lock_release(access_cmap);
	return entry->pa;
}

paddr_t load_page(struct pte *entry, struct addrspace *as, int faulttype){
	//kprintf("load page: entry = %x as = %x\n", entry, as);
	assert (entry != NULL);
	int spl = splhigh();
	assert(vm_bootstrap_done == 1);
	//lock_acquire(access_cmap);
	paddr_t pa = 0;
	unsigned long lru_index;
	unsigned long evict_offset, swapin_offset;

	if(pages_avail > 0){	//leave 5 pages for alloc_kpages?
		pa = KVADDR_TO_PADDR(alloc_upages(1, as));	//allocate one page
		if(pa == 0){
			splx(spl);
			//lock_release(access_cmap);
			return ENOMEM;	//oops ran out of memory!!! :'(
		}
		lru_index = (pa - cmap_start_physaddr)/PAGE_SIZE;

		//update cmap must first update the as that RAM page belongs to, so when we call get_space_on_disk
		//we look for the offset where the page to be swapped in was soted on disk
		update_cmap(lru_index, as, clean, user);
		
		//now get the offset where the page was stored on disk
		swapin_offset = get_space_on_disk(lru_index, entry->va, SWAPIN);


		swap_in(swapin_offset, PADDR_TO_KVADDR(pa));

		assert(pa != 0);
		entry->pa = pa;
		entry->on_mem = 1;
	}
	else{
		//TODO: entry's va is the same as the faultaddress???
		//TODO: we're not freeing all the pages

		// Make space on RAM
		lru_index = find_lru();
		pa = cmap[lru_index].pa;
		int evict = 1;
		if(cmap[lru_index].state == clean) evict = EVICT_CLEAN;
		else evict = EVICT_DIRTY;

		struct pte *evicted_pte = update_pte(lru_index);

		//evict_offset must be called before updating cmap. That way, get_space_on_disk finds on of these:
		//	1) the offset where this page used to be stored, if it happened to be stored on disk before
		//	2) a new offset on disk where the page can be evicted to, if this page was never stored on disk before
		// to do so, it must use the old addrspace that used to own that cmap entry
		if(evict == 1)
			evict_offset = get_space_on_disk(lru_index, evicted_pte->va, SWAPOUT);

		//after this, the cmap entry belongs to the as that is trying to get space on disk
		update_cmap(lru_index, as, dirty, user);

		// Fill space on RAM
		//now that the cmap entry has the addrspace to be swapped in, look for it on disk
		swapin_offset = get_space_on_disk(lru_index, entry->va, SWAPIN);

		//update the cmap entry to be clean

		if(faulttype == VM_FAULT_WRITE)
			update_cmap(lru_index, as, dirty, user);
		else
			update_cmap(lru_index, as, clean, user);

		if(evict == 1)
			swap_out(evict_offset, PADDR_TO_KVADDR(pa));
		swap_in(swapin_offset, PADDR_TO_KVADDR(pa));

		assert(pa != 0);
		entry->pa = pa;
		entry->on_mem = 1;
	}
	assert(entry->on_disk == 1);
	assert((pa & PAGE_FRAME) == pa);

	splx(spl);
	//lock_release(access_cmap);
	return entry->pa;	
}

void update_time(unsigned long index){
	time_t s;
	u_int32_t ns;
	
	gettime(&s, &ns);

	cmap[index].s = s;
	cmap[index].ns = ns;
}

void update_cmap(unsigned long index, struct addrspace *as, cmap_state_t state, page_state_t pstate){
	cmap[index].as = as;
	cmap[index].state = state;
	cmap[index].num_pages = 1;
	cmap[index].first_page = 1;
	cmap[index].p_state = pstate;
	//kprintf("updating cmap of %d, belongs to as = %x\n", cmap[index].pa, cmap[index].as);
	//kprintf("----------------------------------------------------------------------\n");
	//kprintf("----------------------------------------------------------------------\n");
	update_time(index);
}

int on_stack(vaddr_t faultaddress, struct addrspace *as){
	if(faultaddress >= as->stack_begin && faultaddress < as->stack_end)
		return 1;
	return 0;
}

int on_heap(vaddr_t faultaddress, struct addrspace *as){
	if(faultaddress >= as->heap_begin && faultaddress < as->heap_end)
		return 1;
	return 0;
}

int on_region_pages(vaddr_t faultaddress, struct addrspace *as){
	if(faultaddress >= as->pages->va && faultaddress < (as->last_page->va + PAGE_SIZE))
		return 1;
	return 0;
}


struct pte *find_entry_on_mem(vaddr_t faultaddress, struct addrspace *as){

	if (as == NULL) {
		
		// No address space set up. This is probably a kernel
		// fault early in boot. Return EFAULT so as to panic
		// instead of getting into an infinite faulting loop.
		return NULL;
	}

	// Assert that the address space has been set up properly. 

	assert(as->pages != NULL);
	assert(as->last_page != NULL);
	assert(as->stack != NULL);
	assert(as->last_stack != NULL);
	assert(as->heap != NULL);
	assert(as->heap_begin != 0);
	assert(as->heap_end != 0);
	assert(as->regions != NULL);
	assert(as->last_region != NULL);
	assert((as->pages->va & PAGE_FRAME) == as->pages->va);
	assert((as->pages->pa & PAGE_FRAME) == as->pages->pa);
	assert((as->regions->pa & PAGE_FRAME) == as->regions->pa);
	assert((as->regions->pa & PAGE_FRAME) == as->regions->pa);
	assert(faultaddress <= MIPS_KSEG0);

	struct pte* entry;

	if(on_stack(faultaddress, as)){
		entry = as->stack;
		while(entry != NULL){	//search for the address in each of the stack pages
			if((faultaddress >= entry->va) && faultaddress < (entry->va + PAGE_SIZE))break;
			entry = entry->next;
		}
	}
	else if(on_heap(faultaddress, as)){
		entry = as->heap;
		while(entry != NULL){	//search for the address in each of the stack pages
			if((faultaddress >= entry->va) && faultaddress < (entry->va + PAGE_SIZE))break;
			entry = entry->next;
		}
	}
	else if(on_region_pages(faultaddress, as)){
		entry = as->pages;
		while(entry != NULL){	//search for the address in each of the stack pages
			if((faultaddress >= entry->va) && faultaddress < (entry->va + PAGE_SIZE))break;
			entry = entry->next;   
		}
	}
	else {
		return NULL;
	}
	assert(entry->va == faultaddress);
	return entry;
}

int vm_fault(int faulttype, vaddr_t faultaddress)
{
	int spl = splhigh();

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		//We always create pages read-write, so we can't get this 
		panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		splx(spl);
		return EINVAL;
	}

	struct addrspace *as = curthread->t_vmspace;
	struct pte* entry;

	entry = find_entry_on_mem(faultaddress, as);
	if(entry == NULL){
		splx(spl);
		return EFAULT;
	}

	paddr_t pa = entry->pa;

	if(entry->on_mem == 0){
		//assert((pa == 0) || (pa == 0xdeadbeef));
		if(entry->on_disk == 1){
			pa = load_page(entry, as, faulttype);
		}
		else{
			pa = demand_page(entry, as);
		}
	}


	assert(pa != 0);

	// make sure it's page-aligned 
	assert((pa & PAGE_FRAME)==pa);

	u_int32_t ehi, elo;
	int i;
	for (i = 0; i < NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = pa | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, pa);
		TLB_Write(ehi, elo, i);
		splx(spl);
		return 0;
	}

	ehi = faultaddress;
	elo = pa | TLBLO_DIRTY | TLBLO_VALID;
	TLB_Random(ehi, elo);
	splx(spl);
	return 0;
}
