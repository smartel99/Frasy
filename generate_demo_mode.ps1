<#
.SYNOPSIS
    Tool to generate the project files for the demo mode of Frasy.
.DESCRIPTION
    Tool to generate the project files for the demo mode of Frasy.
.PARAMETER Action
    Action to use in premake.
#>

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('cmake', 'clion', 'vs2019', 'vs2022')]
    [string]$Action
)

$og_pwd = $pwd

cd $PSScriptRoot\demo_mode

& $PSScriptRoot\\vendor\\bin\\premake\\premake5.exe $Action

cd $og_pwd