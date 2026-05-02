#include <cstdio>

int test_json();

int main()
{
    int failed = 0;
    failed += test_json();
    if (failed != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", failed);
        return 1;
    }
    std::printf("ok\n");
    return 0;
}
