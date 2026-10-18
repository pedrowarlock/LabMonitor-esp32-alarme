param(
  [switch]$ExtractFromCpp
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourcePath = Join-Path $root "webserver.cpp"
$webPath = Join-Path $root "web"
$headerPath = Join-Path $root "web_assets.hpp"
$logoPath = Join-Path $root "logo.jpg"

if ($ExtractFromCpp) {
  $source = Get-Content -Raw -Encoding UTF8 -LiteralPath $sourcePath
  $match = [regex]::Match($source, 'String html = R"\(([\s\S]*?)\)"\s*;')
  if (-not $match.Success) {
    throw "Nao foi possivel localizar o dashboard HTML em webserver.cpp."
  }

  $html = $match.Groups[1].Value
  $styleMatch = [regex]::Match($html, '<style>([\s\S]*?)</style>')
  $scriptMatch = [regex]::Match($html, '<script>([\s\S]*?)</script>')
  if (-not $styleMatch.Success -or -not $scriptMatch.Success) {
    throw "O dashboard precisa conter exatamente um bloco style e um bloco script."
  }

  $style = $styleMatch.Groups[1].Value.Trim() + "`r`n"
  $script = $scriptMatch.Groups[1].Value.Trim() + "`r`n"
  $html = $html.Substring(0, $styleMatch.Index) + '<link rel="stylesheet" href="/style.css">' + $html.Substring($styleMatch.Index + $styleMatch.Length)
  $scriptMatchAfterStyle = [regex]::Match($html, '<script>([\s\S]*?)</script>')
  $html = $html.Substring(0, $scriptMatchAfterStyle.Index) + '<script src="/script.js"></script>' + $html.Substring($scriptMatchAfterStyle.Index + $scriptMatchAfterStyle.Length)
  $html = $html.Trim() + "`r`n"

  New-Item -ItemType Directory -Force -Path $webPath | Out-Null
  [System.IO.File]::WriteAllText((Join-Path $webPath "index.html"), $html, [System.Text.UTF8Encoding]::new($false))
  [System.IO.File]::WriteAllText((Join-Path $webPath "style.css"), $style, [System.Text.UTF8Encoding]::new($false))
  [System.IO.File]::WriteAllText((Join-Path $webPath "script.js"), $script, [System.Text.UTF8Encoding]::new($false))
} else {
  $html = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $webPath "index.html")
  $style = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $webPath "style.css")
  $script = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $webPath "script.js")
}

function Convert-ToCArray {
  param([string]$Name, [byte[]]$Bytes)
  $builder = [System.Text.StringBuilder]::new()
  [void]$builder.AppendLine("const uint8_t $Name[] PROGMEM = {")
  for ($index = 0; $index -lt $Bytes.Length; $index++) {
    if (($index % 16) -eq 0) { [void]$builder.Append("  ") }
    [void]$builder.Append(('0x{0:X2}' -f $Bytes[$index]))
    if ($index -lt ($Bytes.Length - 1)) { [void]$builder.Append(', ') }
    if ((($index + 1) % 16) -eq 0) { [void]$builder.AppendLine() }
  }
  if (($Bytes.Length % 16) -ne 0) { [void]$builder.AppendLine() }
  [void]$builder.AppendLine('};')
  return $builder.ToString()
}

function Convert-ToHtmlAscii {
  param([string]$Text)
  $builder = [System.Text.StringBuilder]::new()
  for ($index = 0; $index -lt $Text.Length; $index++) {
    $codePoint = [char]::ConvertToUtf32($Text, $index)
    if ($codePoint -gt 0xFFFF) { $index++ }
    if ($codePoint -lt 128) {
      [void]$builder.Append([char]$codePoint)
    } else {
      [void]$builder.Append(('&#x{0:X};' -f $codePoint))
    }
  }
  return $builder.ToString()
}

function Convert-ToJavaScriptAscii {
  param([string]$Text)
  $builder = [System.Text.StringBuilder]::new()
  for ($index = 0; $index -lt $Text.Length; $index++) {
    $codePoint = [char]::ConvertToUtf32($Text, $index)
    if ($codePoint -gt 0xFFFF) { $index++ }
    if ($codePoint -lt 128) {
      [void]$builder.Append([char]$codePoint)
    } else {
      if ($codePoint -le 0xFFFF) {
        [void]$builder.Append(('\u{0:X4}' -f $codePoint))
      } else {
        [void]$builder.Append(('\u{{{0:X}}}' -f $codePoint))
      }
    }
  }
  return $builder.ToString()
}

$utf8 = [System.Text.UTF8Encoding]::new($false)
$servedHtml = Convert-ToHtmlAscii $html
$servedScript = Convert-ToJavaScriptAscii $script
$indexBytes = [System.Text.Encoding]::ASCII.GetBytes($servedHtml + [char]0)
$styleBytes = [System.Text.Encoding]::ASCII.GetBytes($style + [char]0)
$scriptBytes = [System.Text.Encoding]::ASCII.GetBytes($servedScript + [char]0)
$logoBytes = [System.IO.File]::ReadAllBytes($logoPath)

$header = [System.Text.StringBuilder]::new()
[void]$header.AppendLine('#ifndef WEB_ASSETS_HPP')
[void]$header.AppendLine('#define WEB_ASSETS_HPP')
[void]$header.AppendLine()
[void]$header.AppendLine('#include <Arduino.h>')
[void]$header.AppendLine()
[void]$header.Append((Convert-ToCArray 'WEB_INDEX' $indexBytes))
[void]$header.AppendLine("constexpr size_t WEB_INDEX_LEN = $($indexBytes.Length - 1);")
[void]$header.AppendLine()
[void]$header.Append((Convert-ToCArray 'WEB_STYLE' $styleBytes))
[void]$header.AppendLine("constexpr size_t WEB_STYLE_LEN = $($styleBytes.Length - 1);")
[void]$header.AppendLine()
[void]$header.Append((Convert-ToCArray 'WEB_SCRIPT' $scriptBytes))
[void]$header.AppendLine("constexpr size_t WEB_SCRIPT_LEN = $($scriptBytes.Length - 1);")
[void]$header.AppendLine()
[void]$header.Append((Convert-ToCArray 'WEB_LOGO' $logoBytes))
[void]$header.AppendLine("constexpr size_t WEB_LOGO_LEN = $($logoBytes.Length);")
[void]$header.AppendLine()
[void]$header.AppendLine('#endif')
[System.IO.File]::WriteAllText($headerPath, $header.ToString(), [System.Text.UTF8Encoding]::new($false))

Write-Host "Assets web separados e compilados em web_assets.hpp."
Write-Host "Index: $($indexBytes.Length - 1) bytes | CSS: $($styleBytes.Length - 1) bytes | JS: $($scriptBytes.Length - 1) bytes | Logo: $($logoBytes.Length) bytes"
