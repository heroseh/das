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
#define das_noreturn __attribute__((das_noreturn))
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
#define das_cmp_array(a, b) (memcmp(a, b, sizeof(a)) == 0);
#define das_cmp_elmt(a, b) (memcmp(a, b, sizeof(*(a))) == 0);
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
// int* ptr = DasStk_push(&stack_of_ints);
// *ptr = value;
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
#define DasStk_count(stk_ptr) ((*stk_ptr) ? (*(stk_ptr))->count: 0)
#define DasStk_cap(stk_ptr) ((*stk_ptr) ? (*(stk_ptr))->cap: 0)
#define DasStk_elmt_size(stk_ptr) (sizeof(*(*(stk_ptr))->DasStk_data))
#define DasStk_alctor(stk_ptr) ((*stk_ptr) ? (*(stk_ptr))->alctor : DasAlctor_default)

//
// there is no DasStk_init, zeroed data is initialization.
// if you would like to initialize with a capacity just call
// DasStk_resize_cap on a zeroed stack.
// if you would like to initialize with a custom allocator.
// see DasStk_init_with_alctor.
//

//
// preallocates a stack with enough capacity to store init_cap number of elements.
// the memory is allocator with the supplied allocator and is used for all future reallocations.
#define DasStk_init_with_alctor(stk_ptr, init_cap, alctor) \
	_DasStk_init_with_alctor((_DasStkHeader**)stk_ptr, sizeof(**(stk_ptr)), init_cap, alctor, DasStk_elmt_size(stk_ptr))
extern DasBool _DasStk_init_with_alctor(_DasStkHeader** header_out, uintptr_t header_size, uintptr_t init_cap, DasAlctor alctor, uintptr_t elmt_size);

// deallocates the memory and sets the stack to being empty.
#define DasStk_deinit(stk_Ptr) _DasStk_deinit((_DasStkHeader**)stk_ptr, sizeof(**(stk_ptr)), DasStk_elmt_size(stk_ptr))
extern void _DasStk_deinit(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t elmt_size);

// removes all the elements from the deque
#define DasStk_clear(stk_ptr) ((*stk_ptr) ? (*(stk_ptr))->count = 0 : 0)

// returns a pointer to the element at idx.
// this function will abort if idx is out of bounds
#define DasStk_get(stk_ptr, idx) &(DasStk_data(stk_ptr)[_DasStk_assert_idx((_DasStkHeader*)*(stk_ptr), idx, DasStk_elmt_size(stk_ptr))])
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

// insert uninitialized element/s at the index position in the stack.
// the elements from the index position in the stack, will be shifted to the right.
// to make room for the inserted element/s.
// returns das_false on allocation failure, otherwise das_true
#define DasStk_insert(stk_ptr, idx) DasStk_insert_many(stk_ptr, idx, 1)
#define DasStk_insert_many(stk_ptr, idx, elmts_count) \
	_DasStk_insert_many((_DasStkHeader**)stk_ptr, sizeof(**(stk_ptr)), idx, elmts_count, DasStk_elmt_size(stk_ptr))
extern DasBool _DasStk_insert_many(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t idx, uintptr_t elmts_count, uintptr_t elmt_size);

// pushes uninitialized element/s onto the end of the stack
// returns the index of the first new element, but a UINTPTR_MAX will be returned on allocation failure.
#define DasStk_push(stk_ptr) DasStk_push_many(stk_ptr, 1)
#define DasStk_push_many(stk_ptr, elmts_count) \
	_DasStk_push_many((_DasStkHeader**)stk_ptr, sizeof(**(stk_ptr)), elmts_count, DasStk_elmt_size(stk_ptr))
extern uintptr_t _DasStk_push_many(_DasStkHeader** header_in_out, uintptr_t header_size, uintptr_t elmts_count, uintptr_t elmt_size);

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
// returns the index of the start where the string is placed in the stack, but a UINTPTR_MAX will be returned on allocation failure.
extern uintptr_t DasStk_push_str(DasStk(char)* stk, char* str);

// pushes the formatted string on to the end of a byte stack.
// returns the index of the start where the string is placed in the stack, but a UINTPTR_MAX will be returned on allocation failure.
extern uintptr_t DasStk_push_str_fmtv(DasStk(char)* stk, char* fmt, va_list args);
extern uintptr_t DasStk_push_str_fmt(DasStk(char)* stk, char* fmt, ...);

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
// int value = 22;
// DasDeque_push_back(&queue_of_ints, &value);
//
// DasDeque_pop_front(&queue_of_ints, &value);

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
// the memory is allocator with the supplied allocator and is used for all future reallocations.
#define DasDeque_init_with_alctor(deque_ptr, init_cap, alctor) \
	_DasStk_init_with_alctor((_DasStkHeader**)deque_ptr, sizeof(**(deque_ptr)), init_cap, alctor, DasStk_elmt_size(deque_ptr))
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

