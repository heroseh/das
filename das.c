#ifndef DAS_H
#include "das.h"
#endif

#ifdef __linux__
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#endif

// ======================================================================
//
//
// General
//
//
// ======================================================================

#include <stdarg.h> // va_start, va_end

das_noreturn void _das_abort(const char* file, int line, const char* func, char* assert_test, char* message_fmt, ...) {
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
		_aligned_free(ptr);
		return NULL;
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
	init_cap = das_max_u(DasStk_min_cap, init_cap);
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

DasBool _DasStk_init_clone_with_alctor(_DasStkHeader** dst_header_in_out, uintptr_t header_size, _DasStkHeader* src_header, DasAlctor alctor, uintptr_t elmt_size) {
	uintptr_t src_count = DasStk_count(&src_header);
	if (!_DasStk_init_with_alctor(dst_header_in_out, header_size, src_count, alctor, elmt_size))
		return das_false;

	_DasStkHeader* dst_header = *dst_header_in_out;

	void* dst = das_ptr_add(dst_header, header_size);
	void* src = das_ptr_add(src_header, header_size);
	memcpy(dst, src, src_count * elmt_size);
	dst_header->count = src_count;
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
	uintptr_t old_cap = DasStk_cap(header_in_out);
	if (old_cap < new_count) {
		DasBool res = _DasStk_resize_cap(header_in_out, header_size, das_max_u(new_count, old_cap * 2), elmt_size);
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
	uintptr_t size = header ? header_size + cap * elmt_size : 0;
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

void* _DasStk_insert_many(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t idx, void* elmts, uintptr_t elmts_count, uintptr_t elmt_size) {
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
		if (!res) return NULL;
		header = *header_in_out;
    }

	void* data = das_ptr_add(header, header_size);
    void* dst = das_ptr_add(data, idx * elmt_size);

	// shift the elements from idx to (idx + elmts_count), to the right
	// to make room for the elements
    memmove(das_ptr_add(dst, elmts_count * elmt_size), dst, (header->count - idx) * elmt_size);

	if (elmts) {
		memcpy(dst, elmts, elmts_count * elmt_size);
	}

    header->count = new_count;
	return dst;
}

void* _DasStk_push_many(_DasStkHeader** header_in_out, uintptr_t header_size, void* elmts, uintptr_t elmts_count, uintptr_t elmt_size) {
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
		if (!res) return NULL;
		header = *header_in_out;
	}

	void* dst = das_ptr_add(header, header_size + idx * elmt_size);
	if (elmts) {
		memcpy(dst, elmts, elmts_count * elmt_size);
	}

	header->count = new_count;
	return dst;
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

char* DasStk_push_str(DasStk(char)* stk, char* str) {
	uintptr_t len = strlen(str);
	return DasStk_push_many(stk, str, len);
}

void* DasStk_push_str_fmtv(DasStk(char)* stk, char* fmt, va_list args) {
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
	if (!res) return NULL;

	//
	// now call vsnprintf for real this time, with a buffer
	// to actually copy the formatted string.
	char* ptr = das_ptr_add(DasStk_data(stk), insert_idx);
	count = vsnprintf(ptr, count, fmt, args);
	return ptr;
}

void* DasStk_push_str_fmt(DasStk(char)* stk, char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	void* ptr = DasStk_push_str_fmtv(stk, fmt, args);
	va_end(args);
	return ptr;
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

	uintptr_t size = header ? header_size + cap * elmt_size : 0;
	uintptr_t new_size = header_size + new_cap * elmt_size;
	_DasDequeHeader* new_header = das_realloc(alctor, header, size, new_size, alignof(das_max_align_t));
	if (!new_header) return das_false;
	if (!header) {
		new_header->alctor = alctor;
		new_header->front_idx = 0;
		new_header->back_idx = 0;
		new_header->cap = 0;
	}

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
		memcpy(elmts_out, das_ptr_add(data, idx * elmt_size), elmts_count * elmt_size);
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
		memcpy(das_ptr_add(data, idx * elmt_size), elmts, elmts_count * elmt_size);
	}
}

uintptr_t _DasDeque_push_front_many(_DasDequeHeader** header_in_out, uintptr_t header_size, void* elmts, uintptr_t elmts_count, uintptr_t elmt_size) {
	_DasDequeHeader* header = *header_in_out;
	uintptr_t new_count = DasDeque_count(header_in_out) + elmts_count;
	uintptr_t cap = header ? header->cap : 0;
	if (cap < new_count + 1) {
		DasBool res = _DasDeque_resize_cap(header_in_out, header_size, das_max_u(cap * 2, new_count), elmt_size);
		if (!res) return UINTPTR_MAX;
		header = *header_in_out;
	}

	header->front_idx = _DasDeque_wrapping_sub(header->cap, header->front_idx, elmts_count);

	if (elmts) {
		_DasDeque_write(header, header_size, 0, elmts, elmts_count, elmt_size);
	}

	return 0;
}

uintptr_t _DasDeque_push_back_many(_DasDequeHeader** header_in_out, uintptr_t header_size, void* elmts, uintptr_t elmts_count, uintptr_t elmt_size) {
	_DasDequeHeader* header = *header_in_out;
	uintptr_t insert_idx = DasDeque_count(header_in_out);
	uintptr_t new_count = insert_idx + elmts_count;
	uintptr_t cap = header ? header->cap : 0;
	if (cap < new_count + 1) {
		DasBool res = _DasDeque_resize_cap(header_in_out, header_size, das_max_u(cap * 2, new_count), elmt_size);
		if (!res) return UINTPTR_MAX;
		header = *header_in_out;
	}

	header->back_idx = _DasDeque_wrapping_add(header->cap, header->back_idx, elmts_count);

	if (elmts) {
		_DasDeque_write(header, header_size, insert_idx, elmts, elmts_count, elmt_size);
	}

	return insert_idx;
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

// ===========================================================================
//
//
// Platform Error Handling
//
//
// ===========================================================================

DasError _das_get_last_error() {
#ifdef __linux__
	return errno;
#elif _WIN32
    return GetLastError();
#else
#error "unimplemented virtual memory API for this platform"
#endif
}

DasErrorStrRes das_get_error_string(DasError error, char* buf_out, uint32_t buf_out_len) {
#if _WIN32
	DWORD res = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, (LPTSTR)buf_out, buf_out_len, NULL);
	if (res == 0) {
		error = GetLastError();
		das_abort("TODO handle error code: %u", error);
	}
#elif _GNU_SOURCE
	// GNU version (screw these guys for changing the way this works)
	char* buf = strerror_r(error, buf_out, buf_out_len);
	if (strcmp(buf, "Unknown error") == 0) {
		return DasErrorStrRes_invalid_error_arg;
	}

	// if its static string then copy it to the buffer
	if (buf != buf_out) {
		strncpy(buf_out, buf, buf_out_len);

		uint32_t len = strlen(buf);
		if (len < buf_out_len) {
			return DasErrorStrRes_not_enough_space_in_buffer;
		}
	}
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	// use the XSI standard behavior.
	int res = strerror_r(error, buf_out, buf_out_len);
	if (res != 0) {
		int errnum = res;
		if (res == -1)
			errnum = errno;

		if (errnum == EINVAL) {
			return DasErrorStrRes_invalid_error_arg;
		} else if (errnum == ERANGE) {
			return DasErrorStrRes_not_enough_space_in_buffer;
		}
		das_abort("unexpected errno: %u", errnum);
	}
#else
#error "unimplemented virtual memory error string"
#endif
	return DasErrorStrRes_success;
}

// ===========================================================================
//
//
// File Abstraction
//
//
// ===========================================================================

DasError das_file_open(char* path, DasFileFlags flags, DasFileHandle* file_handle_out) {
	das_assert(
		(flags & (DasFileFlags_read | DasFileFlags_write | DasFileFlags_append)),
		"DasFileFlags_{read, write or append} must be set when opening a file"
	);

	das_assert(
		(flags & (DasFileFlags_write | DasFileFlags_append))
			|| !(flags & (DasFileFlags_create_if_not_exist | DasFileFlags_create_new | DasFileFlags_truncate)),
		"file must be opened with DasFileFlags_{write or append} if DasFileFlags_{create_if_not_exist, create_new or truncate} exists"
	);

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	int fd_flags = O_CLOEXEC;

	if (flags & DasFileFlags_create_new) {
		fd_flags |= O_CREAT | O_EXCL;
	} else {
		if (flags & DasFileFlags_create_if_not_exist) {
			fd_flags |= O_CREAT;
		}

		if (flags & DasFileFlags_truncate) {
			fd_flags |= O_TRUNC;
		}
	}

	if ((flags & DasFileFlags_read) && (flags & DasFileFlags_write)) {
		fd_flags |= O_RDWR;
		if (flags & DasFileFlags_append) {
			fd_flags |= O_APPEND;
		}
	} else if (flags & DasFileFlags_read) {
		fd_flags |= O_RDONLY;
	} else if (flags & DasFileFlags_write) {
		fd_flags |= O_WRONLY;
		if (flags & DasFileFlags_append) {
			fd_flags |= O_APPEND;
		}
	}

	mode_t mode = 0666; // TODO allow the user to supply a mode.
	int fd = open(path, fd_flags, mode);
	if (fd == -1) return _das_get_last_error();

	*file_handle_out = (DasFileHandle) { .raw = fd };
#elif _WIN32
	DWORD win_access = 0;
	DWORD win_creation = 0;

	if (flags & DasFileFlags_create_new) {
		win_creation |= CREATE_NEW;
	} else {
		if ((flags & DasFileFlags_create_if_not_exist) && (flags & DasFileFlags_truncate)) {
			win_creation |= CREATE_ALWAYS;
		} else if (flags & DasFileFlags_create_if_not_exist) {
			win_creation |= OPEN_ALWAYS;
		} else if (flags & DasFileFlags_truncate) {
			win_creation |= TRUNCATE_EXISTING;
		}
	}

	if (win_creation == 0)
		win_creation = OPEN_EXISTING;

	if (flags & DasFileFlags_read) {
		win_access |= GENERIC_READ;
	}

	if (flags & DasFileFlags_write) {
		if (flags & DasFileFlags_append) {
			win_access |= FILE_GENERIC_WRITE & ~FILE_WRITE_DATA;
		} else {
			win_access |= GENERIC_WRITE;
		}
	}

	HANDLE handle = CreateFileA(path, win_access, 0, NULL, win_creation, FILE_ATTRIBUTE_NORMAL, 0);
	if (handle == INVALID_HANDLE_VALUE)
		return _das_get_last_error();

	*file_handle_out = (DasFileHandle) { .raw = handle };
#endif

	return DasError_success;
}

DasError das_file_close(DasFileHandle handle) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	if (close(handle.raw) != 0)
		return _das_get_last_error();
#elif _WIN32
	if (CloseHandle(handle.raw) == 0)
		return _das_get_last_error();
#else
#error "unimplemented file API for this platform"
#endif

	return DasError_success;
}

DasError das_file_size(DasFileHandle handle, uint64_t* size_out) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	// get the size of the file
	struct stat s = {0};
	if (fstat(handle.raw, &s) != 0) return _das_get_last_error();
	*size_out = s.st_size;
#elif _WIN32
	LARGE_INTEGER size;
	if (!GetFileSizeEx(handle.raw, &size)) return _das_get_last_error();
	*size_out = size.QuadPart;
#else
#error "unimplemented file API for this platform"
#endif
	return DasError_success;
}

DasError das_file_read(DasFileHandle handle, void* data_out, uintptr_t length, uintptr_t* bytes_read_out) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	ssize_t bytes_read = read(handle.raw, data_out, length);
	if (bytes_read == (ssize_t)-1)
		return _das_get_last_error();
#elif _WIN32
	length = das_min_u(length, UINT32_MAX);
	DWORD bytes_read;
	if (ReadFile(handle.raw, data_out, length, &bytes_read, NULL) == 0)
		return _das_get_last_error();
#else
#error "unimplemented file API for this platform"
#endif

	*bytes_read_out = bytes_read;
	return DasError_success;
}

DasError das_file_read_exact(DasFileHandle handle, void* data_out, uintptr_t length, uintptr_t* bytes_read_out) {
	uintptr_t og_length = length;
	while (length) {
		uintptr_t bytes_read;
		DasError error = das_file_read(handle, data_out, length, &bytes_read);
		if (error) return error;

		if (bytes_read == 0)
			break;

		data_out = das_ptr_add(data_out, bytes_read);
		length -= bytes_read;
	}

	*bytes_read_out = og_length - length;
	return DasError_success;
}

DasError das_file_write(DasFileHandle handle, void* data, uintptr_t length, uintptr_t* bytes_written_out) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	ssize_t bytes_written = write(handle.raw, data, length);
	if (bytes_written == (ssize_t)-1)
		return _das_get_last_error();
#elif _WIN32
	length = das_min_u(length, UINT32_MAX);
	DWORD bytes_written;
	if (WriteFile(handle.raw, data, length, &bytes_written, NULL) == 0)
		return _das_get_last_error();
#else
#error "unimplemented file API for this platform"
#endif

	*bytes_written_out = bytes_written;
	return DasError_success;
}

DasError das_file_write_exact(DasFileHandle handle, void* data, uintptr_t length, uintptr_t* bytes_written_out) {
	uintptr_t og_length = length;
	while (length) {
		uintptr_t bytes_written;
		DasError error = das_file_write(handle, data, length, &bytes_written);
		if (error) return error;

		if (bytes_written == 0)
			break;

		data = das_ptr_add(data, bytes_written);
		length -= bytes_written;
	}

	*bytes_written_out = og_length - length;
	return DasError_success;
}

DasError das_file_seek(DasFileHandle handle, int64_t offset, DasFileSeekFrom from, uint64_t* cursor_offset_out) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	int whence = 0;
	switch (from) {
		case DasFileSeekFrom_start: whence = SEEK_SET; break;
		case DasFileSeekFrom_current: whence = SEEK_CUR; break;
		case DasFileSeekFrom_end: whence = SEEK_END; break;
	}
	off_t cursor_offset = lseek(handle.raw, offset, whence);
	if (cursor_offset == (off_t)-1)
		return _das_get_last_error();
	*cursor_offset_out = cursor_offset;
#elif _WIN32
	int move_method = 0;
	switch (from) {
		case DasFileSeekFrom_start: move_method = FILE_BEGIN; break;
		case DasFileSeekFrom_current: move_method = FILE_CURRENT; break;
		case DasFileSeekFrom_end: move_method = FILE_END; break;
	}
	LARGE_INTEGER distance_to_move;
	distance_to_move.QuadPart = offset;
	LARGE_INTEGER new_offset = {0};

	if (!SetFilePointerEx(handle.raw, distance_to_move, &new_offset, move_method))
		return _das_get_last_error();

	*cursor_offset_out = new_offset.QuadPart;
#else
#error "unimplemented file API for this platform"
#endif
	return DasError_success;
}

DasError das_file_flush(DasFileHandle handle) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	if (fsync(handle.raw) != 0)
		return _das_get_last_error();
#elif _WIN32
	if (FlushFileBuffers(handle.raw) == 0)
		return _das_get_last_error();
#else
#error "unimplemented file API for this platform"
#endif
	return DasError_success;
}

// ===========================================================================
//
//
// Virtual Memory Abstraction
//
//
// ===========================================================================

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
static int _das_virt_mem_prot_unix(DasVirtMemProtection prot) {
	switch (prot) {
		case DasVirtMemProtection_no_access: return 0;
		case DasVirtMemProtection_read: return PROT_READ;
		case DasVirtMemProtection_read_write: return PROT_READ | PROT_WRITE;
		case DasVirtMemProtection_exec_read: return PROT_EXEC | PROT_READ;
		case DasVirtMemProtection_exec_read_write: return PROT_READ | PROT_WRITE | PROT_EXEC;
	}
	return 0;
}
#elif _WIN32
static DWORD _das_virt_mem_prot_windows(DasVirtMemProtection prot) {
	switch (prot) {
		case DasVirtMemProtection_no_access: return PAGE_NOACCESS;
		case DasVirtMemProtection_read: return PAGE_READONLY;
		case DasVirtMemProtection_read_write: return PAGE_READWRITE;
		case DasVirtMemProtection_exec_read: return PAGE_EXECUTE_READ;
		case DasVirtMemProtection_exec_read_write: return PAGE_EXECUTE_READWRITE;
	}
	return 0;
}
#else
#error "unimplemented virtual memory API for this platform"
#endif

DasError das_virt_mem_page_size(uintptr_t* page_size_out, uintptr_t* reserve_align_out) {
#ifdef __linux__
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size ==  (long)-1)
		return _das_get_last_error();

	*page_size_out = page_size;
	*reserve_align_out = page_size;
#elif _WIN32
	SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
	*page_size_out = si.dwPageSize;
	*reserve_align_out = si.dwAllocationGranularity;
#else
#error "unimplemented virtual memory API for this platform"
#endif

	return DasError_success;
}

DasError das_virt_mem_reserve(void* requested_addr, uintptr_t size, void** addr_out) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	// memory is automatically commited on Unix based OSs,
	// so we will restrict the memory from being accessed on reserved.
	int prot = 0;

	// MAP_ANON = means map physical memory and not a file. it also means the memory will be initialized to zero
	// MAP_PRIVATE = keep memory private so child process cannot access them
	// MAP_NORESERVE = do not reserve any swap space for this mapping
	void* addr = mmap(requested_addr, size, prot, MAP_ANON | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
	if (addr == MAP_FAILED)
		return _das_get_last_error();
#elif _WIN32
	void* addr = VirtualAlloc(requested_addr, size, MEM_RESERVE, PAGE_NOACCESS);
	if (addr == NULL)
		return _das_get_last_error();
#else
#error "TODO implement virtual memory for this platform"
#endif

	*addr_out = addr;
	return DasError_success;
}

DasError das_virt_mem_commit(void* addr, uintptr_t size, DasVirtMemProtection protection) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	// memory is automatically commited on Unix based OSs,
	// memory is restricted from being accessed in our das_virt_mem_reserve.
	// so lets just apply the protection for the address space.
	// and then advise the OS that these pages will be used soon.
	int prot = _das_virt_mem_prot_unix(protection);
	if (mprotect(addr, size, prot) != 0) return _das_get_last_error();
	if (madvise(addr, size, MADV_WILLNEED) != 0) return _das_get_last_error();
#elif _WIN32
	DWORD prot = _das_virt_mem_prot_windows(protection);
	if (VirtualAlloc(addr, size, MEM_COMMIT, prot) == NULL)
		return _das_get_last_error();
#else
#error "TODO implement virtual memory for this platform"
#endif
	return DasError_success;
}

DasError das_virt_mem_protection_set(void* addr, uintptr_t size, DasVirtMemProtection protection) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	int prot = _das_virt_mem_prot_unix(protection);
	if (mprotect(addr, size, prot) != 0)
		return _das_get_last_error();
#elif _WIN32
	DWORD prot = _das_virt_mem_prot_windows(protection);
	DWORD old_prot; // unused
	if (!VirtualProtect(addr, size, prot, &old_prot))
		return _das_get_last_error();
#else
#error "TODO implement virtual memory for this platform"
#endif
	return DasError_success;
}

DasError das_virt_mem_decommit(void* addr, uintptr_t size) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

	//
	// advise the OS that these pages will not be needed.
	// the OS will zero fill these pages before you next access them.
	if (madvise(addr, size, MADV_DONTNEED) != 0)
		return _das_get_last_error();

	// memory is automatically commited on Unix based OSs,
	// so we will restrict the memory from being accessed when we "decommit.
	int prot = 0;
	if (mprotect(addr, size, prot) != 0)
		return _das_get_last_error();
#elif _WIN32
	if (VirtualFree(addr, size, MEM_DECOMMIT) == 0)
		return _das_get_last_error();
#else
#error "TODO implement virtual memory for this platform"
#endif
	return DasError_success;
}

DasError das_virt_mem_release(void* addr, uintptr_t size) {
#ifdef __linux__
	if (munmap(addr, size) != 0)
		return _das_get_last_error();
#elif _WIN32
	//
	// unfortunately on Windows all memory must be release at once
	// that was reserved with VirtualAlloc.
	if (!VirtualFree(addr, 0, MEM_RELEASE))
		return _das_get_last_error();
#else
#error "TODO implement virtual memory for this platform"
#endif
	return DasError_success;
}

DasError das_virt_mem_map_file(void* requested_addr, DasFileHandle file_handle, DasVirtMemProtection protection, uint64_t offset, uintptr_t size, void** addr_out, DasMapFileHandle* map_file_handle_out) {
	das_assert(protection != DasVirtMemProtection_no_access, "cannot map a file with no access");

	uintptr_t reserve_align;
	uintptr_t page_size;
	DasError error = das_virt_mem_page_size(&page_size, &reserve_align);
	if (error) return error;

	//
	// round down the offset to the nearest multiple of reserve_align
	//
	uint64_t offset_diff = offset % reserve_align;
	offset -= offset_diff;

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	*map_file_handle_out = NULL;
	int prot = _das_virt_mem_prot_unix(protection);

	size = das_round_up_nearest_multiple_u(size, page_size);
	void* addr = mmap(requested_addr, size, prot, MAP_SHARED, file_handle.raw, offset);
	if (addr == MAP_FAILED)
		return _das_get_last_error();
#elif _WIN32

	DWORD prot = _das_virt_mem_prot_windows(protection);

	DWORD size_high = size >> 32;
	DWORD size_low = size;

	// create a file mapping object for the file
	HANDLE map_file_handle = CreateFileMappingA(file_handle.raw, NULL, prot, 0, 0, NULL);
	if (map_file_handle == NULL)
		return _das_get_last_error();
	*map_file_handle_out = map_file_handle;

	DWORD access = 0;
	switch (protection) {
		case DasVirtMemProtection_exec_read:
		case DasVirtMemProtection_read:
			access = FILE_MAP_READ;
			break;
		case DasVirtMemProtection_exec_read_write:
		case DasVirtMemProtection_read_write:
			access = FILE_MAP_ALL_ACCESS;
			break;
	}

	DWORD offset_high = offset >> 32;
	DWORD offset_low = offset;

	void* addr = MapViewOfFile(map_file_handle, access, offset_high, offset_low, size);
	if (addr == NULL)
		return _das_get_last_error();
#else
#error "TODO implement virtual memory for this platform"
#endif

	//
	// move the pointer to where the user's request offset into the file will be.
	addr = das_ptr_add(addr, offset_diff);
	*addr_out = addr;

	return DasError_success;
}

DasError das_virt_mem_unmap_file(void* addr, uintptr_t size, DasMapFileHandle map_file_handle) {
	uintptr_t reserve_align;
	uintptr_t page_size;
	DasError error = das_virt_mem_page_size(&page_size, &reserve_align);
	if (error) return error;

	//
	// when mapping a file the user is given an offset from the start of the range pages that are mapped
	// to match the file offset they requested.
	// so round down to the address to the nearest reserve align.
	addr = das_ptr_round_down_align(addr, reserve_align);

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	size = das_round_up_nearest_multiple_u(size, page_size);
	return das_virt_mem_release(addr, size);
#elif _WIN32

	if (!UnmapViewOfFile(addr))
		return _das_get_last_error();

	if (!CloseHandle(map_file_handle))
		return _das_get_last_error();

	return DasError_success;
#else
#error "TODO implement virtual memory for this platform"
#endif
}

// ===========================================================================
//
//
// Linear Allocator
//
//
// ===========================================================================

DasError DasLinearAlctor_init(DasLinearAlctor* alctor, uintptr_t reserved_size, uintptr_t commit_grow_size) {
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

DasError DasLinearAlctor_deinit(DasLinearAlctor* alctor) {
	return das_virt_mem_release(alctor->address_space, alctor->reserved_size);
}

static DasBool _DasLinearAlctor_commit_next_chunk(DasLinearAlctor* alctor) {
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

void* DasLinearAlctor_alloc_fn(void* alctor_data, void* ptr, uintptr_t old_size, uintptr_t size, uintptr_t align) {
	DasLinearAlctor* alctor = (DasLinearAlctor*)alctor_data;
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
				if (!_DasLinearAlctor_commit_next_chunk(alctor))
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
					if (!_DasLinearAlctor_commit_next_chunk(alctor))
						return NULL;
				}
			}
		}

		// if we cannot extend in place, then just allocate a new block.
		void* new_ptr = DasLinearAlctor_alloc_fn(alctor, NULL, 0, size, align);
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

// ===========================================================================
//
//
// Pool
//
//
// ===========================================================================

static inline _DasPoolRecord* _DasPool_records(_DasPool* pool, uintptr_t elmt_size) {
	return das_ptr_add(pool->address_space, (uintptr_t)pool->reserved_cap * elmt_size);
}

static inline DasPoolElmtId _DasPool_record_to_id(_DasPool* pool, _DasPoolRecord* record, uint32_t idx_id, uint32_t index_bits) {
	DasPoolElmtId index_mask = (1 << index_bits) - 1;
	DasPoolElmtId id = record->next_id;
	id &= ~index_mask; // clear out the index id that points to the next record
	id |= idx_id; // now store the index id that points to this record
	return id;
}

static void _DasPool_assert_id(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits) {
	das_assert(elmt_id, "the element id cannot be null");
	das_assert(elmt_id & DasPoolElmtId_is_allocated_bit_MASK, "the provided element identifier does not have the allocated bit set.");
	DasPoolElmtId index_mask = (1 << index_bits) - 1;
	uint32_t idx_id = elmt_id & index_mask;
	das_assert(idx_id, "the index identifier cannot be null");
	_DasPoolRecord* record = &_DasPool_records(pool, elmt_size)[idx_id - 1];
	das_assert(record->next_id & DasPoolElmtId_is_allocated_bit_MASK, "the record is not allocated");

	DasPoolElmtId counter_mask = DasPoolElmtId_counter_mask(index_bits);
	uint32_t counter = (elmt_id & counter_mask) >> index_bits;
	uint32_t record_counter = (record->next_id & counter_mask) >> index_bits;
	das_assert(counter == record_counter, "use after free detected... the provided element identifier has a counter of '%u' but the internal one is '%u'", counter, record_counter);
}

DasError _DasPool_init(_DasPool* pool, uint32_t reserved_cap, uint32_t commit_grow_count, uintptr_t elmt_size) {
	das_zero_elmt(pool);

	uintptr_t reserve_align;
	uintptr_t page_size;
	DasError error = das_virt_mem_page_size(&page_size, &reserve_align);
	if (error) return error;

	//
	// reserve the whole address space for the elements array and the records array.
	uintptr_t elmts_size = das_round_up_nearest_multiple_u((uintptr_t)reserved_cap * elmt_size, reserve_align);
	uintptr_t records_size = das_round_up_nearest_multiple_u((uintptr_t)reserved_cap * sizeof(_DasPoolRecord), reserve_align);
	uintptr_t reserved_size = elmts_size + records_size;
	error = das_virt_mem_reserve(NULL, reserved_size, &pool->address_space);
	if (error) return error;

	//
	// see how many elements we can actually fit in the rounded up reserved size of the elements.
	reserved_cap = elmts_size / elmt_size;

	//
	// see how many elements we can actually grow by rounding up grow size to the page size.
	uintptr_t commit_grow_size = das_round_up_nearest_multiple_u((uintptr_t)commit_grow_count * elmt_size, page_size);
	commit_grow_count = commit_grow_size / elmt_size;

	pool->page_size = page_size;
	pool->reserved_cap = reserved_cap;
	pool->commit_grow_count = commit_grow_count;

	return DasError_success;
}

DasError _DasPool_deinit(_DasPool* pool, uintptr_t elmt_size) {
	uintptr_t reserve_align;
	uintptr_t page_size;
	DasError error = das_virt_mem_page_size(&page_size, &reserve_align);
	if (error) return error;

	//
	// decommit and release the reserved address space
	uintptr_t elmts_size = das_round_up_nearest_multiple_u((uintptr_t)pool->reserved_cap * elmt_size, reserve_align);
	uintptr_t records_size = das_round_up_nearest_multiple_u((uintptr_t)pool->reserved_cap * sizeof(_DasPoolRecord), reserve_align);
	uintptr_t reserved_size = elmts_size + records_size;
	error = das_virt_mem_release(pool->address_space, reserved_size);
	if (error) return error;

	*pool = (_DasPool){0};
	return DasError_success;
}

DasError _DasPool_reset(_DasPool* pool, uintptr_t elmt_size) {
	if (pool->commited_cap == 0)
		return DasError_success;

	uintptr_t elmts_size = das_round_up_nearest_multiple_u((uintptr_t)pool->commited_cap * elmt_size, pool->page_size);
	uintptr_t records_size = das_round_up_nearest_multiple_u((uintptr_t)pool->commited_cap * sizeof(_DasPoolRecord), pool->page_size);
	void* elmts = pool->address_space;
	_DasPoolRecord* records = _DasPool_records(pool, elmt_size);

	//
	// decommit all of the commited pages of memory for the elements
	DasError error = das_virt_mem_decommit(elmts, elmts_size);
	if (error) return error;

	//
	// decommit all of the commited pages of memory for the records
	error = das_virt_mem_decommit(records, records_size);
	if (error) return error;

	pool->count = 0;
	pool->cap = 0;
	pool->commited_cap = 0;
	pool->free_list_head_id = 0;
	pool->alloced_list_head_id = 0;
	pool->alloced_list_tail_id = 0;
	return DasError_success;
}

DasBool _DasPool_commit_next_chunk(_DasPool* pool, uintptr_t elmt_size) {
	das_assert(pool->address_space, "pool has not been initialized. use DasPool_init before allocating");
	if (pool->commited_cap == pool->reserved_cap)
		return das_false;

	uintptr_t grow_count = das_min_u(pool->commit_grow_count, pool->reserved_cap - pool->commited_cap);

	uintptr_t elmts_size = das_round_up_nearest_multiple_u((uintptr_t)pool->commited_cap * elmt_size, pool->page_size);
	uintptr_t records_size = das_round_up_nearest_multiple_u((uintptr_t)pool->commited_cap * sizeof(_DasPoolRecord), pool->page_size);

	//
	// commit the next chunk of memory at the end of the currently commited elments
	void* elmts_to_commit = das_ptr_add(pool->address_space, elmts_size);
	uintptr_t elmts_grow_size = das_round_up_nearest_multiple_u((uintptr_t)grow_count * elmt_size, pool->page_size);
	DasError error = das_virt_mem_commit(elmts_to_commit, elmts_grow_size, DasVirtMemProtection_read_write);
	das_assert(error == 0, "unexpected error our parameters should be correct: 0x%x", error);

	//
	// commit the next chunk of memory at the end of the currently commited records
	_DasPoolRecord* records_to_commit = das_ptr_add(_DasPool_records(pool, elmt_size), records_size);
	uintptr_t records_grow_size = das_round_up_nearest_multiple_u((uintptr_t)grow_count * sizeof(_DasPoolRecord), pool->page_size);
	error = das_virt_mem_commit(records_to_commit, records_grow_size, DasVirtMemProtection_read_write);
	das_assert(error == 0, "unexpected error our parameters should be correct: 0x%x", error);

	//
	// calculate commited_cap by using the new elements size in bytes and dividing to get an accurate number.
	// adding the grow_count will lose precision if the elmt_size is not directly divisble by the page_size.
	uintptr_t new_elmts_size = elmts_size + elmts_grow_size;
	pool->commited_cap = new_elmts_size / elmt_size;

	return das_true;
}

DasError _DasPool_reset_and_populate(_DasPool* pool, void* elmts, uint32_t count, uintptr_t elmt_size) {
	DasError error = _DasPool_reset(pool, elmt_size);
	if (error) return error;

	while (pool->commited_cap < count) {
		if (!_DasPool_commit_next_chunk(pool, elmt_size)) {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
			return EOVERFLOW;
#elif _WIN32
			return ERROR_BUFFER_OVERFLOW;
#endif
		}
	}

	//
	// set the records for the elements passed in to the function to allocated and point to the next element.
	_DasPoolRecord* records = _DasPool_records(pool, elmt_size);
	for (uint32_t i = 0; i < count; i += 1) {
		records[i].prev_id = i; // the current index is the identifier of the previous record
		records[i].next_id = DasPoolElmtId_is_allocated_bit_MASK | (i + 2); // + 2 as identifiers are +1 an index
	}

	//
	// copy the elements and set the values in the pool structure
	memcpy(pool->address_space, elmts, (uintptr_t)count * elmt_size);
	pool->count = count;
	pool->cap = count;

	return DasError_success;
}

void* _DasPool_alloc(_DasPool* pool, DasPoolElmtId* id_out, uintptr_t elmt_size, uint32_t index_bits) {
	//
	// if the pool is full, try to increment the capacity by one if we have enough commit memory.
	// if not commit a new chunk.
	// if the pool is not full allocate from the head of the free list.
	uint32_t idx_id;
	if (pool->count == pool->cap) {
		if (pool->cap == pool->commited_cap) {
			if (!_DasPool_commit_next_chunk(pool, elmt_size))
				return NULL;
		}

		pool->cap += 1;
		idx_id = pool->cap;
	} else {
		idx_id = pool->free_list_head_id;
	}

	//
	// allocate an element by removing it from the free list
	// and adding it to the allocated list.
	//

	DasPoolElmtId index_mask = (1 << index_bits) - 1;
	uint32_t idx = idx_id - 1;
	_DasPoolRecord* records = _DasPool_records(pool, elmt_size);
	_DasPoolRecord* record_ptr = &records[idx];
	DasPoolElmtId record = record_ptr->next_id;

	das_debug_assert(record_ptr->prev_id == 0, "the surposedly free list head links to a previous element... so it is not the list head");
	das_debug_assert(!(record & DasPoolElmtId_is_allocated_bit_MASK), "allocated element is in the free list of the pool");

	// set the is allocated bit
	record |= DasPoolElmtId_is_allocated_bit_MASK;

	// get the next free id from the record
	uint32_t next_free_idx_id = record & index_mask;

	// clear the index id
	record &= ~index_mask;

	//
	// create pool identifier by strapping the index id to it.
	*id_out = record | idx_id;

	record_ptr->next_id = record;

	//
	// make the next free record the new list head
	if (next_free_idx_id && next_free_idx_id - 1 < pool->cap) {
		records[next_free_idx_id - 1].prev_id = 0;
	}

	//
	// make the old allocated list tail point to the newly allocated element.
	// and we will then point back to it.
	if (pool->alloced_list_tail_id) {
		records[pool->alloced_list_tail_id - 1].next_id |= idx_id;
		record_ptr->prev_id = pool->alloced_list_tail_id;
	}

	if (pool->alloced_list_head_id == 0) {
		pool->alloced_list_head_id = idx_id;
	}

	void* allocated_elmt = das_ptr_add(pool->address_space, (uintptr_t)idx * elmt_size);
	if (idx_id == pool->free_list_head_id) {
		// data comes from the free list so lets zero it.
		memset(allocated_elmt, 0, elmt_size);
	}

	//
	// update the list head/tail ids
	//
	pool->free_list_head_id = next_free_idx_id;
	pool->alloced_list_tail_id = idx_id;
	pool->count += 1;

	return allocated_elmt;
}

