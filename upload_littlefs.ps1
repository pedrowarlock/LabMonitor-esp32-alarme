param(
  [string]$Port = "COM12"
)

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$mkLittleFs = "$env:LOCALAPPDATA\Arduino15\packages\esp32\tools\mklittlefs\4.0.2-db0513a\mklittlefs.exe"
$esptool = "$env:LOCALAPPDATA\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe"
$source = Join-Path $root "web"
$data = Join-Path $root "data"
$image = Join-Path $root "littlefs.bin"

if (-not (Test-Path $source)) {
  throw "Pasta web nao encontrada: $source"
}
if (-not (Test-Path $mkLittleFs)) {
  throw "mklittlefs nao encontrado: $mkLittleFs"
}
if (-not (Test-Path $esptool)) {
  throw "esptool nao encontrado: $esptool"
}

New-Item -ItemType Directory -Force -Path $data | Out-Null
Copy-Item (Join-Path $source "*") $data -Recurse -Force

& $mkLittleFs -c $data -b 4096 -p 256 -s 0xF0000 $image
if ($LASTEXITCODE -ne 0) {
  throw "Falha ao criar a imagem LittleFS."
}

& $esptool --chip esp32 --port $Port --baud 921600 write-flash 0x310000 $image
if ($LASTEXITCODE -ne 0) {
  throw "Falha ao gravar o LittleFS no ESP32."
}

Write-Host "LittleFS enviado com sucesso pela porta $Port."
