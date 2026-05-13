#include <cstdio>

int test_json();
int test_session();
int test_server();

#if __cplusplus >= 201103L
int test_ext_11();
int test_tool_builder();
#endif

#if __cplusplus >= 202302L
int test_ext_23();
#endif

int main()
{
    int failed = 0;
    failed += test_json();
    failed += test_session();
    failed += test_server();
#if __cplusplus >= 201103L
    failed += test_ext_11();
    failed += test_tool_builder();
#endif
#if __cplusplus >= 202302L
    failed += test_ext_23();
#endif
    if (failed != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", failed);
        return 1;
    }
    std::printf("ok\n");
    return 0;
}
