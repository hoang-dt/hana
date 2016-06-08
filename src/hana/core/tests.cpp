#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "math.h"

using namespace hana::core;

TEST_CASE("log_int", "[core, math]")
{
    int p = log_int(2, 7);
    REQUIRE(p == 2);
    p = log_int(2, 2);
    REQUIRE(p == 1);
    p = log_int(2, 16);
    REQUIRE(p == 4);
    p = log_int(3, 1);
    REQUIRE(p == 0);
}
