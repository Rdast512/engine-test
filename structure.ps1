#Requires -Version 5.0
<#
.SYNOPSIS
    Generates a tree-like structure of a directory and saves it to a text file.

.DESCRIPTION
    This script recursively traverses a specified directory ('src' by default) and
    creates a text file representing the folder and file structure.

.PARAMETER Path
    The path to the source directory you want to map. Defaults to the 'src'
    folder in the current directory.

.PARAMETER OutputFile
    The name of the file to save the structure to. Defaults to 'project_structure.txt'.

.PARAMETER Exclude
    An array of directories or files to exclude (e.g., @("bin", "obj", "*.log")).

.EXAMPLE
    .\Create-ProjectTree.ps1

.EXAMPLE
    .\Create-ProjectTree.ps1 -Path "C:\MyProject\source" -OutputFile "structure.txt" -Exclude @("*.tmp", "node_modules")
#>
param (
    [string]$Path = ".\src",
    [string]$OutputFile = "project_structure.txt",
    [string[]]$Exclude = @("bin", "obj", "node_modules", ".git", ".vs", ".vscode")
)

function Get-DirectoryTree {
    param (
        [string]$Directory,
        [string]$Indent = "",
        [switch]$IsLast
    )

    # Get child items, excluding specified items
    $items = Get-ChildItem -Path $Directory -Exclude $Exclude -Force -ErrorAction SilentlyContinue

    for ($i = 0; $i -lt $items.Count; $i++) {
        $item = $items[$i]
        $lastItem = ($i -eq $items.Count - 1)
        $line = $Indent

        if ($IsLast) {
            $line += "    "
        } else {
            $line += "│   "
        }

        if ($lastItem) {
            $line += "└── "
        } else {
            $line += "├── "
        }

        $line += $item.Name
        $line | Out-File -FilePath $OutputFile -Append -Encoding utf8

        if ($item.PSIsContainer) {
            Get-DirectoryTree -Directory $item.FullName -Indent ($Indent + ($IsLast ? "    " : "│   ")) -IsLast:$lastItem
        }
    }
}

# Start with the root directory
$Path | Out-File -FilePath $OutputFile -Encoding utf8
Get-DirectoryTree -Directory $Path

Write-Host "Project structure has been written to $OutputFile"