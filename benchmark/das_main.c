#include "shared.h"
#include "../das.h"
#include "../das.c"

#define STRUCT(type) \
	typedef_DasStk(type); \
	/* end */

	STRUCT_LIST

#undef STRUCT

int main(int argc, char** argv) {

#define STRUCT(type) \
	DasStk(type) s_##type = NULL; \
	DasStk_resize_cap(&s_##type, test_iteration_count); \
 \
	type v_##type = {0}; \
	for (int i = 0; i < test_iteration_count; i += 1) { \
		v_##type.a = i; \
		v_##type.b = i; \
		v_##type.c = i; \
		v_##type.d = i; \
		uintptr_t idx = DasStk_push(&s_##type); \
		*DasStk_get(&s_##type, idx) = v_##type; \
	} \
	/* end */

	STRUCT_LIST

#undef STRUCT

	return 0;
}

