# バージョン指定
$ANGELSCRIPT_VER = "2.32.0"
$BOOST_VER = "1.68.0"
$ZLIB_VER = "1.2.11"
$LIBPNG_VER = "1.6.34"
$LIBJPEG_VER = "9c"
$LIBOGG_VER = "1.3.3"
$LIBVORBIS_VER = "1.3.6"
$FMT_VER = "4.1.0"
$SPDLOG_VER = "0.17.0"
$GLM_VER = "0.9.9.3"
$FREETYPE_VER = "2.9.1"

# バージョン => URLの形式に変換
$BOOST_VER_UNDERLINE = $BOOST_VER.Replace(".","_")
$ZLIB_VER_NUM = $ZLIB_VER.Replace(".","")
$LIBPNG_VER_NUM = $LIBPNG_VER.Replace(".","")
$LIBPNG_VER_NUM2 = $LIBPNG_VER_NUM.Substring(0,2)
$FREETYPE_VER_NUM = $FREETYPE_VER.Replace(".","")

$BASE_PATH = Get-Location
$LIBRARY_PATH = "$BASE_PATH\library"
$PATCH_PATH = "$BASE_PATH\bootstrap"

$EDITION = Get-ChildItem "C:\Program Files (x86)\Microsoft Visual Studio\2017\" -NAME  | Select-Object -Last 1

$MSBUILD = "C:\Program Files (x86)\Microsoft Visual Studio\2017\$EDITION\MSBuild\15.0\Bin\MSBuild.exe"
if (Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2017\$EDITION\VC\Tools\MSVC\") {
  $VS_TOOLS_VER = Get-ChildItem "C:\Program Files (x86)\Microsoft Visual Studio\2017\$EDITION\VC\Tools\MSVC\" -NAME  | Select-Object -Last 1
  $NMAKE = "C:\Program Files (x86)\Microsoft Visual Studio\2017\$EDITION\VC\Tools\MSVC\$VS_TOOLS_VER\bin\Hostx86\x86\nmake.exe"
}

