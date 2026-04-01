# Interactive shell: type "ping <ipv4> [port]" (TCP connect, not ICMP).
# Double-click or: powershell.exe -ExecutionPolicy Bypass -File "...\open_ping_shell.ps1"

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$env:PYTHONPATH = $RepoRoot
Set-Location $RepoRoot

python -m host_tools.tcp_ping shell
