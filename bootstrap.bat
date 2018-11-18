@powershell -NoProfile -ExecutionPolicy Unrestricted "$s=[scriptblock]::create((gc \"%~f0\"|?{$_.readcount -gt 1})-join\"`n\");&$s" %*&goto:eof

$ANGELSCRIPT_VER = "2.32.0"
$BOOST_VER = "1.68.0"
$ZLIB_VER = "1.2.11"
$LIBPNG_VER = "1.6.34"
$FMT_VER = "4.1.0"
$SPDLOG_VER = "0.17.0"
$GLM_VER = "0.9.9.3"
$FREETYPE_VER = "2.9.1"

$BOOST_VER_UNDERLINE = $BOOST_VER.Replace(".","_")
$ZLIB_VER_NUM = $ZLIB_VER.Replace(".","")
$LIBPNG_VER_NUM = $LIBPNG_VER.Replace(".","")
$LIBPNG_VER_NUM2 = $LIBPNG_VER_NUM.Substring(0,2)

$MSBUILD = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe"
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
Write-Host "Seaurchin BootStrapではSeaurchinの開発環境を自動的に構築をします。"
Read-Host '続行するには Enter キーを押してください'

Write-Host '================================================================================='
Write-Host ''
Write-Host "* ライブラリ展開先フォルダと一時フォルダを生成します。"
Write-Host ''

New-Item library\ -ItemType Directory >$null 2>&1
New-Item tmp\ -ItemType Directory >$null 2>&1

Write-Host '================================================================================='
Write-Host ''
Write-Host "* 環境構築に必要なコマンドを準備します。"
Write-Host ''

if (!(Test-Path "tmp\patch.zip")) {
  Write-Host "** patchコマンドのソースコードを取得します。"
  Write-Host "https://blogs.osdn.jp/2015/01/13/download/patch-2.5.9-7-bin.zip"
  Invoke-WebRequest -Uri "https://blogs.osdn.jp/2015/01/13/download/patch-2.5.9-7-bin.zip" -OutFile "tmp\patch.zip"
  Expand-Archive -Path "tmp/patch.zip" -DestinationPath "tmp"
  Write-Host ""
} else {
  Write-Host "** patchコマンドは既に取得済なので無視しました。"
  Write-Host ""
}

Write-Host "================================================================================="
Write-Host ""
Write-Host "* 依存ライブラリの取得・ビルドを実行します"
Write-Host ""

