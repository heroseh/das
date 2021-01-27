#ifndef DAS_H
#error "expected das.h to be included before das.c"
#endif

// ======================================================================
//
//
// General
//
//
// ======================================================================

#include <stdarg.h> // va_start, va_end

noreturn void _das_abort(const char* file, int line, const char* func, char* assert_test, char* message_fmt, ...) {
	if (assert_test) {
		fprintf(stderr, "assertion failed: %s\nmessage: ", assert_test);
	} else {
		fprintf(stderr, "abort reason: ");
	}

	va_list args;
	va_start(args, message_fmt);
	vfprintf(stderr, message_fmt, args);
	va_end(args);

	fprintf(stderr, "\nfile: %s:%d\n%s\n", file, line, func);
	abort();
}

// ======================================================================
//
//
// Memory Utilities
//
//
// ======================================================================

void* das_ptr_round_up_align(void* ptr, uintptr_t align) {
	das_debug_assert(das_is_power_of_two(align), "align must be a power of two but got: %zu", align);
	return (void*)(((uintptr_t)ptr + (align - 1)) & ~(align - 1));
}

void* das_ptr_round_down_align(void* ptr, uintptr_t align) {
	das_debug_assert(das_is_power_of_two(align), "align must be a power of two but got: %zu", align);
	return (void*)((uintptr_t)ptr & ~(align - 1));
}

// ======================================================================
//
//
// Custom Allocator API
//
//
// ======================================================================

#include <malloc.h>

#ifdef _WIN32

// fortunately, windows provides aligned memory allocation function
void* das_system_alloc_fn(void* alloc_data, void* ptr, uintptr_t old_size, uintptr_t size, uintptr_t align) {
	if (!ptr && size == 0) {
		// reset not supported so do nothing
		return NULL;
	} else if (!ptr) {
		// allocate
		return _aligned_malloc(size, align);
	} else if (ptr && size > 0) {
		// reallocate
		return _aligned_realloc(ptr, size, align);
	} else {
		// deallocate
		return _aligned_free(ptr);
	}
}

#else // posix

//
// the C11 standard says malloc is guaranteed aligned to alignof(max_align_t).
// but since we are targeting C99 we have defined our own das_max_align_t that should resemble the same thing.
// so allocations that have alignment less than or equal to this, can directly call malloc, realloc and free.
// luckly this is for most allocations.
//
// but there are alignments that are larger (for example is intel AVX256 primitives).
// these require calls aligned_alloc.
void* das_system_alloc_fn(void* alloc_data, void* ptr, uintptr_t old_size, uintptr_t size, uintptr_t align) {
	if (!ptr && size == 0) {
		// reset not supported so do nothing
		return NULL;
	} else if (!ptr) {
		// allocate

		// the posix documents say that align must be atleast sizeof(void*)
		align = das_max_u(align, sizeof(void*));
		das_assert(posix_memalign(&ptr, align, size) == 0, "posix_memalign failed: errno %u", errno);
		return ptr;
	} else if (ptr && size > 0) {
		// reallocate
		if (align <= alignof(das_max_align_t)) {
			return realloc(ptr, size);
		}

		//
		// there is no aligned realloction on any posix based systems :(
		void* new_ptr = NULL;
		// the posix documents say that align must be atleast sizeof(void*)
		align = das_max_u(align, sizeof(void*));
		das_assert(posix_memalign(&new_ptr, align, size) == 0, "posix_memalign failed: errno %u", errno);

		memcpy(new_ptr, ptr, das_min_u(old_size, size));
		free(ptr);
		return new_ptr;
	} else {
		// deallocate
		free(ptr);
		return NULL;
	}
}

#endif // _WIN32

// ======================================================================
//
//
// DasStk - linear stack of elements (LIFO)
//
//
// ======================================================================

DasBool _DasStk_init_with_alctor(_DasStkHeader** header_out, uintptr_t header_size, uintptr_t init_cap, DasAlctor alctor, uintptr_t elmt_size) {
	uintptr_t new_size = header_size + init_cap * elmt_size;
	_DasStkHeader* new_header = das_alloc(alctor, new_size, alignof(das_max_align_t));
	if (!new_header) return das_false;

	//
	// initialize the header and pass out the new header pointer
	new_header->count = 0;
	new_header->cap = init_cap;
	new_header->alctor = alctor;
	*header_out = new_header;
	return das_true;
}

