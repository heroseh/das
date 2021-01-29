#ifndef DAS_H
#error "expected das.h to be included before das.c"
#endif

#ifdef __linux__
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <asm-generic/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#elif _WIN32
// TODO: i have read that the windows headers can really slow down compile times.
// since the win32 api is stable maybe we should forward declare the functions and constants manually ourselves.
// maybe we can generate this using the new win32metadata thing if we can figure that out.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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
	struct stat s = {};
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

