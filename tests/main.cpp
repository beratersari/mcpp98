#include <cstdio>

int test_json();
int test_session();
int test_server();

int main()
{
    int failed = 0;
    failed += test_json();
    failed += test_session();
    failed += test_server();
    if (failed != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", failed);
        return 1;
    }
    std::printf("ok\n");
    return 0;
}
