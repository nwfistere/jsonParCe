param(
  [String] $s="ON",
  [String] $t="ON",
  [String] $c="Release",
  [String] $r="OFF"
)

$strict_mode = $s;
$run_tests = $s;
$config = $c;
$use_windows_newlines = $r

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
((git config --global --list | Select-String -Pattern "safe\.directory\s?=\s?\*") -or (git config --global --add safe.directory "*"))

& git config --global core.pager "cat"
& git config --global core.autocrlf "input"
& git config --list

# Execute cmake generation
& $command $params

# Execute build.

$params = "--build $BUILD_DIR -j $NUM_JOBS --config $config"
$params = $params.Split(" ")

& $command $params

if ($use_windows_newlines -eq "ON") {
  $json_test_file_dir = Join-Path $BUILD_DIR _deps jsontestsuite-src test_parsing

  Write-Output "Checking: $json_test_file_dir"

  # Foreach file in json_test_file_dir replace all newlines with crlf.
  Get-ChildItem $json_test_file_dir -Filter *.json |
  ForEach-Object {
    Write-Output "Found $_.FullName"
    (Get-Content $_.FullName).Replace("`n", "`r`n") | Set-Content $_.FullName
  }
}

$command = "ctest.exe"
$params = "-V --output-on-failure -j $NUM_JOBS -C $config --test-dir $BUILD_DIR"
$params = $params.Split(" ")

& $command $params

exit 0;
