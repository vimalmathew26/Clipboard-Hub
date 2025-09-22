param(
  [string]$Text = "ClipboardHub smoke",
  [switch]$Quiet
)

# Write to clipboard
$Text | clip
Start-Sleep -Milliseconds 120

# Read back
$val = Get-Clipboard -Raw

if (-not $Quiet) {
  Write-Host "Wrote: '$Text'"
  Write-Host "Read : '$val'"
}

if ($val -ne $Text) {
  Write-Error "Mismatch between written and read clipboard content."
  exit 1
}

if (-not $Quiet) { Write-Host "Clipboard roundtrip OK." }
exit 0
