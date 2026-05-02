#ifndef MCPP98_TEST_HARNESS_HPP
#define MCPP98_TEST_HARNESS_HPP

#include <cstdio>

struct TestContext {
    int failed;
    const char* suite;
};

inline void check_impl(TestContext& ctx, bool ok, const char* expr, const char* file, int line)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL %s %s:%d %s\n", ctx.suite, file, line, expr);
        ++ctx.failed;
    }
}

#define CHECK(expr) check_impl(ctx, (expr) ? true : false, #expr, __FILE__, __LINE__)

#endif
