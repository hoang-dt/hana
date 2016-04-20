#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "utils.h"

using namespace hana::core;
using namespace hana::idx;

TEST_CASE("guess_bit_string", "[idx, utils]")
{
    char buf[64] = { 'V' };
    StringRef bit_string(buf + 1);
    guess_bit_string(Vector3i(2, 2, 2), bit_string);
    REQUIRE(bit_string == "210");
    guess_bit_string(Vector3i(4, 2, 2), bit_string);
    REQUIRE(bit_string == "0210");
    guess_bit_string(Vector3i(4, 2, 4), bit_string);
    REQUIRE(bit_string == "20210");
    guess_bit_string(Vector3i(7, 6, 1), bit_string);
    REQUIRE(bit_string == "101010");
    guess_bit_string(Vector3i(9, 6, 4), bit_string);
    REQUIRE(bit_string == "010210210");
}