void _DasStk_deinit(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t elmt_size) {
	_DasStkHeader* header = *header_in_out;
	uintptr_t size = header_size + header->cap * elmt_size;
	das_dealloc(header->alctor, header, size, alignof(das_max_align_t));
	*header_in_out = NULL;
}

uintptr_t _DasStk_assert_idx(_DasStkHeader* header, uintptr_t idx, uintptr_t elmt_size) {
	das_assert(idx < header->count, "idx '%zu' is out of bounds for a stack of count '%zu'", idx, header->count);
	return idx;
}

DasBool _DasStk_resize(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t new_count, DasBool zero, uintptr_t elmt_size) {
	_DasStkHeader* header = *header_in_out;
	//
	// extend the capacity of the stack if the new count extends past the capacity.
	if (header->cap < new_count) {
		DasBool res = _DasStk_resize_cap(header_in_out, header_size, das_max_u(new_count, header->cap * 2), elmt_size);
		if (!res) return das_false;
		header = *header_in_out;
	}

	//
	// zero the new memory if requested
	uintptr_t count = header->count;
	if (zero && new_count > count) {
		void* data = das_ptr_add(header, header_size);
		memset(das_ptr_add(data, count * elmt_size), 0, (new_count - count) * elmt_size);
	}

	header->count = new_count;
	return das_true;
}

DasBool _DasStk_resize_cap(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t new_cap, uintptr_t elmt_size) {
	_DasStkHeader* header = *header_in_out;
	uintptr_t cap = header ? header->cap : 0;
	uintptr_t count = header ? header->count : 0;
	DasAlctor alctor = header && header->alctor.fn != NULL ? header->alctor : DasAlctor_default;

	//
	// return if the capacity has not changed
	new_cap = das_max_u(das_max_u(DasStk_min_cap, new_cap), count);
	if (cap == new_cap) return das_true;

	//
	// reallocate the stack while taking into account of size of the header.
	// the block of memory is aligned to alignof(das_max_align_t).
	uintptr_t size = header_size + cap * elmt_size;
	uintptr_t new_size = header_size + new_cap * elmt_size;
	_DasStkHeader* new_header = das_realloc(alctor, header, size, new_size, alignof(das_max_align_t));
	if (!new_header) return das_false;
	if (!header) {
		new_header->alctor = alctor;
		new_header->count = 0;
	}

	//
	// update the capacity in the header and pass out the new header pointer
	new_header->cap = new_cap;
	*header_in_out = new_header;
	return das_true;
}

DasBool _DasStk_insert_many(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t idx, uintptr_t elmts_count, uintptr_t elmt_size) {
	_DasStkHeader* header = *header_in_out;
	das_assert(idx <= header->count, "insert idx '%zu' must be less than or equal to count of '%zu'", idx, header->count);
	uintptr_t count = header ? header->count : 0;
	uintptr_t cap = header ? header->cap : 0;

	//
	// increase the capacity if the number of elements
	// inserted makes the stack exceed it's capacity.
    uintptr_t new_count = count + elmts_count;
    if (cap < new_count) {
		DasBool res = _DasStk_resize_cap(header_in_out, header_size, das_max_u(new_count, cap * 2), elmt_size);
		if (!res) return das_false;
		header = *header_in_out;
    }

	void* data = das_ptr_add(header, header_size);
    void* dst = das_ptr_add(data, idx * elmt_size);

	// shift the elements from idx to (idx + elmts_count), to the right
	// to make room for the elements
    memmove(das_ptr_add(dst, elmts_count * elmt_size), dst, (header->count - idx) * elmt_size);

    header->count = new_count;
	return das_true;
}

uintptr_t _DasStk_push_many(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t elmts_count, uintptr_t elmt_size) {
	_DasStkHeader* header = *header_in_out;
	uintptr_t count = header ? header->count : 0;
	uintptr_t cap = header ? header->cap : 0;
	uintptr_t idx = count;

	//
	// increase the capacity if the number of elements
	// pushed on makes the stack exceed it's capacity.
	uintptr_t new_count = count + elmts_count;
	if (new_count > cap) {
		DasBool res = _DasStk_resize_cap(header_in_out, header_size, das_max_u(new_count, cap * 2), elmt_size);
		if (!res) return UINTPTR_MAX;
		header = *header_in_out;
	}

	header->count = new_count;
	return idx;
}

uintptr_t _DasStk_pop_many(_DasStkHeader* header, uintptr_t elmts_count, uintptr_t elmt_size) {
	elmts_count = das_min_u(elmts_count, header->count);
	header->count -= elmts_count;
	return elmts_count;
}

