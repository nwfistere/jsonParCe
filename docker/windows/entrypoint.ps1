param(
  [String] $s="ON",
  [String] $t="ON",
  [String] $c="Release"
)

$strict_mode = $s;
$run_tests = $s;
$config = $c;

$NUM_JOBS=(Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors;
$BUILD_DIR="./build/windows_${strict_mode}_${config}";
$CMAKE_OPTIONS=("-DCMAKE_BUILD_TYPE:STRING=${config} " +
  "-DJSON_PARCE_ENABLE_TEST:BOOL=ON " +
  "-DJSON_PARCE_ENABLE_EXAMPLE_COMPILE:BOOL=ON " +
  "-DJSON_PARCE_ENABLE_STRICT_MODE:BOOL=${strict_mode}");

Write-Output "BUILD_DIR: ${BUILD_DIR}";
Write-Output "strict_mode: ${strict_mode}";
Write-Output "run_tests: ${run_tests}";
Write-Output "config: ${config}";
Write-Output "CMAKE_OPTIONS: ${CMAKE_OPTIONS}";
Write-Output "NUM_JOBS: ${NUM_JOBS}";

if (Test-Path -Path $BUILD_DIR) {
  Write-Output "$BUILD_DIR exists, deleting."
  Remove-Item -Force -Recurse -Path $BUILD_DIR
  if (Test-Path -Path $BUILD_DIR) {
    Write-Output "Failed to delete $BUILD_DIR"
    exit 2
  }
} else {
  Write-Output "$BUILD_DIR doesn't exist."
}

$command = "cmake.exe"
$params = "-S . -B $BUILD_DIR $CMAKE_OPTIONS"
$params = $params.Split(" ")

# Mark all directories as safe to be owned by others.
# This is required to pull down dependencies.
& git config --global --add safe.directory "*"

# Execute cmake generation
& $command $params

# Execute build.

$params = "--build $BUILD_DIR --config $config"
$params = $params.Split(" ")

& $command $params

$command = "ctest.exe"
$params = "-V --output-on-failure -C $config --test-dir $BUILD_DIR"
$params = $params.Split(" ")

& $command $params 1> "$BUILD_DIR/test.stderr.log"

# & Get-WinSystemLocale

# & gci env:
