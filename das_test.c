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

int main(int argc, char** argv) {
	alloc_test();
	stk_test();
	deque_test();

	printf("all tests were successful\n");
	return 0;
}

