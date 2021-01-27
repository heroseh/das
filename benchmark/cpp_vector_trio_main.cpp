#include <vector>


struct Test0 {
	int a, b, c, d;
};
struct Test1 {
	int a, b, c, d;
};
struct Test3 {
	int a, b, c, d;
};

int main(int argc, char** argv) {
	std::vector<Test0> s = std::vector<Test0>(); \

	Test0 v = {0}; \
	for (int i = 0; i < 2048; i += 1) { \
		v.a = i; \
		v.b = i; \
		v.c = i; \
		v.d = i; \
		s.push_back(v); \
	}

	std::vector<Test1> e = std::vector<Test1>(); \

	Test1 g = {0}; \
	for (int i = 0; i < 2048; i += 1) { \
		g.a = i; \
		g.b = i; \
		g.c = i; \
		g.d = i; \
		e.push_back(g); \
	}

	std::vector<Test3> t = std::vector<Test3>(); \

	Test3 j = {0}; \
	for (int i = 0; i < 2048; i += 3) { \
		j.a = i; \
		j.b = i; \
		j.c = i; \
		j.d = i; \
		t.push_back(j); \
	}

	return 0;
}

