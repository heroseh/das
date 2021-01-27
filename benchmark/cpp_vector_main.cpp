#include "shared.h"
#include <vector>

int main(int argc, char** argv) {

#define STRUCT(type) \
	std::vector<type> s_##type = std::vector<type>(); \
 \
	type v_##type = {0}; \
	for (int i = 0; i < test_iteration_count; i += 1) { \
		v_##type.a = i; \
		v_##type.b = i; \
		v_##type.c = i; \
		v_##type.d = i; \
		s_##type.push_back(v_##type); \
	}
	/* end */

	STRUCT_LIST

#undef STRUCT

	return 0;
}