// pushes uninitialized element/s at the FRONT of the deque.
// they can be initialized with DasDeque_write or writing to a pointer returned by DasDeque_get
// returns das_false on allocation failure, otherwise das_true
#define DasDeque_push_front(deque_ptr) DasDeque_push_front_many(deque_ptr, 1)
#define DasDeque_push_front_many(deque_ptr, elmts_count) _DasDeque_push_front_many((_DasDequeHeader**)deque_ptr, sizeof(**(deque_ptr)), elmts_count, DasDeque_elmt_size(deque_ptr))
DasBool _DasDeque_push_front_many(_DasDequeHeader** header_in_out, uintptr_t header_size, uintptr_t elmts_count, uintptr_t elmt_size);

// pushes uninitialized element/s at the BACK of the deque.
// they can be initialized with DasDeque_write or writing to a pointer returned by DasDeque_get
// returns das_false on allocation failure, otherwise das_true
#define DasDeque_push_back(deque_ptr) DasDeque_push_back_many(deque_ptr, 1)
#define DasDeque_push_back_many(deque_ptr, elmts_count) _DasDeque_push_back_many((_DasDequeHeader**)deque_ptr, sizeof(**(deque_ptr)), elmts_count, DasDeque_elmt_size(deque_ptr))
DasBool _DasDeque_push_back_many(_DasDequeHeader** header_in_out, uintptr_t header_size, uintptr_t elmts_count, uintptr_t elmt_size);

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
// File Abstraction
//
//
// ===========================================================================

typedef uint8_t DasFileAccess;
enum {
	DasFileAccess_read,
	DasFileAccess_write,
	DasFileAccess_read_write,
};

typedef uint8_t DasFileFlags;
enum {
	DasFileFlags_TODO = 0x1,
};

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
typedef int DasFileHandle;
#else
typedef HANDLE DasFileHandle;
#endif

//
// TODO implement for windows that write documentation
// when we can find commonalities.
// currently implemented to open up a file for das_virt_mem_map_file
//
DasBool das_file_open(char* path, DasFileAccess access, DasFileFlags flags, DasFileHandle* file_handle_out);
DasBool das_file_close(DasFileHandle handle);
uint64_t das_file_size(DasFileHandle handle);
uintptr_t das_file_read(DasFileHandle handle, void* data_out, uintptr_t length);
uintptr_t das_file_write(DasFileHandle handle, void* data, uintptr_t length);

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
// 0 means success
typedef uint32_t DasVirtMemError;

//
// @return: the previous error of a virtual memory function call.
//          only call this directly after one of the virtual memory functions.
//          on Unix: this returns errno
//          on Windows: this returns GetLastError()
//
DasVirtMemError das_virt_mem_get_last_error();

typedef uint8_t DasVirtMemErrorStrRes;
enum {
	DasVirtMemErrorStrRes_success,
	DasVirtMemErrorStrRes_invalid_error_arg,
	DasVirtMemErrorStrRes_not_enough_space_in_buffer,
};

//
// get the string of @param(error) and writes it into the @param(buf_out) pointer upto @param(buf_out_len)
// @param(buf_out) is null terminated on success.
//
// @return: the result detailing the success or error.
//
DasVirtMemErrorStrRes das_virt_mem_get_error_string(DasVirtMemError error, char* buf_out, uint32_t buf_out_len);

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
// @return: das_false on failure, otherwise das_true is returned.
//          if errored you can get the error by calling das_virt_mem_get_last_error() directly after this call.
//
DasBool das_virt_mem_page_size(uintptr_t* page_size_out, uintptr_t* reserve_align_out);

//
// reserve a range of the virtual address space but does not commit any physical pages of memory.
// none of this memory cannot be used until das_virt_mem_commit is called.
//
// WARNING: on Windows, you cannot release sub sections of the address space.
//          you can only release the full reserved address space that is issued by this function call.
//          there are also restriction on protection, see das_virt_mem_protection_set.
//
// @param requested_addr: the requested start address you wish to reserve.
//             must be a aligned to the reserve_align that is retrieved from das_virt_mem_page_size function.
//             this is not guaranteed and is only used as hint.
//             NULL will not be used as a hint, instead the OS will choose an address for you.
//
// @param size: the size in bytes you wish to reserve from the @param(requested_addr)
//             must be a multiple of the reserve_align that is retrieved from das_virt_mem_page_size function.
//
// @return: NULL on error, otherwise the start of the reserved block of memory.
//          if errored you can get the error by calling das_virt_mem_get_last_error()
//          directly after this call
//
void* das_virt_mem_reserve(void* requested_addr, uintptr_t size);

