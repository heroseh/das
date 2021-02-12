#ifndef DAS_H
#define DAS_H

// ======================================================================
//
//
// Configuration
//
//
// ======================================================================

//
// if you use types that have a larger alignment that 16 bytes. eg. Intel SSE primitives __m128, __m256.
// override this macro as so the DasStk and DasDeque will work properly with these types.
//
#ifndef das_max_align_t
#define das_max_align_t long double
#endif

#ifndef DasStk_min_cap
#define DasStk_min_cap 16
#endif

#ifndef DasDeque_min_cap
#define DasDeque_min_cap 16
#endif

#ifndef DasAlctor_default
#define DasAlctor_default DasAlctor_system
#endif

// ======================================================================
//
//
// General
//
//
// ======================================================================

#include <stdint.h>
#include <stdio.h> // fprintf, vsnprintf
#include <stdlib.h> // abort
#include <string.h> // memcpy, memmove
#include <stddef.h>
#include <errno.h>
#include <math.h>

#if _WIN32
// forward declare here to avoid including windows.h in the header file.
typedef void* HANDLE;
#endif

typedef uint8_t DasBool;
#define das_false 0
#define das_true 1

#ifndef das_noreturn

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define das_noreturn _Noreturn
#elif defined(__GNUC__)
#define das_noreturn __attribute__((noreturn))
#elif _WIN32
#define das_noreturn __declspec(noreturn)
#else
#define das_noreturn
#endif

#endif // das_noreturn

#ifdef _WIN32
#define das_function_name __FUNCTION__
#elif defined(__GNUC__)
#define das_function_name __func__
#else
#define das_function_name NULL
#endif

das_noreturn void _das_abort(const char* file, int line, const char* func, char* assert_test, char* message_fmt, ...);
#define das_abort(...) \
	_das_abort(__FILE__, __LINE__, das_function_name, NULL, __VA_ARGS__)

