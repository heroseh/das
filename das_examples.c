#include "das.h"
#include "das.c"

void stk_example() {
	// a zeroed stack is the default,
	// you can actually start pushing elements directly on this.
	DasStk(int) stk = 0;


	// you can also preallocate the stack to hold atleast 6 int elements.
	DasStk_resize_cap(&stk, 6);



	// pushes an uninitialized element on the end of the stack.
	// returns an index to the first new element in the stack.
	// idx will be UINTPTR_MAX if allocation failed.
	// it is then initialize to a value of zero.
	// our stack then looks like this: [ 0 ]
	uintptr_t idx = DasStk_push(&stk);
	*DasStk_get(&stk, idx) = 0;



	// pushes 1, 2, 3, 4 on the end of the stack.
	// our stack then looks like this: [ 0, 1, 2, 3, 4 ]
	for (int i = 1; i < 5; i += 1) {
		idx = DasStk_push(&stk);
		*DasStk_get(&stk, idx) = i;
	}



	// get the number of elements in the stack with the DasStk_count macro
	// this will print: DasStk_count(&stk) = 5
	printf("DasStk_count(&stk) = %zu\n", DasStk_count(&stk));



	// get the number of elements the stack can hold before reallocating with the DasStk_cap macro.
	// this will print: DasStk_cap(&stk) = 6 or DasStk_min_cap
	printf("DasStk_cap(&stk) = %zu\n", DasStk_cap(&stk));



	// the DasStk_data macro returns a pointer to the first element in the stack.
	// and elements are stored sequentially.
	// so DasStk_data(&stk)[0] is our value 0
	// and DasStk_data(&stk)[1] is our value 1
	// this is an UNSAFE way of accessing data, since it does not check the size of stack.
	// this way you can access uninitialized memory or go out of the stack's memory boundary.
	printf("[0] = %d and [1] = %d\n", DasStk_data(&stk)[0], DasStk_data(&stk)[1]);



	// the SAFE way to get a pointer to the stack is to use the DasStk_get macro.
	// this will abort the program if you provide an index that is goes out of bounds.
	// since DasStk_count(&stk) == 5, our highest index is 4 and any more will cause an abort.
	// be aware that keeping the pointer and calling more DasStk functions to cause
	// a reallocation is UNSAFE. the DasStk_data(&stk) pointer is likely change when memory is
	// reallocated.
	// FYI pushing, inserting and resizing the stack can cause a reallocation.
	printf("[2] = %d and [3] = %d\n", *DasStk_get(&stk, 2), *DasStk_get(&stk, 3));



	// pushes 5 uninitialized elements on the end of the stack.
	// returns an index to the first new element in the stack.
	// idx will be UINTPTR_MAX if allocation failed.
	// the elements are then is then initialize with a loop.
	idx = DasStk_push_many(&stk, 5);
	for (uintptr_t i = 5; i < 10; i += 1) {
		*DasStk_get(&stk, i) = i;
	}



	// pops the element off the end of the stack and we do nothing with the value.
	// our stack then looks like this: [ 0, 1, 2, 3, 4, 5, 6, 7, 8 ]
	DasStk_pop(&stk);



	// pops the element off the end of the stack.
	// but before we do, we put the element that will be removed, into a variable.
	// our stack then looks like this: [ 0, 1, 2, 3, 4, 5, 6, 7 ]
	// popped_value will be set to 8
	int popped_value = *DasStk_get_last(&stk);
	DasStk_pop(&stk);



	// pops the elements off the end of the stack into an array.
	// but before we do, we copy the elements that will be removed, into an array
	// our stack then looks like this: [ 0, 1, 2, 3, 4 ]
	// int_buf look like this: [ 5, 6, 7 ]
	int int_buf[3] = {0};
	das_copy_elmts(int_buf, DasStk_get_back(&stk, 2), 3);
	DasStk_pop_many(&stk, 3);



	// removes an element from the stack by copying it's right neighbouring elements to the left.
	// so remove the index of 2.
	// our stack then looks like this: [ 0, 1, 3, 4 ]
	DasStk_remove_shift(&stk, 2);



	// removes an element from the stack by copying it's right neighbouring elements to the left.
	// so remove the index of 2.
	// our stack then looks like this: [ 0, 1, 4 ]
	// before we remove, we store the element in the removed_value variable.
	int removed_value = *DasStk_get(&stk, 2);
	DasStk_remove_shift(&stk, 2);



	// removes elements from the stack by copying it's right neighbouring elements to the left.
	// so remove 2 elements from the index of 0.
	// our stack then looks like this: [ 4 ]
	// pair will be set to [ 0, 1 ]
	int pair[2] = {0};
	das_copy_elmts(pair, DasStk_get(&stk, 0), 2);
	DasStk_remove_shift_range(&stk, 0, 2);



	// clear the stack by setting the count to 0
	DasStk_clear(&stk);



	// lets get our stack to look like this again: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ]
	int zero_to_nine[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	DasStk_push_many(&stk, 10);
	for (uintptr_t i = 0; i < 10; i += 1) {
		*DasStk_get(&stk, i) = i;
	}



	// removes an element from the stack by replacing it with the element at the end.
	// so remove the index of 4.
	// our stack then looks like this: [ 0, 1, 2, 3, 9, 5, 6, 7, 8 ]
	// this does not maintain the same order but is faster than a shift.
	// as we are only copying elements from the end to replace the ones we remove.
	// as opposed to shifting all of the right neighbouring elements.
	DasStk_remove_swap(&stk, 4);



	// removes an element from the stack by replacing it with the element at the end.
	// copyings the removed element to a variable.
	// so remove the index of 4.
	// our stack then looks like this: [ 0, 1, 2, 3, 8, 5, 6, 7 ]
	// removed_value will be set to 9
	removed_value = *DasStk_get(&stk, 4);
	DasStk_remove_swap(&stk, 4);



	// removes an element from the stack by replacing it with the element at the end.
	// copyings the removed element to a variable.
	// so elements from the index of 3 to 5 exclusively.
	// our stack then looks like this: [ 0, 1, 2, 6, 7, 5 ]
	// pair will be set to [ 3, 8 ]
	das_copy_elmts(pair, DasStk_get(&stk, 3), 2);
	DasStk_remove_swap_range(&stk, 3, 5);



	// insert an element at an index by shifting the elements from that point to the right.
	// insert with an index of 3
	// our stack then looks like this: [ 0, 1, 2, 77, 6, 7, 5 ]
	DasStk_insert(&stk, 3);
	*DasStk_get(&stk, 3) = 77;



	// make room for an element at an index by shifting the elements from that point to the right.
	// get a pointer back so you can write to the new location.
	// this is useful for inserting big data structures,
	// so you don't have to construct it on the call stack first.
	// our stack then looks like this: [ 0, 1, 67, 2, 77, 6, 7, 5 ]
	DasStk_insert(&stk, 2);
	*DasStk_get(&stk, 2) = 67;

	//
	// we also have DasStk_insert_many that works like DasStk_insert
	// but for adding multiple elements like DasStk_push_many.
	//


}

typedef struct {
	int a, b;
} CustomType;

//
// to use a custom type with DasStk you need to create a stack type for that type.
// use this macro to do that.
typedef_DasStk(CustomType);

// now we can have a CustomType stack
DasStk(CustomType) custom_type_stk;

//
// you have to make sure the type names are a single string with no spaces or symbols.
// an error will be thrown if you try to use a pointer in typedef_DasStk directly.
typedef CustomType* CustomTypePtr;
typedef_DasStk(CustomTypePtr);

// now we can have a CustomType* stack
DasStk(CustomTypePtr) custom_type_ptr_stk;


// an error will be thrown if you use any type qualifier with typedef_DasStk directly.
typedef unsigned int unsigned_int;
typedef_DasStk(unsigned_int);

// now we can have a unsigned int stack
DasStk(CustomTypePtr) unsigned_int_stk;

void deque_example() {
	//
	// a double ended queue implemented as a ring buffer.
	// it allows you to push and pop from both end and not suffer the performance issues a stack will.
	// elements are store sequentially like a stack but can be in two halfs.
	// internally the structure will keep pointers to the front and back of the deque.
	//
	// so a deque can have the following elements:
	// [ 0, 1, 2, 3, 4, 5, 6, 7 ]
	//
	// the value 0 is the front of the deque and is accessed with index 0
	// the value 7 is the back of the deque and is accessed with index 7
	//
	// in memory the deque can be stored like so:
	//            B       F
	// [ 5, 6, 7, . . . . 0, 1, 2, 3, 4, ]
	//
	// F is the front pointer that always points to the first element.
	// B is the back pointer that always points to the element after the element at the back.
	//
	// a zeroed deque is the default,
	// you can actually start pushing elements directly on this.
	DasDeque(int) deque = NULL;



	// initializes the deque to hold 6 int elements before needing to reallocate.
	DasDeque_resize_cap(&deque, 6);



	// push elements 0 to 10 on the back of the deque one by one.
	// deque data will look like: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ]
	for (int i = 0; i < 10; i += 1) {
		DasDeque_push_back(&deque);
		*DasDeque_get_last(&deque) = i;
	}



	// we can pop multiple elements off the front.
	// deque data will look like: [ 3, 4, 5, 6, 7, 8, 9 ]
	// next_batch will look like: [ 0, 1, 2 ]
	int next_batch[3] = {0};
	DasDeque_read(&deque, 0, next_batch, 3);
	DasDeque_pop_front_many(&deque, 3);



	// we can also pop from the back as well.
	// deque data will look like: [ 3, 4, 5, 6, 7, 8 ]
	// popped_elmt will be 9
	int popped_elmt = *DasDeque_get_last(&deque);
	DasDeque_pop_back(&deque);



	// we can then push a value on the front.
	// deque data will look like: [ 9, 3, 4, 5, 6, 7, 8 ]
	// popped_elmt will be 9
	popped_elmt = *DasDeque_get(&deque, 0);
	DasDeque_push_front(&deque);


	// get 3rd element in the queue.
	// you cannot trust that the previous or next element will neighbour this one in memory.
	// so call this function for every index you wish to get.
	// or use the DasDeque_read/write functions.
	int* third_elmt = DasDeque_get(&deque, 2);



	// because this is not a stack, we have to get the
	// number of elements a different way.
	printf("deque.count = %zu\n", DasDeque_count(&deque));



	// the capacity also works the same way.
	printf("deque.cap = %zu\n", DasDeque_cap(&deque));
}


//
// to use a custom type with DasDeque you need to create a deque type for that type.
// use this macro to do that.
typedef_DasDeque(CustomType);

// now we can have a CustomType deque
DasDeque(CustomType) custom_type_deque;

//
// you have to make sure the type names are a single string with no spaces or symbols.
// an error will be thrown if you try to use a pointer in typedef_DasDeque directly.
typedef CustomType* CustomTypePtr;
typedef_DasDeque(CustomTypePtr);

// now we can have a CustomType* deque
DasDeque(CustomTypePtr) custom_type_ptr_deque;


// an error will be thrown if you use any type qualifier with typedef_DasDeque directly.
typedef unsigned int unsigned_int;
typedef_DasDeque(unsigned_int);

// now we can have a unsigned int deque
DasDeque(CustomTypePtr) unsigned_int_deque;


typedef struct {
	void* data;
	int whole_number;
	float number;
	double x;
	double y;
} SomeStruct;

void alloc_example() {
	//
	// internally the DasStk and DasDeque use the allocation API to
	// re/de/allocate their buffers that hold elements.
	//

	// dynamically allocate an element of a type using the default allocator, like so:
	SomeStruct* some_struct = das_alloc_elmt(SomeStruct, DasAlctor_default);



	// data will be uninitialized unless the allocator explicitly zeros it for you.
	// by default it does not, so we will initialize the memory here by zeroing it.
	das_zero_elmt(some_struct);



	// ... later on, deallocate the memory like so
	das_dealloc_elmt(SomeStruct, DasAlctor_default, some_struct);



	// dynamically allocate an array of 12 elements, like so:
	SomeStruct* some_array = das_alloc_array(SomeStruct, DasAlctor_default, 12);
	memset(some_array, 0, sizeof(SomeStruct) * 12);



	// data is stored sequentially in an array.
	// setting all the bytes in the fifth element 0xac.
	SomeStruct* ptr_to_fifth_elmt = &some_array[4];
	memset(ptr_to_fifth_elmt, 0xac, sizeof(SomeStruct));



	// dynamically reallocate the array from 12 to 20 elements, like so:
	some_array = das_realloc_array(SomeStruct, DasAlctor_default, some_array, 12, 20);


	// get the new pointer to the fifth element become the pointer probably changed.
	ptr_to_fifth_elmt = &some_array[4];

	int whole_number = 0xacacacac;
	das_assert(
		memcmp(&ptr_to_fifth_elmt->whole_number, &whole_number, sizeof(int)) == 0,
		"will not fail since: reallocating preserves the original memory");



	// deallocate an array of 20 elements, like so:
	das_dealloc_array(SomeStruct, DasAlctor_default, some_array, 20);



	// dynamically allocate 200 integers that are aligned to 1024.
	// void* das_alloc(uintptr_t size_in_bytes, uintptr_t align_in_bytes)
	int* ints_big_align = (int*)das_alloc(DasAlctor_default, 200 * sizeof(int), 1024);
	das_assert(
		(uintptr_t)ints_big_align % 1024 == 0,
		"will not fail since: alignment means that the pointer is a multiple of 1024");



	// reallocation works like the array one but with bytes instead
	// void* das_realloc(void* ptr, uintptr_t old_size_in_bytes, uintptr_t size_in_bytes, uintptr_t align_in_bytes)
	ints_big_align = das_realloc(DasAlctor_default, ints_big_align, 200 * sizeof(int), 400 * sizeof(int), 1024);



	// dynamically deallocate 400 integers that are aligned to 1024.
	das_dealloc(DasAlctor_default, ints_big_align, 400 * sizeof(int), 1024);
}




//
// as you may have noticed, we dont have any error checking for out of memory.
// this is because a callback is called when an allocator fails to allocate memory.
// this is to keep user code simple by not having to explicitly handle errors all the time.
// also means you dont have to try and return this up a call stack and architect most of your functions
// to handle when out of memory happens.
//
// here is a example out of memory handler. ideally the application should only be writing these.
// but maybe there are use cases for libraries/frameworks to do this.
//
// 'data' is a custom userdata pointer that can be used for anything we like.
// 'alctor' is a pointer to the current allocator, so you can manipulate this if you need.
// 'size' is the required amount for the allocation to succeed.
// return das_true if you think you have solved the problem.
// 		it will try to allocate again and then will aborts on another failure.
// return das_false and the program will abort.
DasBool basic_out_of_mem_handler(void* data, DasAlctor* alctor, uintptr_t size) {
	//
	// here we gracefully close the application by saving all the state we need.
	// and then return das_false.
	// or
	// we can try to resolve the problems by manipulating the allocator.
	// and then return das_true.
	//

	return das_true;
}



//
// here is a implementation of a linear allocator.
// structurely it looks like a stack but will only grow until it is manually reset.
// this is pretty much, the most simple memory allocator.
//
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

void custom_allocator_example() {
	// the buffer is zeroed, so the linear allocator will return zeroed memory
	char buffer[1024] = {0};
	LinearAlctor la = { .data = buffer, .pos = 0, .size = sizeof(buffer) };
	DasAlctor alctor = LinearAlctor_as_das(&la);

	float* floats = das_alloc_array(float, alctor, 256);
	for (int i = 0; i < 256; i += 1) {
		das_assert(floats[i] == 0.f, "will not fail since: linear allocator memory is zero");
	}
}

int main(int argc, char** argv) {
	stk_example();
	deque_example();
	alloc_example();
	custom_allocator_example();

	return 0;
}