//
// requests the OS to commit physical pages of memory to the the address space.
// this address space must be a full or subsection of the reserved address space with das_virt_mem_reserve.
// the memory in the commited address space will be zeroed after calling this function.
//
// @param addr: the start address of the memory you wish to commit.
//              must be a aligned to the page size das_virt_mem_page_size returns.
//              this is not guaranteed and is only used as hint.
//              NULL will not be used as a hint.
//
// @param size: the size in bytes you wish to reserve from the @param(addr)
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @param protection: what the memory is allowed to be used for
//
// @return: das_false on failure, otherwise das_true is returned.
//          if errored you can get the error by calling das_virt_mem_get_last_error() directly after this call.
//
DasBool das_virt_mem_commit(void* addr, uintptr_t size, DasVirtMemProtection protection);

//
// change the protection of a range of memory.
// this memory must have been reserved with das_virt_mem_reserve.
//
// WARNING: on Windows, you can change protection of any number pages
//          but that they all must come from the same call they where reserved with.
//
// @param addr: the start of the pages you wish to change the protection for.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @param size: the size in bytes of the memory you wish to change protection for.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @param protection: what the memory is allowed to be used for
//
// @return: das_false on failure, otherwise das_true is returned.
//          if errored you can get the error by calling das_virt_mem_get_last_error() directly after this call.
//
DasBool das_virt_mem_protection_set(void* addr, uintptr_t size, DasVirtMemProtection protection);

//
// gives the memory back to the OS but will keep the address space reserved
//
// @param addr: the start of the pages you wish to decommit.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @param size: the size in bytes of the memory you wish to release.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @return: das_false on failure, otherwise das_true is returned.
//          if errored you can get the error by calling das_virt_mem_get_last_error() directly after this call.
//
DasBool das_virt_mem_decommit(void* addr, uintptr_t size);

//
// gives the pages reserved back to the OS. the address range must have be reserved with das_virt_mem_reserve.
// you can target sub pages of the original allocation but just make sure the parameters are aligned.
//
// WARNING: on Windows, you cannot release sub sections of the address space.
//          you can only release the full reserved address space that is issued by das_virt_mem_reserve.
//          so @param(size) is ignored on Windows.
//
// @param addr: the start of the pages you wish to release.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @param size: the size in bytes of the memory you wish to release.
//             must be a aligned to the page size das_virt_mem_page_size returns.
//
// @return: das_false on failure, otherwise das_true is returned.
//          if errored you can get the error by calling das_virt_mem_get_last_error() directly after this call.
//
DasBool das_virt_mem_release(void* addr, uintptr_t size);

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
typedef void* DasMapFileHandle; // unused
#else
typedef HANDLE DasMapFileHandle;
#endif

//
// maps a file to the virtual address space of the process.
//
// @param requested_addr: see the same parameter in das_virt_mem_reserve for more info.
//
// @param file_handle: the handle to the file opened with das_file_open
//
// @param protection: what the memory is allowed to be used for.
//     this must match with the file_handle's access rights
//     see das_file_open and DasFileAccess.
//
// @param offset: offset into the file in bytes this does not have to be aligned in anyway.
//     internally it will round this down to the nearest reserve_align and map that.
//
// @param size: size of the file in bytes starting from the offset that you wish to map into the address space.
//     this does not have to be aligned in anyway but it will round this up to the page size internally and map that.
//
// @param map_file_handle_out: a pointer that is set apon success to the handle of the map file.
//     this is only used in Windows and must be passed into das_virt_mem_unmap_file to unmap correctly.
//
// @return: NULL on failure, otherwise the address pointing to the offset into the file that was requested.
//          if errored you can get the error by calling das_virt_mem_get_last_error() directly after this call.
//
void* das_virt_mem_map_file(void* requested_addr, DasFileHandle file_handle, DasVirtMemProtection protection, uint64_t offset, uintptr_t size, DasMapFileHandle* map_file_handle_out);


//
// unmaps a file that was mapped into the virtual address space by das_virt_mem_map_file.
//
// @param addr: the address that was returned when mapping the file.
//
// @param size: the size that was used when mapping the file.
//
// @param map_file_handle: the map file handle that was returned when mapping the file.
//
// @return: das_false on failure, otherwise das_true is returned.
//          if errored you can get the error by calling das_virt_mem_get_last_error() directly after this call.
//
DasBool das_virt_mem_unmap_file(void* addr, uintptr_t size, DasMapFileHandle map_file_handle);

#endif

