#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>


struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	as->pages = NULL;
	as->regions = NULL;

	return as;
}

void
as_destroy(struct addrspace *as)
{
	if (as != NULL) {
		struct region_array *current_region = as->regions;
		struct region_array *next_region;
		while(current_region != NULL){
			next_region = current_region->next;
			kfree(current_region);
			current_region = next_region;
		}
		as->regions = NULL;

		struct pte *current_pte = as->pages;
		struct pte *next_pte;
		while (current_pte != NULL){
			next_pte = current_pte->next;
			kfree(current_pte);
			current_pte = next_pte;
		}
		as->pages = NULL;
	}
	kfree(as);
}

void
as_activate(struct addrspace *as)
{
	int i, spl;

	(void)as;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages; 

	// Align the region. First, the base... 
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;
	assert((vaddr&PAGE_FRAME) == vaddr);

	// ...and now the length. 
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	//need to allocate a new region_array element
	//struct region_array *last_elem;

	if(as->regions == NULL){
		as->regions = (struct region_array *) kmalloc(sizeof(struct region_array));
		as->regions->next = NULL;	//end of linked list
		as->last_region = as->regions;	//first element in the linked list is the last element!
	}
	else{
		as->last_region->next = (struct region_array *) kmalloc(sizeof(struct region_array));
		as->last_region = as->last_region->next;
		as->last_region->next = NULL;	//end of linked list		as->regions->last = last_elem;
	}

	as->last_region->pa = 0;
	as->last_region->va = vaddr;
	as->last_region->rwx = (readable | writeable | executable); //((readable&1)<<2) | ((writeable&1)<<1) | (executable&1);
	as->last_region->num_pages = npages;

	return 0;
}

int
as_prepare_load(struct addrspace *as)
{
	struct region_array *region;
	size_t i;
	vaddr_t va;

	region = as->regions;
	while(region != NULL){

		va = region->va;
		assert(va == (va & PAGE_FRAME));
		for(i = 0; i < region->num_pages; i++){
			if(as->pages==NULL){
				as->pages = (struct pte *) kmalloc(sizeof(struct pte));
				as->last_page = as->pages;
			}
			else{
				as->last_page->next = (struct pte *) kmalloc(sizeof(struct pte));
				as->last_page = as->last_page->next;
			}
			struct pte *last = as->last_page;
			
			last->pa = 0;
			last->va = va;
			last->rwx = region->rwx;
			last->on_mem = 0;
			last->on_disk = 0;
			last->next = NULL;	//end of pte linked list
			va += PAGE_SIZE;
		}
		region = region->next;
	}

	vaddr_t stackva = USERSTACK - VM_STACKPAGES* PAGE_SIZE;
	as->stack_begin = stackva;
	as->stack_end = USERSTACK;

	assert((stackva & PAGE_FRAME) == stackva);

	struct pte* last = as->last_page;
	struct pte *stack_page;

	for( i = 0; i < VM_STACKPAGES; i++){
		stack_page = (struct pte *) kmalloc(sizeof(struct pte));
		last->next = stack_page;

		stack_page->pa = 0;
		stack_page->va = stackva;
		stack_page->next = NULL;
		stack_page->on_mem = 0;
		stack_page->on_disk = 0;
		if(i == 0){
			as->stack = stack_page;
		}

		stackva += PAGE_SIZE;
		last = last->next;
	}
	as->last_stack = last;

	struct pte *heap_page = (struct pte *) kmalloc(sizeof(struct pte));
	last->next = heap_page;
	/*
	paddr_t pa = demand_page(heap_page, as);//KVADDR_TO_PADDR(alloc_upages(1, curthread->t_vmspace));	//allocate one page
	if(pa == 0){
		return ENOMEM;	//oops ran out of memory!!! :'(
	}
	kprintf("U: %x alloc_u'ed a page %d\n",curthread->t_vmspace, pa);
	//assert(as == curthread->t_vmspace);
	//heap_page->as = as;
	heap_page->pa = pa;
	*/
	heap_page->pa = 0;
	heap_page->va = va;
	heap_page->on_mem = 0;
	heap_page->on_disk = 0;
	heap_page->next = NULL;

	as->heap = heap_page;
	as->last_heap = heap_page;
	as->heap_begin = va;	//at first, the heap occupies a single page
	as->heap_end = va;

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{

	*stackptr = USERSTACK;
	return 0;
}


/*
	if as has old has a copy on disk, make a copy on disk
*/
int
as_copy(volatile struct addrspace *old, volatile struct addrspace **ret)
{
	//kprintf("as_copy: old as = %x new_as = %x\n", old, *ret);
	//kprintf("\r");
	int spl = splhigh();
	volatile struct addrspace *new;

	new = as_create();
	if (new == NULL) {
		return ENOMEM;
	}

	struct region_array *region, *new_region;

	region = old->regions;
	while(region != NULL){
		if((new)->regions != NULL){
			new_region = (new)->last_region;
			
			new_region->next = (struct region_array *) kmalloc(sizeof(struct region_array));
			new_region = new_region->next;

			(new)->last_region = new_region;
		}
		else{
			(new)->regions = (struct region_array *) kmalloc(sizeof(struct region_array));
			(new)->last_region = (new)->regions;
			new_region = (new)->regions;
		}

		new_region->pa = region->pa;
		new_region->va = region->va;
		new_region->rwx = region->rwx;
		new_region->num_pages = region->num_pages;
		new_region->next = NULL;

		region = region->next;
	}

	if (as_prepare_load(new)) {
		as_destroy(new);
		splx(spl);
		return ENOMEM;
	}

	volatile struct pte *old_page, *new_page;
	old_page = old->pages;
	new_page = (new)->pages;
	while(old_page != NULL){

		paddr_t pa;

		if((old_page->on_disk == 1) && (old_page->on_mem == 0)){
			//kprintf("%d is on disk, let's load  it\n", old_page->va);
			pa = load_page(old_page, old, VM_FAULT_WRITE);	//load the old page on memory
		}

		if(old_page->on_mem == 1){
			//kprintf("%d is on mem, let's copy\n",old_page->va);
			pa = demand_page(new_page, new);
			assert(pa != 0);
			new_page->pa = pa;
			new_page->on_mem = 1;
		}

		memmove((void *)PADDR_TO_KVADDR(new_page->pa),
			(const void *)PADDR_TO_KVADDR(old_page->pa),PAGE_SIZE);

		old_page = old_page->next;
		new_page = new_page->next;
	}

	*ret = new;
	splx(spl);
	return 0;
}

/*
	if as has old has a copy on disk, make a copy on disk

int
as_copy(struct addrspace *old, struct addrspace **new)
{
	//kprintf("as_copy: old as = %x new_as = %x\n", old, *new);
	//kprintf("\r");
	int spl = splhigh();
	//struct addrspace *new;

	*new = as_create();
	if (*new == NULL) {
		return ENOMEM;
	}

	struct region_array *region, *new_region;

	region = old->regions;
	while(region != NULL){
		if((*new)->regions != NULL){
			new_region = (*new)->last_region;
			
			new_region->next = (struct region_array *) kmalloc(sizeof(struct region_array));
			new_region = new_region->next;

			(*new)->last_region = new_region;
		}
		else{
			(*new)->regions = (struct region_array *) kmalloc(sizeof(struct region_array));
			(*new)->last_region = (*new)->regions;
			new_region = (*new)->regions;
		}

		new_region->pa = region->pa;
		new_region->va = region->va;
		new_region->rwx = region->rwx;
		new_region->num_pages = region->num_pages;
		new_region->next = NULL;

		region = region->next;
	}

	if (as_prepare_load(*new)) {
		as_destroy(*new);
		splx(spl);
		return ENOMEM;
	}

	struct pte *old_page, *new_page;
	old_page = old->pages;
	new_page = (*new)->pages;
	while(old_page != NULL){

		paddr_t pa;

		if((old_page->on_disk == 1) && (old_page->on_mem == 0)){
			pa = load_page(old_page, old);	//load the old page on memory
		}

		if(old_page->on_mem){
			pa = demand_page(new_page, *new);
			assert(pa != 0);
			new_page->pa = pa;
			new_page->on_mem = 1;
		}

		memmove((void *)PADDR_TO_KVADDR(new_page->pa),
			(const void *)PADDR_TO_KVADDR(old_page->pa),PAGE_SIZE);

		old_page = old_page->next;
		new_page = new_page->next;
	}

	//*ret = new;
	splx(spl);
	return 0;
}
*/