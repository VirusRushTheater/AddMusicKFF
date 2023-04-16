#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <filesystem>

#include "asarBinding.h"
#include "Utility.h"

using namespace AddMusic;
namespace fs = std::filesystem;

const fs::path DRIVER_DIR {"../snes-driver"};

TEST_CASE("asarBinding compilation to memory", "[addmusick][asarbinding][compilation]")
{
    AsarBinding asar (DRIVER_DIR / "asm" / "main.asm");
    REQUIRE(true);
}