void _DasStk_remove_swap_range(_DasStkHeader* header, uintptr_t header_size, uintptr_t start_idx, uintptr_t end_idx, uintptr_t elmt_size) {
	das_assert(start_idx <= header->count, "start_idx '%zu' must be less than the count of '%zu'", start_idx, header->count);
	das_assert(end_idx <= header->count, "end_idx '%zu' must be less than or equal to count of '%zu'", end_idx, header->count);

	//
	// get the pointer where the elements are being removed
	uintptr_t remove_count = end_idx - start_idx;
	void* data = das_ptr_add(header, header_size);
	void* dst = das_ptr_add(data, start_idx * elmt_size);

	//
	// work out where the elements at the back of the stack start from.
	uintptr_t src_idx = header->count;
	header->count -= remove_count;
	if (remove_count > header->count) remove_count = header->count;
	src_idx -= remove_count;

	//
	// replace the removed elements with the elements from the back of the stack.
	void* src = das_ptr_add(data, src_idx * elmt_size);
	memmove(dst, src, remove_count * elmt_size);
}

void _DasStk_remove_shift_range(_DasStkHeader* header, uintptr_t header_size, uintptr_t start_idx, uintptr_t end_idx, uintptr_t elmt_size) {
	das_assert(start_idx <= header->count, "start_idx '%zu' must be less than the count of '%zu'", start_idx, header->count);
	das_assert(end_idx <= header->count, "end_idx '%zu' must be less than or equal to count of '%zu'", end_idx, header->count);

	//
	// get the pointer where the elements are being removed
	uintptr_t remove_count = end_idx - start_idx;
	void* data = das_ptr_add(header, header_size);
	void* dst = das_ptr_add(data, start_idx * elmt_size);

	//
	// now replace the elements by shifting the elements to the right over them.
	if (end_idx < header->count) {
		void* src = das_ptr_add(dst, remove_count * elmt_size);
		memmove(dst, src, (header->count - (start_idx + remove_count)) * elmt_size);
	}
	header->count -= remove_count;
}

uintptr_t DasStk_push_str(DasStk(char)* stk, char* str) {
	uintptr_t len = strlen(str);
	uintptr_t idx = DasStk_push_many(stk, len);
	memcpy(DasStk_get(stk, idx), str, len);
	return idx;
}

uintptr_t DasStk_push_str_fmtv(DasStk(char)* stk, char* fmt, va_list args) {
	va_list args_copy;
	va_copy(args_copy, args);

	// add 1 so we have enough room for the null terminator that vsnprintf always outputs
	// vsnprintf will return -1 on an encoding error.
	uintptr_t count = vsnprintf(NULL, 0, fmt, args_copy) + 1;
	va_end(args_copy);
	das_assert(count >= 1, "a vsnprintf encoding error has occurred");

	//
	// resize the stack to have enough room to store the pushed formatted string
	uintptr_t insert_idx = DasStk_count(stk);
	uintptr_t new_count = DasStk_count(stk) + count;
	DasBool res = DasStk_resize_cap(stk, new_count);
	if (!res) return das_false;

	//
	// now call vsnprintf for real this time, with a buffer
	// to actually copy the formatted string.
	char* ptr = das_ptr_add(DasStk_data(stk), insert_idx);
	count = vsnprintf(ptr, count, fmt, args);
	return insert_idx;
}

uintptr_t DasStk_push_str_fmt(DasStk(char)* stk, char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	uintptr_t idx = DasStk_push_str_fmtv(stk, fmt, args);
	va_end(args);
	return idx;
}

// ======================================================================
//
//
// DasDeque - double ended queue (ring buffer)
//
//
// ======================================================================

static inline uintptr_t _DasDeque_wrapping_add(uintptr_t cap, uintptr_t idx, uintptr_t value) {
    uintptr_t res = idx + value;
    if (res >= cap) res = value - (cap - idx);
    return res;
}

static inline uintptr_t _DasDeque_wrapping_sub(uintptr_t cap, uintptr_t idx, uintptr_t value) {
    uintptr_t res = idx - value;
    if (res > idx) res = cap - (value - idx);
    return res;
}

DasBool _DasDeque_init_with_alctor(_DasDequeHeader** header_out, uintptr_t header_size, uintptr_t init_cap, DasAlctor alctor, uintptr_t elmt_size) {
	// add one because the back_idx needs to point to the next empty element slot.
	init_cap = das_max_u(DasDeque_min_cap, init_cap);
	init_cap += 1;

	uintptr_t new_size = header_size + init_cap * elmt_size;
	_DasDequeHeader* new_header = das_alloc(alctor, new_size, alignof(das_max_align_t));
	if (!new_header) return das_false;

	//
	// initialize the header and pass out the new header pointer
	new_header->cap = init_cap;
	new_header->front_idx = 0;
	new_header->back_idx = 0;
	new_header->alctor = alctor;
	*header_out = new_header;
	return das_true;

}


void _DasDeque_deinit(_DasDequeHeader** header_in_out, uintptr_t header_size, uintptr_t elmt_size) {
	_DasDequeHeader* header = *header_in_out;
	uintptr_t size = header_size + header->cap * elmt_size;
	das_dealloc(header->alctor, header, size, alignof(das_max_align_t));
	*header_in_out = NULL;
}

uintptr_t _DasDeque_assert_idx(_DasDequeHeader* header, uintptr_t idx, uintptr_t elmt_size) {
	uintptr_t count = DasDeque_count(&header);
	das_assert(idx < count, "idx '%zu' is out of bounds for a deque of count '%zu'", idx, count);
    idx = _DasDeque_wrapping_add(header->cap, header->front_idx, idx);
    return idx;
}

DasBool _DasDeque_resize_cap(_DasDequeHeader** header_in_out, uintptr_t header_size, uintptr_t new_cap, uintptr_t elmt_size) {
	_DasDequeHeader* header = *header_in_out;
	uintptr_t cap = header ? header->cap : 0;
	new_cap = das_max_u(das_max_u(DasDeque_min_cap, new_cap), DasDeque_count(header_in_out));
	// add one because the back_idx needs to point to the next empty element slot.
	new_cap += 1;
	if (new_cap == cap) return das_true;

	DasAlctor alctor = header && header->alctor.fn != NULL ? header->alctor : DasAlctor_default;

	uintptr_t size = header_size + cap * elmt_size;
	uintptr_t new_size = header_size + new_cap * elmt_size;
	_DasDequeHeader* new_header = das_realloc(alctor, header, size, new_size, alignof(das_max_align_t));
	if (!new_header) return das_false;
	if (!header) {
		new_header->alctor = alctor;
		new_header->front_idx = 0;
		new_header->back_idx = 0;
	}

	new_header->alctor = alctor;
	uintptr_t old_cap = new_header->cap;
	new_header->cap = new_cap;
	*header_in_out = new_header;

	void* data = das_ptr_add(new_header, header_size);

	// move the front_idx and back_idx around to resolve the gaps that could have been created after resizing the buffer
	//
    // A - no gaps created so no need to change
    // --------
    //   F     B        F     B
    // [ V V V . ] -> [ V V V . . . . ]
    //
    // B - less elements before back_idx than elements from front_idx, so copy back_idx after the front_idx
    //       B F                  F         B
    // [ V V . V V V ] -> [ . . . V V V V V . . . . . ]
    //
    // C - more elements before back_idx than elements from front_idx, so copy front_idx to the end
    //       B F           B           F
    // [ V V . V] -> [ V V . . . . . . V ]
    //
    if (new_header->front_idx <= new_header->back_idx) { // A
		// do nothing
    } else if (new_header->back_idx  < old_cap - new_header->front_idx) { // B
        memcpy(das_ptr_add(data, old_cap * elmt_size), data, new_header->back_idx * elmt_size);
        new_header->back_idx += old_cap;
        das_debug_assert(new_header->back_idx > new_header->front_idx, "back_idx must come after front_idx");
    } else { // C
        uintptr_t new_front_idx = new_cap - (old_cap - new_header->front_idx);
        memcpy(das_ptr_add(data, new_front_idx * elmt_size), das_ptr_add(data, new_header->front_idx * elmt_size), (old_cap - new_header->front_idx) * elmt_size);
        new_header->front_idx = new_front_idx;
        das_debug_assert(new_header->back_idx < new_header->front_idx, "front_idx must come after back_idx");
    }

	das_debug_assert(new_header->back_idx < new_cap, "back_idx must remain in bounds");
	das_debug_assert(new_header->front_idx < new_cap, "front_idx must remain in bounds");
	return das_true;
}

