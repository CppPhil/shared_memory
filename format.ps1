Push-Location .

$time_string = Get-Date -Format "HH:mm:ss" | Out-String
Write-Output ">>> format.ps1: starting formatting $time_string"
& "C:\Program Files\Git\git-bash.exe" format.sh | Wait-Process
$time_string = Get-Date -Format "HH:mm:ss" | Out-String
Write-Output "<<< format.ps1: Done $time_string"

Pop-Location

exit 0