[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12

Write-Host '================================================================================'
Write-Host '                                                                                '
Write-Host ' .d8888b.                                             888      d8b              '
Write-Host 'd88P  Y88b                                            888      Y8P              '
Write-Host 'Y88b.                                                 888                       '
Write-Host ' "Y888b.    .d88b.   8888b.  888  888 888d888 .d8888b 88888b.  888 88888b.      '
Write-Host '    "Y88b. d8P  Y8b     "88b 888  888 888P"  d88P"    888 "88b 888 888 "88b     '
Write-Host '      "888 88888888 .d888888 888  888 888    888      888  888 888 888  888     '
Write-Host 'Y88b  d88P Y8b.     888  888 Y88b 888 888    Y88b.    888  888 888 888  888     '
Write-Host ' "Y8888P"   "Y8888  "Y888888  "Y88888 888     "Y8888P 888  888 888 888  888     '
Write-Host '                                                                                '
Write-Host '   888888b.                     888    .d8888b.  888                            '
Write-Host '   888  "88b                    888   d88P  Y88b 888                            '
Write-Host '   888  .88P                    888   Y88b.      888                            '
Write-Host '   8888888K.   .d88b.   .d88b.  888888 "Y888b.   888888 888d888 8888b.  88888b. '
Write-Host '   888  "Y88b d88""88b d88""88b 888       "Y88b. 888    888P"      "88b 888 "88b'
Write-Host '   888    888 888  888 888  888 888         "888 888    888    .d888888 888  888'
Write-Host '   888   d88P Y88..88P Y88..88P Y88b. Y88b  d88P Y88b.  888    888  888 888 d88P'
Write-Host '   8888888P"   "Y88P"   "Y88P"   "Y888 "Y8888P"   "Y888 888    "Y888888 88888P" '
Write-Host '                                                                        888     '
Write-Host '                                                                        888     '
Write-Host '======================================================================= 888 ===='
Write-Host ''
Write-Host 'Seaurchin BootStrapではSeaurchinの開発環境を構築します。'
Write-Host ''

if (!(Test-Path "C:\Program Files (x86)\Microsoft SDKs\Windows Kits\10\ExtensionSDKs\Microsoft.Midi.GmDls\10.0.17763.0")) {
  Write-Host "Windows SDK 10.0.17663.0がインストールされてない為、Bootstrapを続ける事が出来ません。終了します。"
  Write-Host "Windows SDK 10.0.17763.0をインストールしてから再度実行してください。"
  Write-Host ""
  if($Args[0] -ne "quiet") {
    Read-Host "終了するには Enter キーを押してください"
  }
  exit
}
if (!(Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2017\$EDITION\VC\Tools\MSVC\$VS_TOOLS_VER\lib\spectre")){
  Write-Host "注意： Spectre用ライブラリがインストールされていません。"
  Write-Host "       Bootstrapは動作しますが、Seaurchinのビルドが失敗する原因になります。"
  Write-Host "       VC++ 2017 version XX.X vXX.XX Libs for Spectre (x86 and x64)をインストールしてください。"
  Write-Host ""
}
if($Args[0] -ne "quiet") {
  Read-Host "続行するには Enter キーを押してください"
}

Write-Host "================================================================================="
Write-Host ""
Write-Host "* ライブラリ展開先フォルダと一時フォルダを生成します。"
Write-Host ""

New-Item library\ -ItemType Directory >$null 2>&1
New-Item tmp\ -ItemType Directory >$null 2>&1

Write-Host "================================================================================="
Write-Host ""
Write-Host "* 環境構築に必要なコマンドを準備します。"
Write-Host ""

function getBin($url,$name) {
  if (!(Test-Path "$name.zip")) {
    Write-Host "** $name コマンドのバイナリを取得します。"
    Write-Host "$url"
    Invoke-WebRequest -Uri "$url" -OutFile "$name.zip"
    Expand-Archive -Path "$name.zip"
  } else {
    Write-Host "** $name コマンドは既に取得済なので無視しました。"
  }
  Write-Host ""
}

Set-Location "tmp"
getBin "https://blogs.osdn.jp/2015/01/13/download/patch-2.5.9-7-bin.zip" "patch"
getBin "https://ja.osdn.net/frs/redir.php?m=ymu&f=sevenzip%2F64455%2F7za920.zip" "7z"
getBin "http://ftp.vector.co.jp/52/68/2195/nkfwin.zip" "nkf"

$PATCH = Resolve-Path ".\patch\bin\patch.exe"
$7Z = Resolve-Path ".\7z\7za.exe"
$NKF = Resolve-Path ".\nkf\vc2005\win32(98,Me,NT,2000,XP,Vista,7)ISO-2022-JP\nkf.exe"

function dlSource($url,$name,$path) {
  if (!(Test-Path "$name")) {
    if (!(Test-Path "$name.zip")) {
      Write-Host "** $name のデータを取得します。"
      Write-Host "$url"
      Invoke-WebRequest -Uri "$url" -OutFile "$name.zip"
    } else {
      Write-Host "** $name は既に取得済なので無視しました。"
    }
    Write-Host "** $name を展開します。"

    if("$path" -eq "") {
      &$7Z x "$name.zip" >$null
    } else {
      &$7Z x "$name.zip" -o"$path" >$null
    }
  } else {
    Write-Host "** $name は既に展開済なので無視しました。"
  }
}

function dlSourceRename($url,$name,$from) {
  if (!(Test-Path "$name")) {
    dlSource "$url" "$name"
    Write-Host "** $name をリネームします。"
    Rename-Item "$from" "$name"
  } else {
    Write-Host "** $name は既に展開済なので無視しました。"
  }
}

# ========  UTF-8/LF -> S-JIS/CRLF  ========
Set-Location "$BASE_PATH\bootstrap"
foreach($a in Get-ChildItem){
  (Get-Content $a.name) -join "`r`n" | set-content $a.name
}
Set-Location "$BASE_PATH\Seaurchin"
&$NKF -sc --overwrite *.cpp
&$NKF -sc --overwrite *.h
Set-Location $LIBRARY_PATH

Write-Host "================================================================================="
Write-Host ""
Write-Host "* 依存ライブラリの取得・ビルドを実行します"
Write-Host ""

# ========  AngelScript  ========
if (!(Test-Path "angelscript")) {
  dlSourceRename "https://www.angelcode.com/angelscript/sdk/files/angelscript_$ANGELSCRIPT_VER.zip" "angelscript" "sdk"
  Write-Host "** angelscript をビルドします。"
  Set-Location "angelscript\angelscript\projects\msvc2015"
  &$PATCH "angelscript.vcxproj" "$PATCH_PATH\angelscript.patch"
  &$MSBUILD "angelscript.vcxproj" /p:Configuration=Release
  &$MSBUILD "angelscript.vcxproj" /p:Configuration=Debug
  Set-Location "$LIBRARY_PATH"
}

# ========  libpng  ========
if (!(Test-Path "libpng")) {
  dlSourceRename "https://zlib.net/zlib$ZLIB_VER_NUM.zip" "zlib" "zlib-$ZLIB_VER"
  dlSourceRename "http://ftp-osl.osuosl.org/pub/libpng/src/libpng$LIBPNG_VER_NUM2/lpng$LIBPNG_VER_NUM.zip" "libpng" "lpng$LIBPNG_VER_NUM"
  Write-Host "** libpng をビルドします。"  
  Set-Location "libpng\projects\vstudio"
  &$PATCH libpng\libpng.vcxproj         "$PATCH_PATH\libpng.patch"
  &$PATCH pnglibconf\pnglibconf.vcxproj "$PATCH_PATH\pnglibconf.patch"
  &$PATCH pngstest\pngstest.vcxproj     "$PATCH_PATH\pngstest.patch"
  &$PATCH pngtest\pngtest.vcxproj       "$PATCH_PATH\pngtest.patch"
  &$PATCH pngunknown\pngunknown.vcxproj "$PATCH_PATH\pngunknown.patch"
  &$PATCH pngvlaid\pngvlaid.vcxproj     "$PATCH_PATH\pngvalid.patch"
  &$PATCH zlib\zlib.vcxproj             "$PATCH_PATH\zlib.patch"
  &$PATCH zlib.props                    "$PATCH_PATH\zlib.props.patch"
  &$MSBUILD vstudio.sln /p:Configuration=Release # なんか失敗するけど多分正常
  Set-Location "$LIBRARY_PATH"
}

# ========  libjpeg  ========
if (!(Test-Path "libjpeg")) {
  dlSourceRename "https://www.ijg.org/files/jpegsr$LIBJPEG_VER.zip" "libjpeg" "jpeg-$LIBJPEG_VER"
  Write-Host "** libjpeg をビルドします。"
  Set-Location "libjpeg"
  &$NMAKE /f makefile.vs setup-v15
  &$PATCH --force jpeg.vcxproj         "$PATCH_PATH\libjpeg.patch"
  &$MSBUILD jpeg.sln /p:Configuration=Release
  Set-Location "$LIBRARY_PATH"
}

# ========  libogg  ========
if (!(Test-Path "libogg")) {
  dlSourceRename "http://downloads.xiph.org/releases/ogg/libogg-$LIBOGG_VER.zip" "libogg" "libogg-$LIBOGG_VER"
  Write-Host "** libogg をビルドします。"  
  Set-Location "libogg\win32\VS2015"
  &$PATCH --force libogg_static.vcxproj "$PATCH_PATH\libogg.patch"
  &$MSBUILD libogg_static.sln /p:Configuration=Release
  Set-Location "$LIBRARY_PATH"
}

# ========  libvorbis  ========
if (!(Test-Path "libvorbis")) {
  dlSourceRename "http://downloads.xiph.org/releases/vorbis/libvorbis-$LIBVORBIS_VER.zip" "libvorbis" "libvorbis-$LIBVORBIS_VER"
  Write-Host "** libvorbis をビルドします。"  
  Set-Location "libvorbis\win32\VS2010"
  &$PATCH libvorbis\libvorbis_static.vcxproj         "$PATCH_PATH\libvorbis.patch"
  &$PATCH libvorbisfile\libvorbisfile_static.vcxproj "$PATCH_PATH\libvorbisfile.patch"
  &$PATCH vorbisdec\vorbisdec_static.vcxproj         "$PATCH_PATH\vorbisdec.patch"
  &$PATCH vorbisenc\vorbisenc_static.vcxproj         "$PATCH_PATH\vorbisenc.patch"
  &$PATCH libogg.props                               "$PATCH_PATH\libogg.props.patch"
  &$MSBUILD vorbis_static.sln /p:Configuration=Release
  Set-Location "$LIBRARY_PATH"
}

# ========  freetype  ========
if (!(Test-Path "freetype")) {
  dlSourceRename "https://download.savannah.gnu.org/releases/freetype/ft$FREETYPE_VER_NUM.zip" "freetype" "freetype-$FREETYPE_VER"
  Write-Host "** freetype をビルドします。"  
  Set-Location "freetype\builds\windows\vc2010"
  &$PATCH freetype.vcxproj "$PATCH_PATH\freetype.patch"
  &$MSBUILD freetype.sln /p:Configuration="Release Static"
  &$MSBUILD freetype.sln /p:Configuration="Debug Static"
  Set-Location "$LIBRARY_PATH"
}

dlSource "https://rbjhjw.dm.files.1drv.com/y4mrerPw3Zd1tXLWrxQ2ubuFvPJkaniGX82gfkIIYnYJZNIepUOtHiMIAirmikXEyAppmwN3_V7UbdevmEpU5kdR20PEflO1RICwjrUuZcfB0CcarCHeQrNw6ex9qQyyDTJVfCVB4vixKXpucTsma2Q-N2C7tF7_YuN0vjfmoKs8aO-TNk3p-6xoVWGSUpTblwJshqDZPxeZPQlPdYdgf_I6Q/dxlib.7z?download&psid=1" "dxlib" "."
dlSourceRename "https://github.com/gabime/spdlog/archive/v$SPDLOG_VER.zip" "spdlog" "spdlog-$SPDLOG_VER"
dlSourceRename "https://github.com/fmtlib/fmt/releases/download/$FMT_VER/fmt-$FMT_VER.zip" "fmt" "fmt-$FMT_VER"

dlSource "https://sljhjw.dm.files.1drv.com/y4m_ttflbSwtU5qSJt5J47yBqYztzuSzEdEnXb0Ce2BaO0wrf_hfA1pMcJJbOnv-_PtTzKc5OjE_iaCv0pHOqKqLwZIN0Qx0j6JiBNbWiv62IJPv4w5fSqBbr-106Biak3UE2qxNIwY2E0xcMJYwDA5oTsJlDWzeb8RhPzIvNqbEvG_ngusUWEN_KFxWAcK8T32/boost.7z?download&psid=1" "boost" "."
dlSource "http://us.un4seen.com/files/bass24.zip" "bass24" "bass24"
dlSource "http://us.un4seen.com/files/z/0/bass_fx24.zip" "bass24_fx" "bass24_fx"
dlSource "http://us.un4seen.com/files/bassmix24.zip" "bass24_mix" "bass24_mix"

dlSourceRename "https://github.com/mayah/tinytoml/archive/master.zip" "tinytoml" "tinytoml-master"
dlSource "https://github.com/g-truc/glm/releases/download/$GLM_VER/glm-$GLM_VER.zip" "glm"

Write-Host "* ビルドが完了しました。Seaurchin.slnをダブルクリックして開いてください。"

if($Args[0] -ne "quiet") {
  pause
}