if (!(Test-Path "library\angelscript")) {
  if (!(Test-Path "library\angelscript.zip")) {
    Write-Host "** AngelScript のソースコードを取得します。"
    Write-Host "https://www.angelcode.com/angelscript/sdk/files/angelscript_$ANGELSCRIPT_VER.zip"
    Invoke-WebRequest -Uri "https://www.angelcode.com/angelscript/sdk/files/angelscript_$ANGELSCRIPT_VER.zip" -OutFile "library\angelscript.zip"
  } else {
    Write-Host "** AngelScript は既に取得済なので無視しました。"
    Write-Host ""
  }
  Write-Host "** AngelScript を展開します。"
  Expand-Archive -Path "library\angelscript.zip" -DestinationPath "library\angelscript" -force

  Write-Host "** AngelScript をビルドします。"
  .\tmp\bin\patch.exe library\angelscript\sdk\angelscript\projects\msvc2015\angelscript.vcxproj bootstrap\angelscript.patch
  cd library\angelscript\sdk\angelscript\projects\msvc2015
  &$MSBUILD angelscript.vcxproj /p:Configuration=Release
  cd ..\..\..\..\..\..\

  Write-Host ""
} else {
  Write-Host "** AngelScript は既にビルド済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "library\boost")) {
  if (!(Test-Path "library\boost.zip")) {
    Write-Host "** Boost のソースコードを取得します。"
    Write-Host "https://dl.bintray.com/boostorg/release/$BOOST_VER/source/boost_$BOOST_VER_UNDERLINE.zip"
    Invoke-WebRequest -Uri "https://dl.bintray.com/boostorg/release/$BOOST_VER/source/boost_$BOOST_VER_UNDERLINE.zip" -OutFile "library\boost.zip"
  } else {
    Write-Host "** Boost は既に取得済なので無視しました。"
    Write-Host ""
  }
  Write-Host "** Boost を展開します。"
  Expand-Archive -Path "library\boost.zip" -DestinationPath "library\boost" -force

  Write-Host "** Boost をビルドします。"
  cd "library\boost\boost_$BOOST_VER_UNDERLINE"
  cmd /c "bootstrap.bat"
  cmd /c "b2 -j 4"
  cd "..\..\..\"

  Write-Host ""
} else {
  Write-Host "** Boost は既にビルド済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "library\zlib")) {
  if (!(Test-Path "library\zlib.zip")) {
    Write-Host "** zlib のソースコードを取得します。"
    Write-Host "https://zlib.net/zlib$ZLIB_VER_NUM.zip"
    Invoke-WebRequest -Uri "https://zlib.net/zlib$ZLIB_VER_NUM.zip" -OutFile "library\zlib.zip"
  } else {
    Write-Host "** zlib は既に取得済なので無視しました。"
    Write-Host ""
  }
  Write-Host "** zlib を展開します。"
  Expand-Archive -Path "library\zlib.zip" -DestinationPath "library\zlib" -force

  Write-Host "** zlib はヘッダオンリーなのでビルドはスキップしました。"
  Write-Host ""
} else {
  Write-Host "** zlib は既に展開済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "library\libpng")) {
  if (!(Test-Path "library\libpng.zip")) {
    Write-Host "** libpng のソースコードを取得します。"
    Write-Host "http://ftp-osl.osuosl.org/pub/libpng/src/libpng$LIBPNG_VER_NUM2/lpng$LIBPNG_VER_NUM.zip"
    Invoke-WebRequest -Uri "http://ftp-osl.osuosl.org/pub/libpng/src/libpng$LIBPNG_VER_NUM2/lpng$LIBPNG_VER_NUM.zip" -OutFile "library\libpng.zip"
  } else {
    Write-Host "** libpng は既にビルド済なので無視しました。"
    Write-Host ""
  }
  Write-Host "** libpng を展開します。"
  Expand-Archive -Path "library\libpng.zip" -DestinationPath "library\libpng" -force

  Write-Host "** libpng をビルドします。"
  
  .\tmp\bin\patch.exe library\libpng\lpng1634\projects\vstudio\libpng\libpng.vcxproj bootstrap\libpng.patch
  .\tmp\bin\patch.exe library\libpng\lpng1634\projects\vstudio\pnglibconf\pnglibconf.vcxproj bootstrap\pnglibconf.patch
  .\tmp\bin\patch.exe library\libpng\lpng1634\projects\vstudio\pngstest\pngstest.vcxproj bootstrap\pngstest.patch
  .\tmp\bin\patch.exe library\libpng\lpng1634\projects\vstudio\pngtest\pngtest.vcxproj bootstrap\pngtest.patch
  .\tmp\bin\patch.exe library\libpng\lpng1634\projects\vstudio\pngunknown\pngunknown.vcxproj bootstrap\pngunknown.patch
  .\tmp\bin\patch.exe library\libpng\lpng1634\projects\vstudio\pngvlaid\pngvlaid.vcxproj bootstrap\pngvalid.patch
  .\tmp\bin\patch.exe library\libpng\lpng1634\projects\vstudio\zlib\zlib.vcxproj bootstrap\zlib.patch
  .\tmp\bin\patch.exe library\libpng\lpng1634\projects\vstudio\zlib.props bootstrap\zlib.props.patch

  cd "library\libpng\lpng1634\projects\vstudio"
  &$MSBUILD vstudio.sln /p:Configuration=Release
  cd "..\..\..\..\"

  Write-Host ""
} else {
  Write-Host "** libpng は既に展開済なので無視しました。"
  Write-Host ""
}



#download "https://github.com/mayah/tinytoml/archive/master.zip" "tinytoml"
#download "https://github.com/fmtlib/fmt/releases/download/$FMT_VER/fmt-$FMT_VER.zip" "fmt"
#download "https://github.com/gabime/spdlog/archive/v$SPDLOG_VER.zip" "spdlog"
#download "https://github.com/g-truc/glm/releases/download/$GLM_VER/glm-$GLM_VER.zip" "glm"
#download "http://dxlib.o.oo7.jp/DxLib/DxLib_VC3_19d.zip" "dxlib"
#download "https://github.com/ubawurinna/freetype-windows-binaries/releases/download/v$FREETYPE_VER/freetype-$FREETYPE_VER.zip" "freetype"
#download "http://us.un4seen.com/files/bass24.zip" "base24"
#download "http://us.un4seen.com/files/z/0/bass_fx24.zip" "base24_fx"
#download "http://us.un4seen.com/files/bassmix24.zip" "base24_mix"

pause