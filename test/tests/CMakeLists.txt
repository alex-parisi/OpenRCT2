cmake_minimum_required(VERSION 3.20)
include(GoogleTest)

find_package(GTest REQUIRED)

file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/testdata" "${CMAKE_BINARY_DIR}/testdata" SYMBOLIC)
install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_DATADIR}/openrct2\" \"${CMAKE_BINARY_DIR}/data\")")

set(test_files
   "${CMAKE_CURRENT_SOURCE_DIR}/AssertHelpers.hpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/BitSetTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/BoundboxTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/CheatActionTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/ChecksumStreamTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/CircularBuffer.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/CLITests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/CollectionsTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/ColourWithFlagsTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/CryptTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/Endianness.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/EntranceElementTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/EnumMapTest.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/FormattingTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/GuardTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/HeadlessRenderTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/ImageImporterTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/ImageIdTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/IniReaderTest.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/IniWriterTest.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/InvalidationGridTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/IStreamTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/LanguagePackTest.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/LocalisationTest.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/MapSelectionTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/MoneyTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/MultiLaunch.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/fixture/PaintTestFixture.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/Pathfinding.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/Platform.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/PlayTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/QuarterTileTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/Rct1CsgTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/Rct1TablesTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/ReplayTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/RideRatings.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/EntityImportTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/S6ImportExportTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/SawyerCodingTest.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/ScenarioPatcherTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/SegmentTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/SpriteMapTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/ScriptingTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/ShopItemTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/StringTest.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/SurfaceDataTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/TestData.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/TestData.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/TileElements.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/TileElementsView.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/TrackDispatchSweepTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/UnitConversionTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/UtilTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/WidgetStateTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/YawTests.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/ZoomLevelTests.cpp")

add_executable(OpenRCT2Tests ${test_files})
add_executable(OpenRCT2::OpenRCT2Tests ALIAS OpenRCT2Tests)

target_link_libraries(OpenRCT2Tests GTest::gtest GTest::gtest_main libopenrct2)
target_include_directories(OpenRCT2Tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../src")
set_target_properties(OpenRCT2Tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Paint tests need the generated graphics blobs (g2/fonts/palettes/tracks.dat) on disk at the path
# PlatformEnvironment::GetInstallPath() resolves to — i.e. ${CMAKE_BINARY_DIR}/data/*.dat (which is
# the bin/data symlink target). The `graphics` target produces them in ${CMAKE_BINARY_DIR}; without
# them, every paint helper that looks up a palette/remap entry spams LOG_WARNING and the rendered
# output is degraded. Depend on `graphics` so they're built alongside the test binary, then copy the
# four files into place via a post-build step. (The destination files are gitignored.)
if (TARGET graphics)
    add_dependencies(OpenRCT2Tests graphics)
    foreach(dat_file g2 fonts palettes tracks)
        add_custom_command(TARGET OpenRCT2Tests POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${CMAKE_BINARY_DIR}/${dat_file}.dat"
                "${CMAKE_BINARY_DIR}/data/${dat_file}.dat"
            COMMENT "Staging ${dat_file}.dat into the test runtime data dir"
            VERBATIM)
    endforeach()
endif()
# CheatActionTests and the Paint* suite are excluded from discovery and registered as single ctest
# entries because each uses SetUpTestSuite to share expensive one-time work across many tests
# (CheatActionTests: an imported park; PaintTestFixture: a graphics-loaded Context, plus per-test
# park loads). The Paint* filter catches both plain TEST_F's on PaintTestFixture and parameterized
# TEST_P's instantiated under the `Paint` prefix (full gtest name: `Paint/PaintFoo.Bar/<param>`).
gtest_discover_tests(OpenRCT2Tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DISCOVERY_MODE PRE_TEST
    TEST_FILTER -CheatActionTests.*:Paint*)

add_test(NAME CheatActionTests
    COMMAND OpenRCT2Tests --gtest_filter=CheatActionTests.*
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_test(NAME PaintTests
    COMMAND OpenRCT2Tests --gtest_filter=Paint*
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