void _DasPool_dealloc(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits) {
	_DasPool_assert_id(pool, elmt_id, elmt_size, index_bits);

	DasPoolElmtId index_mask = (1 << index_bits) - 1;

	_DasPoolRecord* records = _DasPool_records(pool, elmt_size);
	uint32_t dealloced_idx_id = elmt_id & index_mask;
	_DasPoolRecord* dealloced_record_ptr = &records[dealloced_idx_id - 1];
	DasPoolElmtId dealloced_record = dealloced_record_ptr->next_id;

	uint32_t prev_allocated_elmt_id = dealloced_record_ptr->prev_id;
	uint32_t next_allocated_elmt_id = dealloced_record & index_mask;


	//
	// place the deallocated element in the free list somewhere.
	//
	uint32_t next_free_idx_id = pool->free_list_head_id;
	if (pool->order_free_list_on_dealloc && next_free_idx_id < dealloced_idx_id) {
		//
		// we have order the free list enabled, so store in low to high order.
		// this will keep allocations new allocations closer to eachother.
		// move up the free list until the deallocating elmt_id is less than that index id the record points to.
		DasPoolElmtId* elmt_next_free_idx_id;
		uint32_t nfi = next_free_idx_id;
		uint32_t count = pool->count;
		for (uint32_t i = 0; i < count; i += 1) {
			elmt_next_free_idx_id = &records[nfi - 1].next_id;
			nfi = *elmt_next_free_idx_id & index_bits;
			if (nfi > dealloced_idx_id) break;
		}

		//
		// now make the record point this deallocated record instead
		//
		DasPoolElmtId record = *elmt_next_free_idx_id;

		//
		// before we do, make the record we used to point to, point back to the deallocated element instead.
		if (nfi) {
			records[nfi - 1].prev_id = dealloced_idx_id;
		}
		dealloced_record_ptr->prev_id = nfi;

		record &= ~index_mask; // clear the index id that points to the original next free element
		record |= dealloced_idx_id; // set the index id of the deallocated element
		*elmt_next_free_idx_id = record;

		next_free_idx_id = nfi;
	} else {
		//
		// place at the head of the free list
		pool->free_list_head_id = dealloced_idx_id;
		dealloced_record_ptr->prev_id = 0;
	}

	//
	// "free" the element by updating the records.
	//
	{
		//
		// go to the next element our deallocated element used to point to and make it point back to
		// the previous element the deallocated element used to point to.
		if (next_allocated_elmt_id) {
			records[next_allocated_elmt_id - 1].prev_id = prev_allocated_elmt_id;
		} else {
			pool->alloced_list_tail_id = prev_allocated_elmt_id;
		}

		if (prev_allocated_elmt_id) {
			_DasPoolRecord* pr = &records[prev_allocated_elmt_id - 1];
			DasPoolElmtId r = pr->next_id;
			r &= ~index_mask; // clear the index that points to the element we have just deallocated
			r |= next_allocated_elmt_id; // now make it point to the element our deallocated element used to point to.
			pr->next_id = r;
		} else {
			pool->alloced_list_head_id = next_allocated_elmt_id;
		}

		//
		// extract the counter and then increment it's value.
		// but make sure it wrap if we reach the maximum the counter bits can hold.
		//
		DasPoolElmtId counter_mask = DasPoolElmtId_counter_mask(index_bits);
		uint32_t counter = (elmt_id & counter_mask) >> index_bits;
		uint32_t counter_max = counter_mask >> index_bits;
		if (counter == counter_max) {
			counter = 0;
		} else {
			counter += 1;
		}

		//
		// now update the counter by clearing and setting.
		dealloced_record &= ~counter_mask;
		dealloced_record |= counter << index_bits;

		//
		// now set the index to the next element in the free list.
		dealloced_record &= ~index_mask;
		dealloced_record |= next_free_idx_id;

		//
		// clear the is allocated bit then store the record back in the array.
		dealloced_record &= ~DasPoolElmtId_is_allocated_bit_MASK;
		dealloced_record_ptr->next_id = dealloced_record;
	}

	pool->count -= 1;
}

void* _DasPool_id_to_ptr(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits) {
	_DasPool_assert_id(pool, elmt_id, elmt_size, index_bits);
	DasPoolElmtId index_mask = (1 << index_bits) - 1;
	uint32_t idx = (elmt_id & index_mask) - 1;
	return das_ptr_add(pool->address_space, (uintptr_t)idx * elmt_size);
}

uint32_t _DasPool_id_to_idx(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits) {
	_DasPool_assert_id(pool, elmt_id, elmt_size, index_bits);
	DasPoolElmtId index_mask = (1 << index_bits) - 1;
	return (elmt_id & index_mask) - 1;
}

DasPoolElmtId _DasPool_ptr_to_id(_DasPool* pool, void* ptr, uintptr_t elmt_size, uint32_t index_bits) {
	das_debug_assert(pool->address_space <= ptr && ptr < das_ptr_add(pool->address_space, (uintptr_t)pool->cap * elmt_size), "pointer was not allocated with this pool");
	uint32_t idx = das_ptr_diff(ptr, pool->address_space) / elmt_size;
	_DasPoolRecord* record = &_DasPool_records(pool, elmt_size)[idx];
	das_debug_assert(record->next_id & DasPoolElmtId_is_allocated_bit_MASK, "the pointer is a freed element");

	//
	// lets make an identifier from the record by just setting the index in it.
	return _DasPool_record_to_id(pool, record, idx + 1, index_bits);
}

uint32_t _DasPool_ptr_to_idx(_DasPool* pool, void* ptr, uintptr_t elmt_size, uint32_t index_bits) {
	das_debug_assert(pool->address_space <= ptr && ptr < das_ptr_add(pool->address_space, (uintptr_t)pool->cap * elmt_size), "pointer was not allocated with this pool");
	uint32_t idx = das_ptr_diff(ptr, pool->address_space) / elmt_size;
	_DasPoolRecord* record = &_DasPool_records(pool, elmt_size)[idx];
	das_debug_assert(record->next_id & DasPoolElmtId_is_allocated_bit_MASK, "the pointer is a freed element");
	return idx;
}

