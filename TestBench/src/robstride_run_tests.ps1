# Run from repository root (PowerShell):
# .\TestBench\src\robstride_run_tests.ps1
# If scripts are blocked, copy/paste the two compilation commands from README.
$ErrorActionPreference = 'Stop'
$tests = @('robstride_diagnostics_test', 'robstride_csv_integration_test')
foreach ($test in $tests) {
    Write-Host "`nBuilding $test ..."
    & g++ -std=c++17 -Wall -Wextra -pedantic "TestBench/src/$test.cpp" `
        -o "TestBench/src/$test.exe"
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $test" }
    & "./TestBench/src/$test.exe"
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $test" }
}
Write-Host "`nAll Robstride diagnostics tests passed."