void _DasDeque_read(_DasDequeHeader* header, uintptr_t header_size, uintptr_t idx, void* elmts_out, uintptr_t elmts_count, uintptr_t elmt_size) {
	uintptr_t count = DasDeque_count(&header);
	das_assert(idx + elmts_count <= count, "idx '%zu' and elmts_count '%zu' will go out of bounds for a deque of count '%zu'", idx, count);

    idx = _DasDeque_wrapping_add(header->cap, header->front_idx, idx);

	void* data = das_ptr_add(header, header_size);
	if (header->cap < idx + elmts_count) {
		//
		// there is enough elements that the read will
		// exceed the back and cause the idx to loop around.
		// so copy in two parts
		uintptr_t rem_count = header->cap - idx;
		// copy to the end of the buffer
		memcpy(elmts_out, das_ptr_add(data, idx * elmt_size), rem_count * elmt_size);
		// copy to the beginning of the buffer
		memcpy(das_ptr_add(elmts_out, rem_count * elmt_size), data, ((elmts_count - rem_count) * elmt_size));
	} else {
		//
		// coping the elements can be done in a single copy
		memcpy(elmts_out, das_ptr_add(data, (header->front_idx - elmts_count) * elmt_size), elmts_count * elmt_size);
	}
}

void _DasDeque_write(_DasDequeHeader* header, uintptr_t header_size, uintptr_t idx, void* elmts, uintptr_t elmts_count, uintptr_t elmt_size) {
	uintptr_t count = DasDeque_count(&header);
	das_assert(idx + elmts_count <= count, "idx '%zu' and elmts_count '%zu' will go out of bounds for a deque of count '%zu'", idx, count);

    idx = _DasDeque_wrapping_add(header->cap, header->front_idx, idx);

	void* data = das_ptr_add(header, header_size);
	if (header->cap < idx + elmts_count) {
		//
		// there is enough elements that the write will
		// exceed the back and cause the idx to loop around.
		// so copy in two parts
		uintptr_t rem_count = header->cap - idx;
		// copy to the end of the buffer
		memcpy(das_ptr_add(data, idx * elmt_size), elmts, rem_count * elmt_size);
		// copy to the beginning of the buffer
		memcpy(data, das_ptr_add(elmts, rem_count * elmt_size), ((elmts_count - rem_count) * elmt_size));
	} else {
		//
		// coping the elements can be done in a single copy
		memcpy(das_ptr_add(data, (header->front_idx - elmts_count) * elmt_size), elmts, elmts_count * elmt_size);
	}
}

DasBool _DasDeque_push_front_many(_DasDequeHeader** header_in_out, uintptr_t header_size, uintptr_t elmts_count, uintptr_t elmt_size) {
	_DasDequeHeader* header = *header_in_out;
	uintptr_t new_count = DasDeque_count(header_in_out) + elmts_count;
	uintptr_t cap = header ? header->cap : 0;
	if (cap < new_count + 1) {
		DasBool res = _DasDeque_resize_cap(header_in_out, header_size, das_max_u(cap * 2, new_count), elmt_size);
		if (!res) return das_false;
		header = *header_in_out;
	}

	header->front_idx = _DasDeque_wrapping_sub(header->cap, header->front_idx, elmts_count);
	return das_true;
}

DasBool _DasDeque_push_back_many(_DasDequeHeader** header_in_out, uintptr_t header_size, uintptr_t elmts_count, uintptr_t elmt_size) {
	_DasDequeHeader* header = *header_in_out;
	uintptr_t new_count = DasDeque_count(header_in_out) + elmts_count;
	uintptr_t cap = header ? header->cap : 0;
	if (cap < new_count + 1) {
		DasBool res = _DasDeque_resize_cap(header_in_out, header_size, das_max_u(cap * 2, new_count), elmt_size);
		if (!res) return das_false;
		header = *header_in_out;
	}

	header->back_idx = _DasDeque_wrapping_add(header->cap, header->back_idx, elmts_count);
	return das_true;
}

uintptr_t _DasDeque_pop_front_many(_DasDequeHeader* header, uintptr_t elmts_count, uintptr_t elmt_size) {
    if (header->front_idx == header->back_idx) return 0;
	elmts_count = das_min_u(elmts_count, DasDeque_count(&header));

	header->front_idx = _DasDeque_wrapping_add(header->cap, header->front_idx, elmts_count);
	return elmts_count;
}

uintptr_t _DasDeque_pop_back_many(_DasDequeHeader* header, uintptr_t elmts_count, uintptr_t elmt_size) {
    if (header->front_idx == header->back_idx) return 0;
	elmts_count = das_min_u(elmts_count, DasDeque_count(&header));

	header->back_idx = _DasDeque_wrapping_sub(header->cap, header->back_idx, elmts_count);
	return elmts_count;
}
