
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

extern "C" EXPORT void no_args() {}

extern "C" EXPORT int add_one(int a) { return a + 1; }

extern "C" EXPORT int increment() {
	static int i = 0;
	return ++i;
}

extern "C" EXPORT bool advanced(const char* c, unsigned int n) {
	if (!c) return false;

	unsigned int s = 0;
	for (; s < n; ++s) {
		if (c[s] == '\0') break;
	}
	return s == n;
}