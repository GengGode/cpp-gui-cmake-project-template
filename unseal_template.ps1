function GetInputProjectName {
    param (
        [string]$projectName
    )

    while (-not $projectName.Trim()) {
        $projectName = Read-Host "Enter the project name (non-empty)"
    }

    return $projectName.Trim()
}

$name = GetInputProjectName
$identifier_name = $name -replace '[^a-zA-Z0-9]', '_'
$fileTypes = @("*.txt", "*.hpp")

Get-ChildItem -Recurse -Include $fileTypes | ForEach-Object {
    $content = (Get-Content $_.FullName -Raw)
    if ($content -match 'cpp_gui_cmake_project_template_identifier') {
        $newContent = $content -replace 'cpp_gui_cmake_project_template_identifier', $identifier_name
        Set-Content -Path $_.FullName -Value $newContent -NoNewline
        Write-Host "Updated: $($_.FullName)"
    }
}
Get-ChildItem -Recurse -Include $fileTypes | ForEach-Object {
    $content = (Get-Content $_.FullName -Raw)
    if ($content -match 'cpp_gui_cmake_project_template') {
        $newContent = $content -replace 'cpp_gui_cmake_project_template', $name
        Set-Content -Path $_.FullName -Value $newContent -NoNewline
        Write-Host "Updated: $($_.FullName)"
    }
}

git add *
git commit -am "unseal: init $name"
code .