void* _DasPool_idx_to_ptr(_DasPool* pool, uint32_t idx, uintptr_t elmt_size) {
	das_assert(idx < pool->cap, "index of '%u' is out of the pool boundary of '%u' elements", idx, pool->cap);

	_DasPoolRecord* record = &_DasPool_records(pool, elmt_size)[idx];
	das_debug_assert(record->next_id & DasPoolElmtId_is_allocated_bit_MASK, "the index is a freed element");

	return das_ptr_add(pool->address_space, (uintptr_t)idx * elmt_size);
}

DasPoolElmtId _DasPool_idx_to_id(_DasPool* pool, uint32_t idx, uintptr_t elmt_size, uint32_t index_bits) {
	das_assert(idx < pool->cap, "index of '%u' is out of the pool boundary of '%u' elements", idx, pool->cap);

	_DasPoolRecord* record = &_DasPool_records(pool, elmt_size)[idx];
	das_debug_assert(record->next_id & DasPoolElmtId_is_allocated_bit_MASK, "the index is a freed element");

	//
	// lets make an identifier from the record by just setting the index in it.
	return _DasPool_record_to_id(pool, record, idx + 1, index_bits);
}

DasPoolElmtId _DasPool_iter_next(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits) {
	//
	// if NULL is passed in, then get the first record of the allocated list.
	if (elmt_id == 0) {
		uint32_t first_allocated_id = pool->alloced_list_head_id;
		if (first_allocated_id == 0)
			return 0;

		return _DasPool_idx_to_id(pool, first_allocated_id - 1, elmt_size, index_bits);
	}

	_DasPool_assert_id(pool, elmt_id, elmt_size, index_bits);
	//
	// extract the index from the element identifier
	DasPoolElmtId index_mask = (1 << index_bits) - 1;
	uint32_t idx_id = elmt_id & index_mask;

	//
	// get the record for the element
	_DasPoolRecord* records = _DasPool_records(pool, elmt_size);
	DasPoolElmtId record = records[idx_id - 1].next_id;

	//
	// extract the next element index from the record
	uint32_t next_idx_id = record & index_mask;
	if (next_idx_id == 0) return 0;

	//
	// now convert the next record to an identifier for that next record/element
	_DasPoolRecord* next_record = &records[next_idx_id - 1];
	return _DasPool_record_to_id(pool, next_record, next_idx_id, index_bits);
}

DasPoolElmtId _DasPool_iter_prev(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits) {
	//
	// if NULL is passed in, then get the last record of the allocated list.
	if (elmt_id == 0) {
		uint32_t last_allocated_id = pool->alloced_list_tail_id;
		if (last_allocated_id == 0)
			return 0;

		return _DasPool_idx_to_id(pool, last_allocated_id - 1, elmt_size, index_bits);
	}

	_DasPool_assert_id(pool, elmt_id, elmt_size, index_bits);
	//
	// extract the index from the element identifier
	DasPoolElmtId index_mask = (1 << index_bits) - 1;
	uint32_t idx_id = elmt_id & index_mask;

	//
	// get the record for the element
	_DasPoolRecord* records = _DasPool_records(pool, elmt_size);
	uint32_t prev_idx_id = records[idx_id - 1].prev_id;
	if (prev_idx_id == 0) return 0;

	//
	// now convert the previous record to an identifier for that previous record/element
	_DasPoolRecord* prev_record = &records[prev_idx_id - 1];
	return _DasPool_record_to_id(pool, prev_record, prev_idx_id, index_bits);
}

DasPoolElmtId _DasPool_decrement_record_counter(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits) {
	_DasPool_assert_id(pool, elmt_id, elmt_size, index_bits);
	DasPoolElmtId index_mask = (1 << index_bits) - 1;
	uint32_t idx_id = elmt_id & index_mask;
	_DasPoolRecord* record = &_DasPool_records(pool, elmt_size)[idx_id - 1];

	DasPoolElmtId counter_mask = DasPoolElmtId_counter_mask(index_bits);
	uint32_t counter = (elmt_id & counter_mask) >> index_bits;
	uint32_t counter_max = counter_mask >> index_bits;
	if (counter == 0) {
		counter = counter_max;
	} else {
		counter -= 1;
	}

	//
	// now update the counter by clearing and setting.
	record->next_id &= ~counter_mask;
	record->next_id |= counter << index_bits;

	return _DasPool_record_to_id(pool, record, idx_id, index_bits);
}

DasBool _DasPool_is_idx_allocated(_DasPool* pool, uint32_t idx, uintptr_t elmt_size) {
	das_assert(idx < pool->cap, "index of '%u' is out of the pool boundary of '%u' elements", idx, pool->cap);
	_DasPoolRecord* record = &_DasPool_records(pool, elmt_size)[idx];
	return (record->next_id & DasPoolElmtId_is_allocated_bit_MASK) == DasPoolElmtId_is_allocated_bit_MASK;
}

DasBool _DasPool_is_id_valid(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits) {
	if (elmt_id == 0) return das_false;
	if ((elmt_id & DasPoolElmtId_is_allocated_bit_MASK) == 0) return das_false;
	DasPoolElmtId index_mask = (1 << index_bits) - 1;
	uint32_t idx_id = elmt_id & index_mask;
	if (idx_id == 0) return das_false;
	_DasPoolRecord* record = &_DasPool_records(pool, elmt_size)[idx_id - 1];
	if ((record->next_id & DasPoolElmtId_is_allocated_bit_MASK) == 0) return das_false;

	DasPoolElmtId counter_mask = DasPoolElmtId_counter_mask(index_bits);
	uint32_t counter = (elmt_id & counter_mask) >> index_bits;
	uint32_t record_counter = (record->next_id & counter_mask) >> index_bits;
	if (counter != record_counter) return das_false;

	return das_true;
}