#define das_assert(cond, ...) \
	if (!(cond)) _das_abort(__FILE__, __LINE__, das_function_name, #cond, __VA_ARGS__)

#define das_assert_loc(file, line, func, cond, ...) \
	if (!(cond)) _das_abort(file, line, func, #cond, __VA_ARGS__)

#if DAS_DEBUG_ASSERTIONS
#define das_debug_assert das_assert
#else
#define das_debug_assert(cond, ...) (void)(cond)
#endif

#ifndef alignof
#ifdef __TINYC__
#define alignof __alignof__
#else
#define alignof _Alignof
#endif
#endif

static inline uintptr_t das_min_u(uintptr_t a, uintptr_t b) { return a < b ? a : b; }
static inline intptr_t das_min_s(intptr_t a, intptr_t b) { return a < b ? a : b; }
static inline double das_min_f(double a, double b) { return a < b ? a : b; }

static inline uintptr_t das_max_u(uintptr_t a, uintptr_t b) { return a > b ? a : b; }
static inline intptr_t das_max_s(intptr_t a, intptr_t b) { return a > b ? a : b; }
static inline double das_max_f(double a, double b) { return a > b ? a : b; }

static inline uintptr_t das_clamp_u(uintptr_t v, uintptr_t min, uintptr_t max) { return v > max ? max : (v >= min ? v : min); }
static inline intptr_t das_clamp_s(intptr_t v, intptr_t min, intptr_t max) { return v > max ? max : (v >= min ? v : min); }
static inline double das_clamp_f(double v, double min, double max) { return v > max ? max : (v >= min ? v : min); }

static inline uintptr_t das_round_up_nearest_multiple_u(uintptr_t v, uintptr_t multiple) {
	uintptr_t rem = v % multiple;
	if (rem == 0) return v;
	return v + multiple - rem;
}

static inline intptr_t das_round_up_nearest_multiple_s(intptr_t v, intptr_t multiple) {
	intptr_t rem = v % multiple;
	if (v > 0) {
		return v + multiple - rem;
	} else {
		return v + rem;
	}
}

static inline double das_round_up_nearest_multiple_f(double v, double multiple) {
	double rem = fmod(v, multiple);
	if (v > 0.0) {
		return v + multiple - rem;
	} else {
		return v + rem;
	}
}

static inline uintptr_t das_round_down_nearest_multiple_u(uintptr_t v, uintptr_t multiple) {
	uintptr_t rem = v % multiple;
	return v - rem;
}

static inline intptr_t das_round_down_nearest_multiple_s(intptr_t v, intptr_t multiple) {
	intptr_t rem = v % multiple;
	if (v >= 0) {
		return v - rem;
	} else {
		return v - rem - multiple;
	}
}

static inline double das_round_down_nearest_multiple_f(double v, double multiple) {
	double rem = fmod(v, multiple);
	if (v >= 0.0) {
		return v - rem;
	} else {
		return v - rem - multiple;
	}
}

#define das_most_set_bit_idx(v) _das_most_set_bit_idx(v, sizeof(v))
static inline uintmax_t _das_most_set_bit_idx(uintmax_t v, uint32_t sizeof_type) {
#ifdef __GNUC__
	uint32_t idx = sizeof(v) * 8;
	uint32_t type_diff = (sizeof(uintmax_t) - sizeof_type) * 8;
	idx -= type_diff;
	if (sizeof(v) == sizeof(long long)) {
		idx -= __builtin_clzll(v);
	} else if (sizeof(v) == sizeof(long)) {
		idx -= __builtin_clzl(v);
	} else {
		idx -= __builtin_clz(v);
	}
	idx -= 1;
	return idx;
#else
#error "unhandle least set bit for this platform"
#endif
}

static inline uintmax_t das_least_set_bit_idx(uintmax_t v) {
#ifdef __GNUC__
	if (sizeof(v) == sizeof(long long)) {
		return __builtin_ctzll(v);
	} else if (sizeof(v) == sizeof(long)) {
		return __builtin_ctzl(v);
	} else {
		return __builtin_ctz(v);
	}
#else
#error "unhandle least set bit for this platform"
#endif
}

// ======================================================================
//
//
// Memory Utilities
//
//
// ======================================================================

extern void* das_ptr_round_up_align(void* ptr, uintptr_t align);
extern void* das_ptr_round_down_align(void* ptr, uintptr_t align);
#define das_is_power_of_two(v) (((v) != 0) && (((v) & ((v) - 1)) == 0))
#define das_ptr_add(ptr, by) (void*)((uintptr_t)(ptr) + (uintptr_t)(by))
#define das_ptr_sub(ptr, by) (void*)((uintptr_t)(ptr) - (uintptr_t)(by))
#define das_ptr_diff(to, from) ((char*)(to) - (char*)(from))
#define das_zero_elmt(ptr) memset(ptr, 0, sizeof(*(ptr)))
#define das_zero_elmts(ptr, elmts_count) memset(ptr, 0, sizeof(*(ptr)) * (elmts_count))
#define das_zero_array(array) memset(array, 0, sizeof(array))
#define das_copy_array(dst, src) memcpy(dst, src, sizeof(dst))
#define das_copy_elmts(dst, src, elmts_count) memcpy(dst, src, elmts_count * sizeof(*(dst)))
#define das_copy_overlap_elmts(dst, src, elmts_count) memmove(dst, src, elmts_count * sizeof(*(dst)))
#define das_cmp_array(a, b) (memcmp(a, b, sizeof(a)) == 0)
#define das_cmp_elmt(a, b) (memcmp(a, b, sizeof(*(a))) == 0)
#define das_cmp_elmts(a, b, elmts_count) (memcmp(a, b, elmts_count * sizeof(*(a))) == 0)

// for X86/64 and ARM. maybe be different for other architectures.
#define das_cache_line_size 64

// ======================================================================
//
//
// Custom Allocator API
//
//
// ======================================================================

// an allocation function for allocating, reallocating and deallocating memory.
/*
	if (!ptr && size == 0) {
		// reset
	} else if (!ptr) {
		// allocate
	} else if (ptr && size > 0) {
		// reallocate
	} else {
		// deallocate
	}
*/
// returns NULL on allocation failure
typedef void* (*DasAllocFn)(void* alctor_data, void* ptr, uintptr_t old_size, uintptr_t size, uintptr_t align);

//
// DasAlctor is the custom allocator data structure.
// is used to point to the implementation of a custom allocator.
//
// EXAMPLE of a local alloctor:
//
// typedef struct {
//     void* data;
//     uint32_t pos;
//     uint32_t size;
// } LinearAlctor;
//
// void* LinearAlctor_alloc_fn(void* alctor_data, void* ptr, uintptr_t old_size, uintptr_t size, uintptr_t align)
//
// // we zero the buffer so this allocator will allocate zeroed memory.
// char buffer[1024] = {0};
// LinearAlctor linear_alctor = { .data = buffer, .pos = 0, .size = sizeof(buffer) };
// DasAlctor alctor = { .data = &linear_alctor, .fn = LinearAlctor_alloc_fn };
//
typedef struct {
	// the allocation function, see the notes above DasAllocFn
	DasAllocFn fn;

	// the data is an optional pointer used to point to an allocator's data structure.
	// it is passed into the DasAllocFn as the first argument.
	// this field can be NULL if you are implementing a global allocator.
	void* data;
} DasAlctor;

void* das_system_alloc_fn(void* alloc_data, void* ptr, uintptr_t old_size, uintptr_t size, uintptr_t align);
#define DasAlctor_system ((DasAlctor){ .fn = das_system_alloc_fn, .data = NULL })

// ======================================================================
//
//
// Dynamic Allocation API
//
//
// ======================================================================
// these macros will de/allocate a memory from the supplied allocator.
// memory will be uninitialized unless the allocator you have zeros the memory for you.
//
// if you allocate some memory, and you want deallocate or expand the memory.
// it is up to you provide the correct 'old_size' that you allocated the memory with in the first place.
// unless you know that the allocator you are using does not care about that.
//
// all @param(align), must be a power of two and greater than 0.
//

// allocates memory that can hold a single element of type @param(T).
// @return: an appropriately aligned pointer to that allocated memory location.
#define das_alloc_elmt(T, alctor) (T*)das_alloc(alctor, sizeof(T), alignof(T))

// deallocates memory that can hold a single element of typeof(*@param(ptr)).
#define das_dealloc_elmt(T, alctor, ptr) das_dealloc(alctor, ptr, sizeof(T), alignof(T))

// allocates memory that can hold an array of @param(count) number of @param(T) elements.
// @return: an appropriately aligned pointer to that allocated memory location.
#define das_alloc_array(T, alctor, count) (T*)das_alloc(alctor, (count) * sizeof(T), alignof(T))

// reallocates memory that can hold an array of @param(old_count) to @param(count) number of typeof(*@param(ptr)) elements.
// the memory of @param(ptr)[0] up to @param(ptr)[old_size] is preserved.
// @return a pointer to allocated memory with enough bytes to store a single element of type typeof(*@param(ptr)).
#define das_realloc_array(T, alctor, ptr, old_count, count) \
	das_realloc(alctor, ptr, (old_count) * sizeof(*(ptr)), (count) * sizeof(T), alignof(T))

// deallocates memory that can hold an array of @param(old_count) number of typeof(*@param(ptr)) elements.
#define das_dealloc_array(T, alctor, ptr, old_count) das_dealloc(alctor, ptr, (old_count) * sizeof(T), alignof(T))

// allocates @param(size) bytes of memory that is aligned to @param(align)
#define das_alloc(alctor, size, align) alctor.fn(alctor.data, NULL, 0, size, align);

// reallocates @param(old_size) to @param(size) bytes of memory that is aligned to @param(align)
// the memory of @param(ptr) up to @param(ptr) + @param(old_size') is preserved.
// if @param(ptr) == NULL, then this will call the alloc function for current allocator instead
#define das_realloc(alctor, ptr, old_size, size, align) alctor.fn(alctor.data, ptr, old_size, size, align);

// deallocates @param(old_size) bytes of memory that is aligned to @param(align'
#define das_dealloc(alctor, ptr, old_size, align) alctor.fn(alctor.data, ptr, old_size, 0, align);

#define das_alloc_reset(alctor) alctor.fn(alctor.data, NULL, 0, 0, 0);

// ======================================================================
//
//
// DasStk - linear stack of elements (LIFO)
//
//
// ======================================================================
//
// LIFO is the optimal usage, so push elements and pop them from the end.
// can be used as a growable array.
// elements are allocated using the Dynamic Allocation API that is defined lower down in this file.
//
// DasStk API example usage:
//
// DasStk(int) stack_of_ints;
// DasStk_resize_cap(&stack_of_ints, 64);
//
// int* elmt = DasStk_push(&stack_of_ints);
// *elmt = 55;
//
// DasStk_pop(&stack_of_ints);
//

#define DasStk(T) DasStkHeader_##T*

//
// the internal header used in the dynamic internal functions.
// WARNING: these field must match the DasStkHeader generated with the typedef_DasStk macro below
typedef struct _DasStkHeader _DasStkHeader;
struct _DasStkHeader {
	uintptr_t count;
	uintptr_t cap;
	DasAlctor alctor;
};


//
// you need to make sure that you use this macro in global scope
// to define a structure for the type you want to use.
//
// we have a funky name for the data field,
// so an error will get thrown if you pass the
// wrong structure into one of the macros below.
#define typedef_DasStk(T) \
typedef struct { \
	uintptr_t count; \
	uintptr_t cap; \
	DasAlctor alctor; \
	T DasStk_data[]; \
} DasStkHeader_##T

typedef_DasStk(char);
typedef_DasStk(short);
typedef_DasStk(int);
typedef_DasStk(long);
typedef_DasStk(float);
typedef_DasStk(double);
typedef_DasStk(size_t);
typedef_DasStk(ptrdiff_t);
typedef_DasStk(uint8_t);
typedef_DasStk(uint16_t);
typedef_DasStk(uint32_t);
typedef_DasStk(uint64_t);
typedef_DasStk(int8_t);
typedef_DasStk(int16_t);
typedef_DasStk(int32_t);
typedef_DasStk(int64_t);

#define DasStk_data(stk_ptr) ((*stk_ptr) ? (*(stk_ptr))->DasStk_data : NULL)
#define DasStk_count(stk_ptr) ((*stk_ptr) ? (*(stk_ptr))->count : 0)
#define DasStk_set_count(stk_ptr, new_count) ((*stk_ptr) ? (*(stk_ptr))->count = (new_count) : 0)
#define DasStk_cap(stk_ptr) ((*stk_ptr) ? (*(stk_ptr))->cap : 0)
#define DasStk_elmt_size(stk_ptr) (sizeof(*(*(stk_ptr))->DasStk_data))
#define DasStk_alctor(stk_ptr) ((*stk_ptr) ? (*(stk_ptr))->alctor : DasAlctor_default)

#define DasStk_for_each(stk_ptr, INDEX_NAME) \
	for (uintptr_t INDEX_NAME = 0, _end_ = DasStk_count(stk_ptr); INDEX_NAME < _end_; INDEX_NAME += 1)

//
// there is no DasStk_init, zeroed data is initialization.
//

//
// preallocates a stack with enough capacity to store init_cap number of elements.
#define DasStk_init_with_cap(stk_ptr, init_cap) DasStk_init_with_alctor(stk_ptr, init_cap, DasAlctor_default)
// the memory is allocator with the supplied allocator and is used for all future reallocations.
#define DasStk_init_with_alctor(stk_ptr, init_cap, alctor) \
	_DasStk_init_with_alctor((_DasStkHeader**)stk_ptr, sizeof(**(stk_ptr)), init_cap, alctor, DasStk_elmt_size(stk_ptr))
extern DasBool _DasStk_init_with_alctor(_DasStkHeader** header_out, uintptr_t header_size, uintptr_t init_cap, DasAlctor alctor, uintptr_t elmt_size);

//
// initialize a stack by copying all the elements from another stack.
#define DasStk_init_clone(dst_stk_ptr, src_stk_ptr) DasStk_init_clone_with_alctor(dst_stk_ptr, src_stk_ptr, DasAlctor_default)
// the memory is allocator with the supplied allocator and is used for all future reallocations.
#define DasStk_init_clone_with_alctor(dst_stk_ptr, src_stk_ptr, alctor) \
	_DasStk_init_clone_with_alctor((_DasStkHeader**)dst_stk_ptr, sizeof(**(dst_stk_ptr)), (_DasStkHeader*)*(src_stk_ptr), alctor, DasStk_elmt_size(dst_stk_ptr))
DasBool _DasStk_init_clone_with_alctor(_DasStkHeader** dst_header_in_out, uintptr_t header_size, _DasStkHeader* src_header, DasAlctor alctor, uintptr_t elmt_size);

// deallocates the memory and sets the stack to being empty.
#define DasStk_deinit(stk_ptr) _DasStk_deinit((_DasStkHeader**)stk_ptr, sizeof(**(stk_ptr)), DasStk_elmt_size(stk_ptr))
extern void _DasStk_deinit(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t elmt_size);

// removes all the elements from the deque
#define DasStk_clear(stk_ptr) ((*stk_ptr) ? (*(stk_ptr))->count = 0 : 0)

// returns a pointer to the element at idx.
// this function will abort if idx is out of bounds
#define DasStk_get(stk_ptr, idx) (&DasStk_data(stk_ptr)[_DasStk_assert_idx((_DasStkHeader*)*(stk_ptr), idx, DasStk_elmt_size(stk_ptr))])
extern uintptr_t _DasStk_assert_idx(_DasStkHeader* header, uintptr_t idx, uintptr_t elmt_size);

// returns a pointer to the element at idx starting from the back of the stack.
// this function will abort if idx is out of bounds
#define DasStk_get_back(stk_ptr, idx) DasStk_get(stk_ptr, DasStk_count(stk_ptr) - 1 - (idx))

// returns a pointer to the first element
// this function will abort if the stack is empty
#define DasStk_get_first(stk_ptr) DasStk_get(stk_ptr, 0)

// returns a pointer to the last element
// this function will abort if the stack is empty
#define DasStk_get_last(stk_ptr) DasStk_get(stk_ptr, DasStk_count(stk_ptr) - 1)

// resizes the stack to have new_count number of elements.
// if new_count is greater than DasStk_cap(stk_ptr), then this function will internally call DasStk_resize_cap to make more room.
// if new_count is greater than DasStk_count(stk_ptr), new elements will be uninitialized,
// unless you want the to be zeroed by passing in zero == das_true.
// returns das_false on allocation failure, otherwise das_true
#define DasStk_resize(stk_ptr, new_count, zero) \
	_DasStk_resize((_DasStkHeader**)stk_ptr, sizeof(**(stk_ptr)), new_count, zero, DasStk_elmt_size(stk_ptr))
extern DasBool _DasStk_resize(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t new_count, DasBool zero, uintptr_t elmt_size);

// reallocates the capacity of the stack to have enough room for new_cap number of elements.
// if new_cap is less than DasStk_count(stk_ptr) then new_cap is set to hold atleast DasStk_count(stk_ptr).
// returns das_false on allocation failure, otherwise das_true
#define DasStk_resize_cap(stk_ptr, new_cap) \
	_DasStk_resize_cap((_DasStkHeader**)stk_ptr, sizeof(**(stk_ptr)), new_cap, DasStk_elmt_size(stk_ptr))
extern DasBool _DasStk_resize_cap(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t new_cap, uintptr_t elmt_size);

// inserts element/s onto the index position in the stack that will be initialized with
// the data stored at @param(elmts). if @param(elmts) is NULL, then the elements are uninitialized.
// the elements from the index position in the stack, will be shifted to the right.
// to make room for the inserted element/s.
// returns a pointer to the first new element, but NULL will be returned on allocation failure.
#define DasStk_insert(stk_ptr, idx, elmt) DasStk_insert_many(stk_ptr, idx, elmt, 1)
#define DasStk_insert_many(stk_ptr, idx, elmts, elmts_count) \
	_DasStk_insert_many((_DasStkHeader**)stk_ptr, sizeof(**(stk_ptr)), idx, elmts, elmts_count, DasStk_elmt_size(stk_ptr))
extern void* _DasStk_insert_many(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t idx, void* elmts, uintptr_t elmts_count, uintptr_t elmt_size);

// pushes element/s onto the end of the stack that will be initialized with
// the data stored at @param(elmts). if @param(elmts) is NULL, then the elements are uninitialized.
// returns a pointer to the first new element, but NULL will be returned on allocation failure.
#define DasStk_push(stk_ptr, elmt) DasStk_push_many(stk_ptr, elmt, 1)
#define DasStk_push_many(stk_ptr, elmts, elmts_count) \
	_DasStk_push_many((_DasStkHeader**)stk_ptr, sizeof(**(stk_ptr)), elmts, elmts_count, DasStk_elmt_size(stk_ptr))
extern void* _DasStk_push_many(_DasStkHeader** header_in_out, uintptr_t header_size, void* elmts, uintptr_t elmts_count, uintptr_t elmt_size);

// removes element/s from the end of the stack
// returns the number of elements popped from the stack.
#define DasStk_pop(stk_ptr) DasStk_pop_many(stk_ptr, 1)
#define DasStk_pop_many(stk_ptr, elmts_count) \
	_DasStk_pop_many((_DasStkHeader*)*(stk_ptr), elmts_count, DasStk_elmt_size(stk_ptr))
extern uintptr_t _DasStk_pop_many(_DasStkHeader* header, uintptr_t elmts_count, uintptr_t elmt_size);

// removes element/s from the start_idx up to end_idx (exclusively).
// elements at the end of the stack are moved to replace the removed elements.
#define DasStk_remove_swap(stk_ptr, idx) \
	DasStk_remove_swap_range(stk_ptr, idx, idx + 1)
#define DasStk_remove_swap_range(stk_ptr, start_idx, end_idx) \
	_DasStk_remove_swap_range((_DasStkHeader*)*(stk_ptr), sizeof(**(stk_ptr)), start_idx, end_idx, DasStk_elmt_size(stk_ptr))
extern void _DasStk_remove_swap_range(_DasStkHeader* header, uintptr_t header_size, uintptr_t start_idx, uintptr_t end_idx, uintptr_t elmt_size);

// removes element/s from the start_idx up to end_idx (exclusively).
// elements that come after are shifted to the left to replace the removed elements.
#define DasStk_remove_shift(stk_ptr, idx) \
	DasStk_remove_shift_range(stk_ptr, idx, idx + 1)
#define DasStk_remove_shift_range(stk_ptr, start_idx, end_idx) \
	_DasStk_remove_shift_range((_DasStkHeader*)*(stk_ptr), sizeof(**(stk_ptr)), start_idx, end_idx, DasStk_elmt_size(stk_ptr))
extern void _DasStk_remove_shift_range(_DasStkHeader* header, uintptr_t header_size, uintptr_t start_idx, uintptr_t end_idx, uintptr_t elmt_size);

// pushes the string on to the end of a byte stack.
// returns the pointer to the start where the string is placed in the stack, but NULL will be returned on allocation failure.
extern char* DasStk_push_str(DasStk(char)* stk, char* str);

// pushes the formatted string on to the end of a byte stack.
// returns the pointer to the start where the string is placed in the stack, but NULL will be returned on allocation failure.
extern void* DasStk_push_str_fmtv(DasStk(char)* stk, char* fmt, va_list args);
#ifdef __GNUC__
extern void* DasStk_push_str_fmt(DasStk(char)* stk, char* fmt, ...) __attribute__ ((format (printf, 2, 3)));
#else
extern void* DasStk_push_str_fmt(DasStk(char)* stk, char* fmt, ...);
#endif

// ======================================================================
//
//
// DasDeque - double ended queue (ring buffer)
//
//
// ======================================================================
//
// designed to be used for FIFO or LIFO operations.
// elements are allocated using the Dynamic Allocation API that is defined lower down in this file.
//
// - empty when 'front_idx' == 'back_idx'
// - internally 'cap' is the number of allocated elements, but the deque can only hold 'cap' - 1.
//     this is because the back_idx needs to point to the next empty element slot.
// - 'front_idx' will point to the item at the front of the queue
// - 'back_idx' will point to the item after the item at the back of the queue
//
// DasDeque API example usage:
//
// DasDeque(int) queue_of_ints;
// DasDeque_init_with_cap(&queue_of_ints, 64);
//
// DasDeque_push_back(&queue_of_ints, NULL);
// *DasDeque_get_last(&queue_of_ints) = 22;
//
// int popped_value = *DasDeque_get(&queue_of_ints, 0);
// DasDeque_pop_front(&queue_of_ints);

#define DasDeque(T) DasDequeHeader_##T*

//
// the internal header used in the dynamic internal functions.
// WARNING: these field must match the DasDequeHeader generated with the typedef_DasDeque macro below
typedef struct {
	uintptr_t cap;
	uintptr_t front_idx;
	uintptr_t back_idx;
	DasAlctor alctor;
} _DasDequeHeader;

//
// you need to make sure that you use this macro in global scope
// to define a structure for the type you want to use.
//
// we have a funky name for the data field,
// so an error will get thrown if you pass the
// wrong structure into one of the macros below.
#define typedef_DasDeque(T) \
typedef struct { \
	uintptr_t cap; \
	uintptr_t front_idx; \
	uintptr_t back_idx; \
	DasAlctor alctor; \
	T DasDeque_data[]; \
} DasDequeHeader_##T

typedef_DasDeque(char);
typedef_DasDeque(short);
typedef_DasDeque(int);
typedef_DasDeque(long);
typedef_DasDeque(float);
typedef_DasDeque(double);
typedef_DasDeque(size_t);
typedef_DasDeque(ptrdiff_t);
typedef_DasDeque(uint8_t);
typedef_DasDeque(uint16_t);
typedef_DasDeque(uint32_t);
typedef_DasDeque(uint64_t);
typedef_DasDeque(int8_t);
typedef_DasDeque(int16_t);
typedef_DasDeque(int32_t);
typedef_DasDeque(int64_t);

#define DasDeque_data(deque_ptr) ((*deque_ptr) ? (*(deque_ptr))->DasDeque_data : NULL)

// returns the number of elements that can be stored in the queue before a reallocation is required.
#define DasDeque_cap(deque_ptr) ((*deque_ptr) ? ((*(deque_ptr))->cap == 0 ? 0 : (*(deque_ptr))->cap - 1) : 0)

// returns the number of elements
#define DasDeque_count(deque_ptr) \
	((*deque_ptr) \
		? ((*(deque_ptr))->back_idx >= (*(deque_ptr))->front_idx \
			? (*(deque_ptr))->back_idx - (*(deque_ptr))->front_idx \
			: (*(deque_ptr))->back_idx + ((*(deque_ptr))->cap - (*(deque_ptr))->front_idx) \
		) \
		: 0 \
	)

#define DasDeque_front_idx(deque_ptr) ((*deque_ptr) ? (*(deque_ptr))->front_idx : 0)
#define DasDeque_back_idx(deque_ptr) ((*deque_ptr) ? (*(deque_ptr))->back_idx : 0)
#define DasDeque_elmt_size(deque_ptr) (sizeof(*(*(deque_ptr))->DasDeque_data))
#define DasDeque_alctor(deque_ptr) ((*deque_ptr) ? (*(deque_ptr))->alctor : DasAlctor_default)

//
// there is no DasDeque_init, zeroed data is initialization.
//

// preallocates a deque with enough capacity to store init_cap number of elements.
#define DasDeque_init_with_cap(deque_ptr, init_cap) DasDeque_init_with_alctor(deque_ptr, init_cap, DasAlctor_default)
// the memory is allocator with the supplied allocator and is used for all future reallocations.
#define DasDeque_init_with_alctor(deque_ptr, init_cap, alctor) \
	_DasDeque_init_with_alctor((_DasDequeHeader**)deque_ptr, sizeof(**(deque_ptr)), init_cap, alctor, DasDeque_elmt_size(deque_ptr))
extern DasBool _DasDeque_init_with_alctor(_DasDequeHeader** header_out, uintptr_t header_size, uintptr_t init_cap, DasAlctor alctor, uintptr_t elmt_size);

// deallocates the memory and sets the deque to being empty.
#define DasDeque_deinit(deque_ptr) _DasDeque_deinit((_DasDequeHeader**)deque_ptr, sizeof(**(deque_ptr)), DasDeque_elmt_size(deque_ptr))
extern void _DasDeque_deinit(_DasDequeHeader** header_in_out, uintptr_t header_size, uintptr_t elmt_size);

// removes all the elements from the deque
#define DasDeque_clear(deque_ptr) ((*deque_ptr) ? (*(deque_ptr))->front_idx = (*(deque_ptr))->back_idx : 0)

// returns a pointer to the element at idx.
// this function will abort if idx is out of bounds
#define DasDeque_get(deque_ptr, idx) (&DasDeque_data(deque_ptr)[_DasDeque_assert_idx((_DasDequeHeader*)*(deque_ptr), idx, DasDeque_elmt_size(deque_ptr))])
extern uintptr_t _DasDeque_assert_idx(_DasDequeHeader* header, uintptr_t idx, uintptr_t elmt_size);

// returns a pointer to the element at idx starting from the back of the deque.
// this function will abort if idx is out of bounds
#define DasDeque_get_back(deque_ptr, idx) DasDeque_get(deque_ptr, DasDeque_count(deque_ptr) - 1 - (idx))

// returns a pointer to the first element
// this function will abort if the deque is empty
#define DasDeque_get_first(deque_ptr) DasDeque_get(deque_ptr, 0)
//
// returns a pointer to the last element
// this function will abort if the deque is empty
#define DasDeque_get_last(deque_ptr) DasDeque_get(deque_ptr, DasDeque_count(deque_ptr) - 1)

// reallocates the capacity of the data to have enough room for new_cap number of elements.
// if new_cap is less than DasDeque_count(deque_ptr) then new_cap is set to DasDeque_count(deque_ptr).
// returns das_false on allocation failure, otherwise das_true
#define DasDeque_resize_cap(deque_ptr, new_cap) \
	_DasDeque_resize_cap((_DasDequeHeader**)deque_ptr, sizeof(**(deque_ptr)), new_cap, DasDeque_elmt_size(deque_ptr))
DasBool _DasDeque_resize_cap(_DasDequeHeader** header_in_out, uintptr_t header_size, uintptr_t new_cap, uintptr_t elmt_size);


// read elements out of the deque starting from a specified index.
#define DasDeque_read(deque_ptr, idx, elmts_out, elmts_count) _DasDeque_read((_DasDequeHeader*)*(deque_ptr), sizeof(**(deque_ptr)), idx, elmts_out, elmts_count, DasDeque_elmt_size(deque_ptr))
void _DasDeque_read(_DasDequeHeader* header, uintptr_t header_size, uintptr_t idx, void* elmts_out, uintptr_t elmts_count, uintptr_t elmt_size);

// writes elements to the deque starting from a specified index.
#define DasDeque_write(deque_ptr, idx, elmts, elmts_count) _DasDeque_write((_DasDequeHeader*)*(deque_ptr), sizeof(**(deque_ptr)), idx, elmts, elmts_count, DasDeque_elmt_size(deque_ptr))
void _DasDeque_write(_DasDequeHeader* header, uintptr_t header_size, uintptr_t idx, void* elmts, uintptr_t elmts_count, uintptr_t elmt_size);

// pushes element/s at the FRONT of the deque with the data stored at @param(elmts).
// if @param(elmts) is NULL, then the elements are uninitialized.
// returns an index to the first new element in the deque (which is always 0),
// but UINTPTR_MAX will be returned on allocation failure
#define DasDeque_push_front(deque_ptr, elmt) DasDeque_push_front_many(deque_ptr, elmt, 1)
#define DasDeque_push_front_many(deque_ptr, elmts, elmts_count) _DasDeque_push_front_many((_DasDequeHeader**)deque_ptr, sizeof(**(deque_ptr)), elmts, elmts_count, DasDeque_elmt_size(deque_ptr))
uintptr_t _DasDeque_push_front_many(_DasDequeHeader** header_in_out, uintptr_t header_size, void* elmts, uintptr_t elmts_count, uintptr_t elmt_size);

// pushes element/s at the BACK of the deque with the data stored at @param(elmts).
// if @param(elmts) is NULL, then the elements are uninitialized.
// returns an index to the first new element in the deque,
// but UINTPTR_MAX will be returned on allocation failure
#define DasDeque_push_back(deque_ptr, elmt) DasDeque_push_back_many(deque_ptr, elmt, 1)
#define DasDeque_push_back_many(deque_ptr, elmts, elmts_count) _DasDeque_push_back_many((_DasDequeHeader**)deque_ptr, sizeof(**(deque_ptr)), elmts, elmts_count, DasDeque_elmt_size(deque_ptr))
uintptr_t _DasDeque_push_back_many(_DasDequeHeader** header_in_out, uintptr_t header_size, void* elmts, uintptr_t elmts_count, uintptr_t elmt_size);

// removes element/s from the FRONT of the deque
// returns the number of elements popped from the deque.
#define DasDeque_pop_front(deque_ptr) DasDeque_pop_front_many(deque_ptr, 1)
#define DasDeque_pop_front_many(deque_ptr, elmts_count) _DasDeque_pop_front_many((_DasDequeHeader*)*(deque_ptr), elmts_count, DasDeque_elmt_size(deque_ptr))
uintptr_t _DasDeque_pop_front_many(_DasDequeHeader* header, uintptr_t elmts_count, uintptr_t elmt_size);

// removes element/s from the BACK of the deque
// returns the number of elements popped from the deque.
#define DasDeque_pop_back(deque_ptr) DasDeque_pop_back_many(deque_ptr, 1)
#define DasDeque_pop_back_many(deque_ptr, elmts_count) _DasDeque_pop_back_many((_DasDequeHeader*)*(deque_ptr), elmts_count, DasDeque_elmt_size(deque_ptr))
uintptr_t _DasDeque_pop_back_many(_DasDequeHeader* header, uintptr_t elmts_count, uintptr_t elmt_size);

// ===========================================================================
//
//
// Platform Error Handling
//
//
// ===========================================================================

//
// 0 means success
// TODO: this just directly maps to errno on Unix and DWORD GetLastError on Windows.
// the problem is we cant programatically handle errors in a cross platform way.
// in future create our own enumeration that contains all the errors.
// right now the user can just get the string by calling das_get_error_string.
typedef uint32_t DasError;
#define DasError_success 0

typedef uint8_t DasErrorStrRes;
enum {
	DasErrorStrRes_success,
	DasErrorStrRes_invalid_error_arg,
	DasErrorStrRes_not_enough_space_in_buffer,
};

//
// get the string of @param(error) and writes it into the @param(buf_out) pointer upto @param(buf_out_len)
// @param(buf_out) is null terminated on success.
//
// @return: the result detailing the success or error.
//
DasErrorStrRes das_get_error_string(DasError error, char* buf_out, uint32_t buf_out_len);

// ===========================================================================
//
//
// File Abstraction
//
//
// ===========================================================================
//
// heavily inspired by the Rust standard library File API.
// we needed a file abstraction for the virtual memory
// but it's nice being able to talk directly to the OS
// instead and have an unbuffered API unlike fread, fwrite.
//
//

typedef uint8_t DasFileFlags;
enum {
	// specifies that the open file can be read from.
	DasFileFlags_read = 0x1,
	// specifies that the open file can be written to.
	// starts the cursor at the start of the file if append is not set.
	DasFileFlags_write = 0x2,
	// specifies that the open file can be written to.
	// the cursor at the end of the file.
	DasFileFlags_append = 0x4,
	// truncate an existing file by removing it's contents are starting from 0 bytes.
	// must be opened with write or append.
	DasFileFlags_truncate = 0x8,
	// creates a new file if it does not exist.
	// must be opened with write or append.
	DasFileFlags_create_if_not_exist = 0x10,
	// only creates a new file and errors if the file exists.
	// enabling this will ignore create_if_not_exist and truncate.
	DasFileFlags_create_new = 0x20,
};

typedef struct DasFileHandle DasFileHandle;
struct DasFileHandle {
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	int raw; // native Unix File Descriptor
#elif _WIN32
	HANDLE raw; // native Windows File Handle
#else
#error "unimplemented file API for this platform"
#endif
};

//
// opens a file to be use by the other file functions or das_virt_mem_map_file.
//
// @param(path): a path to the file you wish to open
//
// @param(flags): flags to choose how you want the file to be opened.
//     see DasFileFlags for more information.
//
// @param(file_handle_out): a pointer to the file handle that is set
//     when this function returns successfully.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_file_open(char* path, DasFileFlags flags, DasFileHandle* file_handle_out);

//
// closes a file that was opened with das_file_open.
//
// @param(handle): the file handle created with successful call to das_file_open
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_file_close(DasFileHandle handle);

//
// gets the size of the file in bytes
//
// @param(handle): the file handle created with successful call to das_file_open
//
// @param(size_out): a pointer to a value that is set to size of the file when this function returns successfully.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_file_size(DasFileHandle handle, uint64_t* size_out);

//
// attempts to read bytes from a file at it's current cursor in one go.
// the cursor is incremented by the number of bytes read.
//
// @param(handle): the file handle created with successful call to das_file_open
//
// @param(data_out): a pointer to where the data is read to.
//
// @param(length): the number of bytes you wish to try and read into @param(data_out) in one go.
//
// @param(bytes_read_out): a pointer to a value that is set to the number of bytes read when this function returns successfully.
//     on success: if *bytes_read_out == 0 then the end of the file has been reached and no bytes where read
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_file_read(DasFileHandle handle, void* data_out, uintptr_t length, uintptr_t* bytes_read_out);

//
// attempts to read bytes from a file at it's current cursor until the supplied output buffer is filled up.
// the cursor is incremented by the number of bytes read.
//
// @param(handle): the file handle created with successful call to das_file_open
//
// @param(data_out): a pointer to where the data is read to.
//
// @param(length): the number of bytes you wish to try and read into @param(data_out)
//
// @param(bytes_read_out): a pointer to a value that is set to the number of bytes read when this function returns successfully.
//     on success: if *bytes_read_out == 0 then the end of the file has been reached and no bytes where read
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_file_read_exact(DasFileHandle handle, void* data_out, uintptr_t length, uintptr_t* bytes_read_out);

//
// attempts to write bytes to a file at it's current cursor in one go.
// the cursor is incremented by the number of bytes written.
//
// @param(handle): the file handle created with successful call to das_file_open
//
// @param(data): a pointer to where the data is written to.
//
// @param(length): the number of bytes you wish to try and write into @param(data) in one go.
//
// @param(bytes_written_out): a pointer to a value that is set to the number of bytes written when this function returns successfully.
//     on success: if *bytes_written_out == 0 then the end of the file has been reached and no bytes where written
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_file_write(DasFileHandle handle, void* data, uintptr_t length, uintptr_t* bytes_written_out);

//
// attempts to write bytes from a file at it's current cursor until the supplied output buffer is filled up.
// the cursor is incremented by the number of bytes written.
//
// @param(handle): the file handle created with successful call to das_file_open
//
// @param(data): a pointer to where the data is written to.
//
// @param(length): the number of bytes you wish to try and write into @param(data)
//
// @param(bytes_written_out): a pointer to a value that is set to the number of bytes written when this function returns successfully.
//     on success: if *bytes_written_out == 0 then the end of the file has been reached and no bytes where written
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_file_write_exact(DasFileHandle handle, void* data, uintptr_t length, uintptr_t* bytes_written_out);

typedef uint8_t DasFileSeekFrom;
enum {
	// the file cursor offset will be set to @param(offset) in bytes
	DasFileSeekFrom_start,
	// the file cursor offset will be set to the file's current cursor offset + @param(offset) in bytes
	DasFileSeekFrom_current,
	// the file cursor offset will be set to the file's size + @param(offset) in bytes
	DasFileSeekFrom_end
};

//
// attempts to move the file's cursor to a different location in the file to read and write from.
//
// @param(handle): the file handle created with successful call to das_file_open
//
// @param(offset): offset from where the @param(from) is targeting. see DasFileSeekFrom for more information
//
// @param(from): a target to seek from in the file. see DasFileSeekFrom for more information
//
// @param(cursor_offset_out): a pointer to a value that is set to the file's cursor offset when this function returns successfully.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_file_seek(DasFileHandle handle, int64_t offset, DasFileSeekFrom from, uint64_t* cursor_offset_out);

//
// flush any queued data in the OS for the file to the storage device.
//
// @param(handle): the file handle created with successful call to das_file_open
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_file_flush(DasFileHandle handle);

// ===========================================================================
//
//
// Virtual Memory Abstraction
//
//
// ===========================================================================

typedef uint8_t DasVirtMemProtection;
enum {
	DasVirtMemProtection_no_access,
	DasVirtMemProtection_read,
	DasVirtMemProtection_read_write,
	DasVirtMemProtection_exec_read,
	DasVirtMemProtection_exec_read_write,
};

//
// @param(page_size_out):
//     the page size of the OS.
//     used to align the parameters of the virtual memory functions to a page.
//
// @param(reserve_align_out):
//     a pointer to recieve the alignment of the virtual memory reserve address.
//     this is used as the alignment the requested_addr parameter of das_virt_mem_reserve.
//     this is guaranteed to the be the same as page size or a multiple of it.
//     On Unix: this is just the page_size
//     On Windows: this is what known as the page granularity.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_virt_mem_page_size(uintptr_t* page_size_out, uintptr_t* reserve_align_out);

//
// reserve a range of the virtual address space but does not commit any physical pages of memory.
// none of this memory cannot be used until das_virt_mem_commit is called.
//
// WARNING: on Windows, you cannot release sub sections of the address space.
//          you can only release the full reserved address space that is issued by this function call.
//          there are also restriction on protection, see das_virt_mem_protection_set.
//
// @param(requested_addr): the requested start address you wish to reserve.
//     be a aligned to the reserve_align that is retrieved from das_virt_mem_page_size function.
//     this is not guaranteed and is only used as hint.
//     NULL will not be used as a hint, instead the OS will choose an address for you.
//
// @param(size): the size in bytes you wish to reserve from the @param(requested_addr)
//     must be a multiple of the reserve_align that is retrieved from das_virt_mem_page_size function.
//
// @param(addr_out) a pointer to a value that is set to the start of the reserved block of memory
//     when this function returns successfully.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_virt_mem_reserve(void* requested_addr, uintptr_t size, void** addr_out);

//
// requests the OS to commit physical pages of memory to the the address space.
// this address space must be a full or subsection of the reserved address space with das_virt_mem_reserve.
// the memory in the commited address space will be zeroed after calling this function.
//
// @param(addr): the start address of the memory you wish to commit.
//     must be a aligned to the page size das_virt_mem_page_size returns.
//     this is not guaranteed and is only used as hint.
//     NULL will not be used as a hint.
//
// @param(size): the size in bytes you wish to reserve from the @param(addr)
//     must be a aligned to the page size das_virt_mem_page_size returns.
//
// @param(protection): what the memory is allowed to be used for
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_virt_mem_commit(void* addr, uintptr_t size, DasVirtMemProtection protection);

//
// change the protection of a range of memory.
// this memory must have been reserved with das_virt_mem_reserve.
//
// WARNING: on Windows, you can change protection of any number pages
//          but that they all must come from the same call they where reserved with.
//
// @param(addr): the start of the pages you wish to change the protection for.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @param(size): the size in bytes of the memory you wish to change protection for.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @param(protection): what the memory is allowed to be used for
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_virt_mem_protection_set(void* addr, uintptr_t size, DasVirtMemProtection protection);

//
// gives the memory back to the OS but will keep the address space reserved
//
// @param(addr): the start of the pages you wish to decommit.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @param(size): the size in bytes of the memory you wish to release.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_virt_mem_decommit(void* addr, uintptr_t size);

//
// gives the reserved pages back to the OS. the address range must have be reserved with das_virt_mem_reserve.
// all commit pages in the released address space are automatically decommit when you release.
//
// on non Windows systems only:
//     you can target sub pages of the original allocation but just make sure the parameters are aligned.
//
// WARNING: on Windows, you cannot release sub sections of the address space.
//          you can only release the full reserved address space that is issued by das_virt_mem_reserve.
//          so @param(size) is ignored on Windows.
//
// @param(addr): the start of the pages you wish to release.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @param(size): the size in bytes of the memory you wish to release.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_virt_mem_release(void* addr, uintptr_t size);

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
typedef void* DasMapFileHandle; // unused
#elif _WIN32
typedef HANDLE DasMapFileHandle;
#endif

//
// maps a file to the virtual address space of the process.
//
// @param(requested_addr): see the same parameter in das_virt_mem_reserve for more info.
//
// @param(file_handle): the handle to the file opened with das_file_open
//
// @param(protection): what the memory is allowed to be used for.
//     this must match with the file_handle's access rights
//     see das_file_open and DasFileFlags.
//
// @param(offset): offset into the file in bytes this does not have to be aligned in anyway.
//     internally it will round this down to the nearest reserve_align and map that.
//
// @param(size): size of the file in bytes starting from the offset that you wish to map into the address space.
//     this does not have to be aligned in anyway but it will round this up to the page size internally and map that.
//
// @param(addr_out) a pointer to a value that is set to the address pointing to the offset into the file that was requested
//     when this function returns successfully.
//
// @param(map_file_handle_out): a pointer that is set apon success to the handle of the map file.
//     this is only used in Windows and must be passed into das_virt_mem_unmap_file to unmap correctly.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_virt_mem_map_file(void* requested_addr, DasFileHandle file_handle, DasVirtMemProtection protection, uint64_t offset, uintptr_t size, void** addr_out, DasMapFileHandle* map_file_handle_out);


//
// unmaps a file that was mapped into the virtual address space by das_virt_mem_map_file.
//
// @param(addr): the address that was returned when mapping the file.
//
// @param(size): the size that was used when mapping the file.
//
// @param(map_file_handle): the map file handle that was returned when mapping the file.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError das_virt_mem_unmap_file(void* addr, uintptr_t size, DasMapFileHandle map_file_handle);

// ===========================================================================
//
//
// Linear Allocator
//
//
// ===========================================================================
//
// this linear allocator directly reserve address space and commits chunks
// of memory as and when they are needed. so you can reserve 2GBs and no
// memory is taken up on the system.
// all allocated memory is zeroed by the OS when the commit new chunks.
//

typedef struct {
	void* address_space;
	uintptr_t pos;
	uintptr_t commited_size;
	uintptr_t commit_grow_size;
	uintptr_t reserved_size;
} DasLinearAlctor;

//
// initializes the linear allocator and reserves the address space
// needed to store @param(reserved_size) in bytes.
//
// @param(alctor): a pointer the linear allocator structure to initialize.
//
// @param(reserved_size): the maximum size the linear allocator can expand to in bytes.
//
// @param(commit_grow_size): the amount of memory that is commit when the linear allocator needs to grow
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError DasLinearAlctor_init(DasLinearAlctor* alctor, uintptr_t reserved_size, uintptr_t commit_grow_size);

//
// deinitializes the linear allocator and release the address space back to the OS
//
// @param(alctor): a pointer the linear allocator structure.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
DasError DasLinearAlctor_deinit(DasLinearAlctor* alctor);

//
// this is the allocator alloc function used in the DasAlctor interface.
//
// reset: set the next allocation position back to 0 and decommit all existing memory back to the OS
//
// alloc: try to bump up the next allocation position if there is enough commited memory and return the pointer to the zeroed memory.
//     if go past the commited memory then try to commit more if it has not reache the maximum reserved size already.
//     if we have exhausted the reserve size, then the allocation fails.
//
// realloc: if this was the previous allocation then try to extend the allocation in place.
//     if not then allocate new memory and copy the old allocation there.
//
// dealloc: do nothing
//
void* DasLinearAlctor_alloc_fn(void* alctor_data, void* ptr, uintptr_t old_size, uintptr_t size, uintptr_t align);

//
// creates an instance of the DasAlctor interface using a DasLinearAlctor.
#define DasLinearAlctor_as_das(linear_alctor_ptr) \
	(DasAlctor){ .fn = DasLinearAlctor_alloc_fn, .data = linear_alctor_ptr };

// ===========================================================================
//
//
// Pool
//
//
// ===========================================================================
// an optimized way of allocating lots of the same element type quickly.
// the allocated elements are linked so can be iterated.
// elements can be accessed by a validated identifier, index or pointer.
//
// it doesn't make sense making this allocator use the DasAlctor interface.
// it is very limited only being able to allocate elements of the same type.
// and the validated identifier should be the main way the elements are accessed.
//

//
// this type contains an is allocated bit, a counter and an index.
// the number of bits the index uses is passed in to most pool functions.
// the allocated bit is at the MSB.
// this means the counter bits can be workout by removing the index bits and allocated bit from the equation.
//
// eg. index_bits = 20
//
// index mask:       0x000fffff
// counter mask:     0x7ff00000
// is allocated bit: 0x80000000
//
typedef uint32_t DasPoolElmtId;
#define DasPoolElmtId_is_allocated_bit_MASK 0x80000000
#define DasPoolElmtId_counter_mask(index_bits) (~(((1 << index_bits) - 1) | DasPoolElmtId_is_allocated_bit_MASK))

#define DasPoolElmtId_idx(Type, id) ((id).raw & ((1 << Type##_index_bits) - 1))
#define DasPoolElmtId_counter(Type, id) (((id).raw & DasPoolElmtId_counter_mask(Type##_index_bits)) >> Type##_index_bits)

//
// use this to typedef a element identifier for your pool type.
// there is an index bits variable and null identifier created by concatenating
// type name. for typedef_DasPoolElmtId(TestId, 20)
// TestId_index_bits and TestId_null will be defined.
//
// @param(Type): the name you wish to call the type
//
// @param(index_bits): the number of index bits you wish to use for the identifier.
//     the number of counter bits is worked out from the rest. see DasPoolElmtId documentation.
//
#define typedef_DasPoolElmtId(Type, index_bits) \
	static uint32_t Type##_index_bits = index_bits; \
	typedef union Type Type; \
	union Type { DasPoolElmtId Type##_raw; DasPoolElmtId raw; }; \
	static Type Type##_null = { .raw = 0 }

//
// the internal record to link to the previous and next item.
// can be in a free list or an allocated list.
//
typedef struct _DasPoolRecord _DasPoolRecord;
struct _DasPoolRecord {
	uint32_t prev_id;
	// this next_id contains the counter and allocated bit for the element this record structure refers to.
	DasPoolElmtId next_id;
};

//
// the internal pool
// WARNING: this must match the typedef'd pool below
//
typedef struct _DasPool _DasPool;
struct _DasPool {
	/*
	// the data layout of the 'address_space' field

	T elements[reserved_cap]
	_DasPoolRecord records[reserved_cap]

	// an _DasPoolRecord.next_id stores the is_allocated_bit, a counter and an index in the same integer.
	// the index points to the next allocated index when it is allocated and
	// will point to the the next free index when it is not allocated.
	*/
	void* address_space;
	uint32_t count;
	uint32_t cap;
	uint32_t commited_cap;
	uint32_t commit_grow_count;
	uint32_t reserved_cap;
	uint32_t page_size;
	uint32_t free_list_head_id;
	uint32_t alloced_list_head_id;
	uint32_t alloced_list_tail_id: 31;
	uint32_t order_free_list_on_dealloc: 1;
};

//
// macro to use the typedef'd pool
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(T): the type you wish to use in the element pool
//
#define DasPool(IdType, T) DasPool_##IdType##_##T

//
// use this to typedef pool for a identifier and element type.
// refer to the type using the DasPool macro.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(T): the type you wish to use in the element pool
//
#define typedef_DasPool(IdType, T) \
typedef struct { \
	T* IdType##_address_space; \
	uint32_t count; \
	uint32_t cap; \
	uint32_t commited_cap; \
	uint32_t commit_grow_count; \
	uint32_t reserved_cap; \
	uint32_t page_size; \
	uint32_t free_list_head_id; \
	uint32_t alloced_list_head_id; \
	uint32_t alloced_list_tail_id: 31; \
	uint32_t order_free_list_on_dealloc: 1; \
} DasPool_##IdType##_##T

//
// initializes the pool and reserves the address space needed to store @param(reserved_cap) number of elements.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(reserved_cap): the suggested maximum number of elements the pool can expand to in bytes.
//     this will be round up so that the virtual address space is reserved with a size aligned to reserve align.
//     you can get the round up value in DasPool.reserved_cap
//
// @param(commit_grow_count): the suggested amount of elements to grow the commited memory of the pool when it needs to grow.
//     this will be round up so that the commit grow size is aligned to the page size.
//     you can get the round up value in DasPool.commit_grow_count.
//     this new number will not be the exact grow count if the element size is not directly divisble by page size.
//     this is due to the remainder not being stored in the integer.
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
#define DasPool_init(IdType, pool, reserved_cap, commit_grow_count) \
	_DasPool_init((_DasPool*)pool, reserved_cap, commit_grow_count, sizeof(*(pool)->IdType##_address_space))
DasError _DasPool_init(_DasPool* pool, uint32_t reserved_cap, uint32_t commit_grow_count, uintptr_t elmt_size);

//
// deinitializes the pool by releasing the address space back to the OS and zeroing the pool structure
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
#define DasPool_deinit(IdType, pool) \
	_DasPool_deinit((_DasPool*)pool, sizeof(*(pool)->IdType##_address_space))
DasError _DasPool_deinit(_DasPool* pool, uintptr_t elmt_size);

//
// reset the pool as if it has just been initialized. all commited memory is decommited
// but the address_space is still reserved.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
#define DasPool_reset(IdType, pool) \
	_DasPool_reset((_DasPool*)pool, sizeof(*(pool)->IdType##_address_space))
DasError _DasPool_reset(_DasPool* pool, uintptr_t elmt_size);

//
// does a DasPool_reset and then initializes the pool with an array of elements.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(elmts): a pointer to the array of elements
//
// @param(count): the number of elements in the array
//
// @return: 0 on success, otherwise a error code to indicate the error.
//
#define DasPool_reset_and_populate(IdType, pool, elmts, count) \
	_DasPool_reset_and_populate((_DasPool*)pool, elmts, count, sizeof(*(pool)->IdType##_address_space))
DasError _DasPool_reset_and_populate(_DasPool* pool, void* elmts, uint32_t count, uintptr_t elmt_size);

//
// allocates a zeroed element from the pool. the new element will be pushed on to
// the tail of the allocated linked list.
// the pool will try to allocate by using the head of the free list,
// otherwise it will extend the capacity.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(id_out): a pointer that is set apon success to the identifier of this allocation
//
// @return: a pointer to the new zeroed element but a value of NULL if allocation failed.
//
#define DasPool_alloc(IdType, pool, id_out) \
	_DasPool_alloc((_DasPool*)pool, &(id_out)->IdType##_raw, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits)
void* _DasPool_alloc(_DasPool* pool, DasPoolElmtId* id_out, uintptr_t elmt_size, uint32_t index_bits);

//
// deallocates an element from the pool. the element will be removed from the allocated linked list
// and will be pushed on to the head of the free list.
// internally the element's counter will be incremented so the next allocation that
// uses the same memory location will have a different counter.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(elmt_id): the element identifier you wish to deallocate
//
#define DasPool_dealloc(IdType, pool, elmt_id) \
	_DasPool_dealloc((_DasPool*)pool, elmt_id.IdType##_raw, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits)
void _DasPool_dealloc(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits);

//
// gets the pointer for the element with the provided element identifier.
// aborts if the identifier is invalid (already freed or out of bounds).
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(elmt_id): the element identifier you wish to get a pointer to
//
// @return: a pointer to the element
//
#define DasPool_id_to_ptr(IdType, pool, elmt_id) \
	((typeof((pool)->IdType##_address_space))_DasPool_id_to_ptr((_DasPool*)pool, elmt_id.IdType##_raw, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits))
void* _DasPool_id_to_ptr(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits);

//
// gets the index for the element with the provided element identifier.
// aborts if the identifier is invalid (already freed or out of bounds).
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(elmt_id): the element identifier you wish to get a pointer to
//
// @return: the index of the element
//
#define DasPool_id_to_idx(IdType, pool, elmt_id) \
	_DasPool_id_to_idx((_DasPool*)pool, elmt_id.IdType##_raw, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits)
uint32_t _DasPool_id_to_idx(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits);

//
// gets the identifier for the element with the provided element pointer.
// aborts if the pointer is out of bounds or the element at the pointer is not allocated.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(ptr): the pointer to the element you wish to get an identifier for
//
// @return: the identifier to the element
//
#define DasPool_ptr_to_id(IdType, pool, ptr) \
	((IdType) { .IdType##_raw = _DasPool_ptr_to_id((_DasPool*)pool, ptr, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits) })
DasPoolElmtId _DasPool_ptr_to_id(_DasPool* pool, void* ptr, uintptr_t elmt_size, uint32_t index_bits);

//
// gets the index for the element with the provided element pointer.
// aborts if the pointer is out of bounds or the element at the pointer is not allocated.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(ptr): the pointer to the element you wish to get an index for
//
// @return: the index of the element
//
#define DasPool_ptr_to_idx(IdType, pool, ptr) \
	_DasPool_ptr_to_idx((_DasPool*)pool, ptr, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits)
uint32_t _DasPool_ptr_to_idx(_DasPool* pool, void* ptr, uintptr_t elmt_size, uint32_t index_bits);

//
// gets the pointer for the element with the provided element index.
// aborts if the index is out of bounds or the element at the index is not allocated.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(idx): the index of the element you wish to get a pointer to
//
// @return: a pointer to the element
//
#define DasPool_idx_to_ptr(IdType, pool, idx) \
	((typeof((pool)->IdType##_address_space))_DasPool_idx_to_ptr((_DasPool*)pool, idx, sizeof(*(pool)->IdType##_address_space)))
void* _DasPool_idx_to_ptr(_DasPool* pool, uint32_t idx, uintptr_t elmt_size);

//
// gets the identifier for the element with the provided element index.
// aborts if the index is out of bounds or the element at the index is not allocated.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(idx): the index of the element you wish to get a identifier for
//
// @return: the identifier of the element
//
#define DasPool_idx_to_id(IdType, pool, idx) \
	((IdType) { .IdType##_raw = _DasPool_idx_to_id((_DasPool*)pool, idx, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits) })
DasPoolElmtId _DasPool_idx_to_id(_DasPool* pool, uint32_t idx, uintptr_t elmt_size, uint32_t index_bits);

//
// goes to the next allocated element in the linked list from the provided element identifier.
// if @param(elmt_id) is the null identifier then the start of the allocated linked list is returned.
// see typedef_DasPoolElmtId for how to get the null identifier.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(elmt_id): the element identifier to move from
//
// @return: the identifier of the next element, the null identifier will be returned if the element
// at the end of the allocated linked list was the @param(elmt_id)
//
#define DasPool_iter_next(IdType, pool, elmt_id) \
	((IdType) { .IdType##_raw = _DasPool_iter_next((_DasPool*)pool, elmt_id.IdType##_raw, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits) })
DasPoolElmtId _DasPool_iter_next(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits);

//
// goes to the previous allocated element in the linked list from the provided element identifier.
// if @param(elmt_id) is the null identifier then the end of the allocated linked list is returned.
// see typedef_DasPoolElmtId for how to get the null identifier.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(elmt_id): the element identifier to move from
//
// @return: the identifier of the previous element, the null identifier will be returned if the element
// at the start of the allocated linked list was the @param(elmt_id)
//
#define DasPool_iter_prev(IdType, pool, elmt_id) \
	((IdType) { .IdType##_raw = _DasPool_iter_prev((_DasPool*)pool, elmt_id.IdType##_raw, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits) })
DasPoolElmtId _DasPool_iter_prev(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits);

//
// decrements the internal counter of the element by 1. this will invalidate
// the current identifier and restore the previous identifier at the same index
// as this one.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(elmt_id): the element identifier you wish to decrement
//
// @return: the new identifier as a result of decrementing the counter
//
#define DasPool_decrement_record_counter(IdType, pool, elmt_id) \
	((IdType) { .IdType##_raw = _DasPool_decrement_record_counter((_DasPool*)pool, elmt_id.IdType##_raw, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits) })
DasPoolElmtId _DasPool_decrement_record_counter(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits);

//
// see if an element at the provided index is allocated or not.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(idx): the index of the element you wish to check
//
// @return: das_true if the element is allocated, otherwise das_false is returned
//
#define DasPool_is_idx_allocated(IdType, pool, idx) \
	_DasPool_is_idx_allocated((_DasPool*)pool, idx, sizeof(*(pool)->IdType##_address_space))
DasBool _DasPool_is_idx_allocated(_DasPool* pool, uint32_t idx, uintptr_t elmt_size);

//
// see if the supplied element identifier is valid and can be used to get an element.
//
// @param(IdType): the name of the element identifier made with typedef_DasPoolElmtId
//
// @param(pool): a pointer to the pool structure
//
// @param(elmt_id): the element identifier you wish to check
//
// @return: das_true if the element identifier is valid, otherwise das_false is returned
//
#define DasPool_is_id_valid(IdType, pool, elmt_id) \
	_DasPool_is_id_valid((_DasPool*)pool, elmt_id.IdType##_raw, sizeof(*(pool)->IdType##_address_space), IdType##_index_bits)
DasBool _DasPool_is_id_valid(_DasPool* pool, DasPoolElmtId elmt_id, uintptr_t elmt_size, uint32_t index_bits);

#endif

