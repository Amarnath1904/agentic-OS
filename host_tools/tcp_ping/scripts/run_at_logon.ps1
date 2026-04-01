# Start TCP ping HTTP API in the background (no console, no outbound probes until /ping).
# Task Scheduler or Startup shortcut: powershell.exe -ExecutionPolicy Bypass -File "...\run_at_logon.ps1"

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$env:PYTHONPATH = $RepoRoot
Set-Location $RepoRoot

# Use full path to pythonw if needed, e.g. C:\Python312\pythonw.exe
Start-Process -FilePath "pythonw" -ArgumentList @("-m", "host_tools.tcp_ping", "serve") -WorkingDirectory $RepoRoot -WindowStyle Hidden
