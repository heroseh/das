#include "das.h"
#include "das.c"

void stk_test() {
	DasStk(int) stk = NULL;

	int elmt = 0;
	uintptr_t idx = DasStk_push(&stk);
	*DasStk_get(&stk, idx) = elmt;
	das_assert(memcmp(DasStk_data(&stk), &elmt, sizeof(int)) == 0, "test failed: DasStk_push");

	for (int i = 1; i < 10; i += 1) {
		idx = DasStk_push(&stk);
		*DasStk_get(&stk, idx) = i;
	}

	int b[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	das_assert(DasStk_count(&stk) == 10, "test failed: DasStk_push cause resize capacity");
	das_assert(memcmp(DasStk_data(&stk), b, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_push cause resize capacity");

	int popped_elmt = *DasStk_get_last(&stk);
	DasStk_pop(&stk);
	das_assert(popped_elmt == 9, "test failed: DasStk_pop");
	das_assert(DasStk_count(&stk) == 9, "test failed: DasStk_pop");
	int c[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
	das_assert(memcmp(DasStk_data(&stk), c, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_pop");

	popped_elmt = *DasStk_get(&stk, 4);
	DasStk_remove_shift(&stk, 4);
	das_assert(popped_elmt == 4, "test failed: DasStk_remove_shift middle");
	das_assert(DasStk_count(&stk) == 8, "test failed: DasStk_remove_shift middle");
	int d[] = {0, 1, 2, 3, 5, 6, 7, 8};
	das_assert(memcmp(DasStk_data(&stk), d, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_remove_shift middle");

	popped_elmt = *DasStk_get(&stk, 0);
	DasStk_remove_shift(&stk, 0);
	das_assert(popped_elmt == 0, "test failed: DasStk_remove_shift start");
	das_assert(DasStk_count(&stk) == 7, "test failed: DasStk_remove_shift start");
	int e[] = {1, 2, 3, 5, 6, 7, 8};
	das_assert(memcmp(DasStk_data(&stk), e, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_remove_shift start");

	popped_elmt = *DasStk_get(&stk, 6);
	DasStk_remove_shift(&stk, 6);
	das_assert(popped_elmt == 8, "test failed: DasStk_remove_shift end");
	das_assert(DasStk_count(&stk) == 6, "test failed: DasStk_remove_shift end");
	int f[] = {1, 2, 3, 5, 6, 7};
	das_assert(memcmp(DasStk_data(&stk), f, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_remove_shift end");

	popped_elmt = *DasStk_get(&stk, 2);
	DasStk_remove_swap(&stk, 2);
	das_assert(popped_elmt == 3, "test failed: DasStk_remove_swap middle");
	das_assert(DasStk_count(&stk) == 5, "test failed: DasStk_remove_swap middle");
	int g[] = {1, 2, 7, 5, 6};
	das_assert(memcmp(DasStk_data(&stk), g, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_remove_swap middle");

	popped_elmt = *DasStk_get(&stk, 0);
	DasStk_remove_swap(&stk, 0);
	das_assert(popped_elmt == 1, "test failed: DasStk_remove_swap start");
	das_assert(DasStk_count(&stk) == 4, "test failed: DasStk_remove_swap start");
	int h[] = {6, 2, 7, 5};
	das_assert(memcmp(DasStk_data(&stk), h, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_remove_swap start");

	popped_elmt = *DasStk_get(&stk, 3);
	DasStk_remove_swap(&stk, 3);
	das_assert(popped_elmt == 5, "test failed: DasStk_remove_swap end");
	das_assert(DasStk_count(&stk) == 3, "test failed: DasStk_remove_swap end");
	int i[] = {6, 2, 7};
	das_assert(memcmp(DasStk_data(&stk), i, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_remove_swap end");

	elmt = 77;
	DasStk_insert(&stk, 2);
	*DasStk_get(&stk, 2) = elmt;
	das_assert(DasStk_count(&stk) == 4, "test failed: DasStk_insert middle");
	int j[] = {6, 2, 77, 7};
	das_assert(memcmp(DasStk_data(&stk), j, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_insert middle");

	elmt = 88;
	DasStk_insert(&stk, 0);
	*DasStk_get(&stk, 0) = elmt;
	das_assert(DasStk_count(&stk) == 5, "test failed: DasStk_insert start");
	int k[] = {88, 6, 2, 77, 7};
	das_assert(memcmp(DasStk_data(&stk), k, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_insert start");

	elmt = 99;
	DasStk_insert(&stk, 5);
	*DasStk_get(&stk, 5) = elmt;
	das_assert(DasStk_count(&stk) == 6, "test failed: DasStk_insert end");
	int l[] = {88, 6, 2, 77, 7, 99};
	das_assert(memcmp(DasStk_data(&stk), l, DasStk_count(&stk) * sizeof(int)) == 0, "test failed: DasStk_insert end");
}

void deque_test() {
	DasDeque(int) deque = NULL;
	DasDeque_resize_cap(&deque, 6);
	das_assert(deque && deque->front_idx == 0 && deque->back_idx == 0 && deque->cap >= 6, "test failed: DasDeque_resize_cap");

	for (int i = 0; i < 10; i += 1) {
		DasDeque_push_front(&deque);
		*DasDeque_get(&deque, 0) = i; // initialize the value we pushed on
		das_assert(DasDeque_count(&deque) == i + 1, "test failed: DasDeque_push_front enough to resize the capacity");
	}

	// deque data should look like: 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
	for (int i = 0; i < 10; i += 1) {
		das_assert(*DasDeque_get(&deque, i) == 9 - i, "test failed: DasDeque_push_front enough to resize the capacity");
	}

	for (int i = 0; i < 10; i += 1) {
		int elmt = *DasDeque_get(&deque, 0);
		DasDeque_pop_front(&deque);
		das_assert(elmt == 9 - i, "test failed: DasDeque_pop_front");
		das_assert(DasDeque_count(&deque) == 9 - i, "test failed: DasDeque_pop_front");
	}


	DasDeque_deinit(&deque);
	DasDeque_resize_cap(&deque, 6);
	das_assert(deque && deque->front_idx == 0 && deque->back_idx == 0 && deque->cap >= 6, "test failed: DasDeque_resize_cap");

	for (int i = 0; i < 10; i += 1) {
		DasDeque_push_back(&deque);
		*DasDeque_get_back(&deque, 0) = i; // initialize the value we pushed on
		das_assert(DasDeque_count(&deque) == i + 1, "test failed: DasDeque_push_back enough to resize the capacity");
	}

	// deque data should look like: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
	for (int i = 0; i < 10; i += 1) {
		das_assert(*DasDeque_get(&deque, i) == i, "test failed: DasDeque_push_front enough to resize the capacity");
	}

	for (int i = 0; i < 10; i += 1) {
		int elmt = *DasDeque_get_back(&deque, 0);
		DasDeque_pop_back(&deque);
		das_assert(elmt == 9 - i, "test failed: DasDeque_pop_back");
		das_assert(DasDeque_count(&deque) == 9 - i, "test failed: DasDeque_pop_back");
	}
}

typedef struct {
	void* data;
	uint32_t pos;
	uint32_t size;
} LinearAlctor;

void* LinearAlctor_alloc_fn(void* alctor_data, void* ptr, uintptr_t old_size, uintptr_t size, uintptr_t align) {
	LinearAlctor* alctor = (LinearAlctor*)alctor_data;
	if (!ptr && size == 0) {
		// reset
		alctor->pos = 0;
	} else if (!ptr) {
		// allocate
		ptr = das_ptr_add(alctor->data, alctor->pos);
		ptr = das_ptr_round_up_align(ptr, align);
		uint32_t next_pos = das_ptr_diff(ptr, alctor->data) + size;
		if (next_pos <= alctor->size) {
			alctor->pos = next_pos;
			return ptr;
		} else {
			return NULL;
		}
	} else if (ptr && size > 0) {
		// reallocate

		// check if the ptr is the last allocation to resize in place
		if (das_ptr_add(alctor->data, alctor->pos - old_size) == ptr) {
			uint32_t next_pos = das_ptr_diff(ptr, alctor->data) + size;
			if (next_pos <= alctor->size) {
				alctor->pos = next_pos;
				return ptr;
			}
		}

		// if we cannot extend in place, then just allocate a new block.
		void* new_ptr = LinearAlctor_alloc_fn(alctor, NULL, 0, size, align);
		if (new_ptr == NULL) { return NULL; }

		memcpy(new_ptr, ptr, size < old_size ? size : old_size);
		return new_ptr;
	} else {
		// deallocate
		// do nothing
		return NULL;
	}

	return NULL;
}

DasAlctor LinearAlctor_as_das(LinearAlctor* alctor) {
	return (DasAlctor){ .fn = LinearAlctor_alloc_fn, .data = alctor };
}

typedef struct {
	void* data;
	int whole_number;
	float number;
	double x;
	double y;
} SomeStruct;

void alloc_test() {
	DasAlctor alctor = DasAlctor_default;
	SomeStruct* some = das_alloc_elmt(SomeStruct, alctor);

	int* ints = das_alloc_array(int, alctor, 200);
	memset(ints, 0xac, 200 * sizeof(int));

	int* ints_big_align = (int*)das_alloc(alctor, 200 * sizeof(int), 1024);
	memset(ints_big_align, 0xef, 200 * sizeof(int));
	das_assert((uintptr_t)ints_big_align % 1024 == 0, "test failed: trying to alignment our int array to 1024");

	{
		// the buffer is zeroed, so the linear allocator will return zeroed memory
		char buffer[1024] = {0};
		LinearAlctor la = { .data = buffer, .pos = 0, .size = sizeof(buffer) };

		// make the allocations functions use the linear allocator
		alctor = LinearAlctor_as_das(&la);

		float* floats = das_alloc_array(float, alctor, 256);
		for (int i = 0; i < 256; i += 1) {
			das_assert(floats[i] == 0, "test failed: linear allocator memory should be zero");

			float* buffer_float = (float*)(buffer + (i * sizeof(float)));
			das_assert(&floats[i] == buffer_float, "test failed: linear allocator pointers should match");
		}

		das_assert(la.pos == la.size, "test failed: we should have consumed all of the linear allocators memory");

		// restore the system allocator
		alctor = DasAlctor_default;
	}

	das_dealloc_elmt(SomeStruct, alctor, some);

	ints = das_realloc_array(int, alctor, ints, 200, 400);
	for (int i = 0; i < 200; i += 1) {
		das_assert(ints[i] == 0xacacacac, "test failed: reallocation has not preserved the memory");
	}


	ints_big_align = (int*)das_realloc(alctor, ints_big_align, 200 * sizeof(int), 400 * sizeof(int), 1024);
	for (int i = 0; i < 200; i += 1) {
		das_assert(ints_big_align[i] == 0xefefefef, "test failed: reallocation has not preserved the memory");
	}

	das_dealloc(alctor, ints_big_align, 400 * sizeof(int), 1024);
	das_dealloc_array(int, alctor, ints, 400);
}

typedef struct {
	void* address_space;
	uintptr_t pos;
	uintptr_t commited_size;
	uintptr_t commit_grow_size;
	uintptr_t reserved_size;
} VirtMemLinearAlctor;

DasError VirtMemLinearAlctor_init(VirtMemLinearAlctor* alctor, uintptr_t reserved_size, uintptr_t commit_grow_size) {
	uintptr_t reserve_align;
	uintptr_t page_size;
	DasError error = das_virt_mem_page_size(&page_size, &reserve_align);
	if (error) return error;

	// reserved_size must be a multiple of the reserve_align.
	// commit_grow_size must be multiple of the page size.
	commit_grow_size = das_round_up_nearest_multiple_u(commit_grow_size, page_size);
	reserved_size = das_round_up_nearest_multiple_u(reserved_size, reserve_align);
	reserved_size = das_round_up_nearest_multiple_u(reserved_size, commit_grow_size);
	void* address_space;
	error = das_virt_mem_reserve(NULL, reserved_size, &address_space);
	if (error) return error;

	alctor->address_space = address_space;
	alctor->pos = 0;
	alctor->commited_size = 0;
	alctor->commit_grow_size = commit_grow_size;
	alctor->reserved_size = reserved_size;
	return DasError_success;
}

DasBool _VirtMemLinearAlctor_commit_next_chunk(VirtMemLinearAlctor* alctor) {
	if (alctor->commited_size == alctor->reserved_size) {
		// linear alloctor reserved_size has been exhausted.
		// there is no more memory to commit.
		return das_false;
	} else {
		//
		// commit the next pages of memory using the grow size the linear allocator was initialized with.
		void* next_pages_start = das_ptr_add(alctor->address_space, alctor->commited_size);

		//
		// make sure the commit grow size does not go past the reserved address_space.
		uintptr_t grow_size = das_min_u(alctor->commit_grow_size, alctor->reserved_size - alctor->commited_size);

		DasError error = das_virt_mem_commit(next_pages_start, grow_size, DasVirtMemProtection_read_write);
		das_assert(error == 0, "failed to commit memory next_pages_start(%p), grow_size(%zu), error_code(0x%x)",
			next_pages_start, grow_size, error);
		alctor->commited_size += grow_size;
		return das_true;
	}
}

void* VirtMemLinearAlctor_alloc_fn(void* alctor_data, void* ptr, uintptr_t old_size, uintptr_t size, uintptr_t align) {
	VirtMemLinearAlctor* alctor = (VirtMemLinearAlctor*)alctor_data;
	if (!ptr && size == 0) {
		// reset by decommiting the memory back to the OS but retaining the reserved address space.
		DasError error = das_virt_mem_decommit(alctor->address_space, alctor->commited_size);
		das_assert(error == 0, "failed to decommit memory address_space(%p), commited_size(%zu)",
			alctor->address_space, alctor->commited_size);

		alctor->pos = 0;
		alctor->commited_size = 0;
	} else if (!ptr) {
		// allocate
		while (1) {
			//
			// get the next pointer and align it
			ptr = das_ptr_add(alctor->address_space, alctor->pos);
			ptr = das_ptr_round_up_align(ptr, align);
			uint32_t next_pos = das_ptr_diff(ptr, alctor->address_space) + size;
			if (next_pos <= alctor->commited_size) {
				//
				// success, the requested size can fit in the linear block of memory.
				alctor->pos = next_pos;
				return ptr;
			} else {
				//
				// failure, not enough room in the linear block of memory that is commited.
				// so lets try to commit more memory.
				if (!_VirtMemLinearAlctor_commit_next_chunk(alctor))
					return NULL;
			}
		}
	} else if (ptr && size > 0) {
		// reallocate

		// check if the ptr is the last allocation to resize in place
		if (das_ptr_add(alctor->address_space, alctor->pos - old_size) == ptr) {
			while (1) {
				uint32_t next_pos = das_ptr_diff(ptr, alctor->address_space) + size;
				if (next_pos <= alctor->commited_size) {
					alctor->pos = next_pos;
					return ptr;
				} else {
					//
					// failure, not enough room in the linear block of memory that is commited.
					// so lets try to commit more memory.
					if (!_VirtMemLinearAlctor_commit_next_chunk(alctor))
						return NULL;
				}
			}
		}

		// if we cannot extend in place, then just allocate a new block.
		void* new_ptr = VirtMemLinearAlctor_alloc_fn(alctor, NULL, 0, size, align);
		if (new_ptr == NULL) { return NULL; }

		memcpy(new_ptr, ptr, size < old_size ? size : old_size);
		return new_ptr;
	} else {
		// deallocate
		// do nothing
		return NULL;
	}

	return NULL;
}

DasAlctor VirtMemLinearAlctor_as_das(VirtMemLinearAlctor* alctor) {
	return (DasAlctor){ .fn = VirtMemLinearAlctor_alloc_fn, .data = alctor };
}

void virt_mem_tests() {
	uintptr_t reserve_align;
	uintptr_t page_size;
	DasError error = das_virt_mem_page_size(&page_size, &reserve_align);
	das_assert(error == 0, "failed to get the page size: 0x%x", error);

	//
	// tests that need to fail cannot be checked without
	// segfault handlers and this is messy across different OSs.
	// maybe in the future we can do this. but not now.
	// so for now just increment this macro and manually test these.
	// maybe this can be passed through in the command-line with the -D flags.
#define RUN_FAIL_TEST 0

	//
	// reserved MBs and grow by commiting KBs at a time
	VirtMemLinearAlctor la_alctor = {0};
	uintptr_t reserved_size = reserve_align * 1024;
	uintptr_t commit_grow_size = page_size;
	error = VirtMemLinearAlctor_init(&la_alctor, reserved_size, commit_grow_size);
	das_assert(error == 0, "failed to initial linear allocator: 0x%x", error);

	DasAlctor alctor = VirtMemLinearAlctor_as_das(&la_alctor);

#if RUN_FAIL_TEST == 1
	//
	// writing this byte will crash since memory has not been commited
	printf("RUN_FAIL_TEST %u: we should get a SIGSEGV here\n", RUN_FAIL_TEST);
	memset(la_alctor.address_space, 0xac, 1);
#endif

	//
	// this tests reserved memory and then committing it later
	void* ptr = das_alloc(alctor, commit_grow_size * 2, 1);
	memset(ptr, 0xac, commit_grow_size * 2);

#if RUN_FAIL_TEST == 2
	printf("RUN_FAIL_TEST %u: we should get a SIGSEGV here\n", RUN_FAIL_TEST);
	// writing 1 more byte over will crash the program
	memset(ptr, 0xac, commit_grow_size * 2 + 1);
#endif

	//
	// reset the allocator.
	// this will decommit all the pages and start from the beginning
	das_alloc_reset(alctor);

#if RUN_FAIL_TEST == 3
	printf("RUN_FAIL_TEST %u: we should get a SIGSEGV here\n", RUN_FAIL_TEST);
	//
	// writing this byte will crash since memory has been decommitted
	memset(la_alctor.address_space, 0xac, 1);
#endif

	//
	// this tests committing the memory that we previously decommitted.
	ptr = das_alloc(alctor, commit_grow_size * 2, 1);
	das_assert(ptr == la_alctor.address_space, "reset should make our allocator start from the beginning again");
	for (uintptr_t i = 0; i < commit_grow_size * 2; i += 1) {
		uint8_t byte = *(uint8_t*)das_ptr_add(ptr, i);
		das_assert(
			byte == 0,
			"memory should be zero after we decommit and commit the memory again... got 0x%x at %zu",
			byte, i
		);
	}

	//
	// committing 3 pages and marking the middle as read only
	reserved_size = das_round_up_nearest_multiple_u(page_size * 3, reserve_align);
	error = das_virt_mem_reserve(NULL, reserved_size, &ptr);
	error = das_virt_mem_commit(ptr, page_size * 3, DasVirtMemProtection_read_write);
	void* first_page = ptr;
	void* middle_page = das_ptr_add(ptr, page_size);
	void* last_page = das_ptr_add(ptr, page_size * 2);
	error = das_virt_mem_protection_set(middle_page, page_size, DasVirtMemProtection_read);

	//
	// test writing to the surrounding pages that have read write access.
	memset(first_page, 0xac, page_size);
	memset(last_page, 0xac, page_size);

	//
	// test reading from the middle page as the protection should be read only
	das_assert(*(uint8_t*)middle_page == 0, "newly commited memory should be 0");

#if RUN_FAIL_TEST == 4
	printf("RUN_FAIL_TEST %u: we should get a SIGSEGV here\n", RUN_FAIL_TEST);
	//
	// writing this byte will crash since memory it has read only protection
	memset(middle_page, 0xac, 1);
#endif

	//
	// release the memory and unreserve the address space
	error = das_virt_mem_release(ptr, reserved_size);

#if RUN_FAIL_TEST == 5
	printf("RUN_FAIL_TEST %u: we should get a SIGSEGV here\n", RUN_FAIL_TEST);
	//
	// writing this byte will crash since memory it has been released/unmapped
	memset(first_page, 0xac, 1);
#endif

	char* path = "das_test.c";
	DasFileHandle file_handle;
	error = das_file_open(path, DasFileFlags_read, &file_handle);
	das_assert(error == 0, "error opening file at %s : 0x%x", path, error);

	//
	// test mapping the file of the source code of this executable and reading the first line.
	// the offset starts after the first character to test the offset functionality.
	// the size is short and not a page size to test that out too.
	uint64_t this_file_map_offset = 1;
	uintptr_t this_file_map_size = 32;
	DasMapFileHandle this_file_map_file_handle;
	void* this_file_mem;
	error = das_virt_mem_map_file(
		NULL, file_handle, DasVirtMemProtection_read,
		this_file_map_offset, this_file_map_size, &this_file_mem, &this_file_map_file_handle);

	das_assert(error == 0, "failed to map the source code of the executable");
	char* first_line_of_code = "include \"das.h\"";
	das_assert(
		strncmp(this_file_mem, first_line_of_code, strlen(first_line_of_code)) == 0,
		"failed testing mapping the source code of the exe and reading the first line");


	error = das_virt_mem_unmap_file(this_file_mem, this_file_map_size, this_file_map_file_handle);
	das_assert(error == 0, "failed testing closing the memory mapped file of the source code");

	das_file_close(file_handle);

#undef RUN_FAIL_TEST
}

int main(int argc, char** argv) {
	alloc_test();
	stk_test();
	deque_test();
	virt_mem_tests();

	printf("all tests were successful\n");
	return 0;
}

