@powershell -NoProfile -ExecutionPolicy Unrestricted "$s=[scriptblock]::create((gc \"%~f0\"|?{$_.readcount -gt 1})-join\"`n\");&$s" %*&goto:eof

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

$BOOST_VER_UNDERLINE = $BOOST_VER.Replace(".","_")
$ZLIB_VER_NUM = $ZLIB_VER.Replace(".","")
$LIBPNG_VER_NUM = $LIBPNG_VER.Replace(".","")
$LIBPNG_VER_NUM2 = $LIBPNG_VER_NUM.Substring(0,2)
$FREETYPE_VER_NUM = $FREETYPE_VER.Replace(".","")

$BASE_PATH = pwd
$LIBRARY_PATH = "$BASE_PATH\library"
$PATCH_PATH = "$BASE_PATH\bootstrap"

$MSBUILD = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe"
$VS_TOOLS_VER = ls "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\" -NAME  | Select-Object -Last 1
$NMAKE = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\$VS_TOOLS_VER\bin\Hostx86\x86\nmake.exe"

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

function noBuild($url,$name) {
  if (!(Test-Path "$name")) {
    if (!(Test-Path "$name.zip")) {
      Write-Host "** $name のデータを取得します。"
      Write-Host "$url"
      Invoke-WebRequest -Uri "$url" -OutFile "$name.zip"
    } else {
      Write-Host "** $name は既に取得済なので無視しました。"
      Write-Host ""
    }
    Write-Host "** $name を展開します。"
    Expand-Archive -Path "$name.zip" -DestinationPath "$name" -force

    Write-Host "** $name はビルド不要なのでビルドはスキップしました。"
    Write-Host ""
  } else {
    Write-Host "** $name は既に展開済なので無視しました。"
    Write-Host ""
  }
}

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

cd tmp

if (!(Test-Path "patch.zip")) {
  Write-Host "** patchコマンドのソースコードを取得します。"
  Write-Host "https://blogs.osdn.jp/2015/01/13/download/patch-2.5.9-7-bin.zip"
  Invoke-WebRequest -Uri "https://blogs.osdn.jp/2015/01/13/download/patch-2.5.9-7-bin.zip" -OutFile "patch.zip"
  Expand-Archive -Path "patch.zip"
  Write-Host ""
} else {
  Write-Host "** patchコマンドは既に取得済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "7z.zip")) {
  Write-Host "** 7zコマンドのソースコードを取得します。"
  Write-Host "https://ja.osdn.net/frs/redir.php?m=ymu&f=sevenzip%2F64455%2F7za920.zip"
  Invoke-WebRequest -Uri "https://ja.osdn.net/frs/redir.php?m=ymu&f=sevenzip%2F64455%2F7za920.zip" -OutFile "7z.zip"
  Expand-Archive -Path "7z.zip"
  Write-Host ""
} else {
  Write-Host "** 7zコマンドは既に取得済なので無視しました。"
  Write-Host ""
}

$PATCH = Resolve-Path ".\patch\bin\patch.exe"
$7Z = Resolve-Path ".\7z\7za.exe"

Write-Host "================================================================================="
Write-Host ""
Write-Host "* 依存ライブラリの取得・ビルドを実行します"
Write-Host ""

cd $LIBRARY_PATH

if (!(Test-Path "angelscript")) {
  if (!(Test-Path "angelscript.zip")) {
    Write-Host "** AngelScript のソースコードを取得します。"
    Write-Host "https://www.angelcode.com/angelscript/sdk/files/angelscript_$ANGELSCRIPT_VER.zip"
    Invoke-WebRequest -Uri "https://www.angelcode.com/angelscript/sdk/files/angelscript_$ANGELSCRIPT_VER.zip" -OutFile "angelscript.zip"
  } else {
    Write-Host "** AngelScript は既に取得済なので無視しました。"
    Write-Host ""
  }
  Write-Host "** AngelScript を展開します。"
  &$7Z x angelscript.zip -oangelscript >$null

  Write-Host "** AngelScript をビルドします。"
  cd angelscript\sdk\angelscript\projects\msvc2015
  &$PATCH angelscript.vcxproj "$PATCH_PATH\angelscript.patch"
  &$MSBUILD angelscript.vcxproj /p:Configuration=Release
  &$MSBUILD angelscript.vcxproj /p:Configuration=Debug
  cd "$LIBRARY_PATH"

  Write-Host ""
} else {
  Write-Host "** AngelScript は既にビルド済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "boost")) {
  if (!(Test-Path "boost.zip")) {
    Write-Host "** Boost のソースコードを取得します。"
    Write-Host "https://dl.bintray.com/boostorg/release/$BOOST_VER/source/boost_$BOOST_VER_UNDERLINE.zip"
    Invoke-WebRequest -Uri "https://dl.bintray.com/boostorg/release/$BOOST_VER/source/boost_$BOOST_VER_UNDERLINE.zip" -OutFile "boost.zip"
  } else {
    Write-Host "** Boost は既に取得済なので無視しました。"
    Write-Host ""
  }
  Write-Host "** Boost を展開します。"
  &$7Z x boost.zip >$null
  Rename-Item "boost_$BOOST_VER_UNDERLINE" "boost"

  Write-Host "** Boost をビルドします。"
  cd "boost"
  cmd /c "bootstrap.bat"
  cmd /c "b2 -j 4"
  cd "$LIBRARY_PATH"

  Write-Host ""
} else {
  Write-Host "** Boost は既にビルド済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "zlib")) {
  if (!(Test-Path "zlib.zip")) {
    Write-Host "** zlib のソースコードを取得します。"
    Write-Host "https://zlib.net/zlib$ZLIB_VER_NUM.zip"
    Invoke-WebRequest -Uri "https://zlib.net/zlib$ZLIB_VER_NUM.zip" -OutFile "zlib.zip"
  } else {
    Write-Host "** zlib は既に取得済なので無視しました。"
    Write-Host ""
  }

  Write-Host "** zlib を展開します。"
  &$7Z x zlib.zip >$null
  Rename-Item "zlib-$ZLIB_VER" "zlib"

  Write-Host "** zlib はビルド不要なのでビルドはスキップしました。"
  Write-Host ""
} else {
  Write-Host "** zlib は既に取得済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "libpng")) {
  if (!(Test-Path "libpng.zip")) {
    Write-Host "** libpng のソースコードを取得します。"
    Write-Host "http://ftp-osl.osuosl.org/pub/libpng/src/libpng$LIBPNG_VER_NUM2/lpng$LIBPNG_VER_NUM.zip"
    Invoke-WebRequest -Uri "http://ftp-osl.osuosl.org/pub/libpng/src/libpng$LIBPNG_VER_NUM2/lpng$LIBPNG_VER_NUM.zip" -OutFile "libpng.zip"
  } else {
    Write-Host "** libpng は既に取得済なので無視しました。"
    Write-Host ""
  }

  Write-Host "** libpng を展開します。"
  &$7Z x libpng.zip >$null
  Rename-Item "lpng$LIBPNG_VER_NUM" "libpng"

  Write-Host "** libpng をビルドします。"  
  cd "libpng\projects\vstudio"
  &$PATCH libpng\libpng.vcxproj         "$PATCH_PATH\libpng.patch"
  &$PATCH pnglibconf\pnglibconf.vcxproj "$PATCH_PATH\pnglibconf.patch"
  &$PATCH pngstest\pngstest.vcxproj     "$PATCH_PATH\pngstest.patch"
  &$PATCH pngtest\pngtest.vcxproj       "$PATCH_PATH\pngtest.patch"
  &$PATCH pngunknown\pngunknown.vcxproj "$PATCH_PATH\pngunknown.patch"
  &$PATCH pngvlaid\pngvlaid.vcxproj     "$PATCH_PATH\pngvalid.patch"
  &$PATCH zlib\zlib.vcxproj             "$PATCH_PATH\zlib.patch"
  &$PATCH zlib.props                    "$PATCH_PATH\zlib.props.patch"
  # なんか失敗するけど多分これ正常
  &$MSBUILD vstudio.sln /p:Configuration=Release
  cd "$LIBRARY_PATH"

  Write-Host ""
} else {
  Write-Host "** libpng は既にビルド済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "libjpeg")) {
  if (!(Test-Path "libjpeg.zip")) {
    Write-Host "** libjpeg のソースコードを取得します。"
    Write-Host "https://www.ijg.org/files/jpegsr$LIBJPEG_VER.zip"
    Invoke-WebRequest -Uri "https://www.ijg.org/files/jpegsr$LIBJPEG_VER.zip" -OutFile "libjpeg.zip"
  } else {
    Write-Host "** libjpeg は既に取得済なので無視しました。"
    Write-Host ""
  }

  Write-Host "** libjpeg を展開します。"
  &$7Z x libjpeg.zip >$null
  Rename-Item "jpeg-$LIBJPEG_VER" "libjpeg"

  Write-Host "** libjpeg をビルドします。"
  cd "libjpeg"
  &$NMAKE /f makefile.vs setup-v15
  &$PATCH --force jpeg.vcxproj         "$PATCH_PATH\libjpeg.patch"
  &$MSBUILD jpeg.sln /p:Configuration=Release
  cd "$LIBRARY_PATH"

  Write-Host ""
} else {
  Write-Host "** libjpeg は既にビルド済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "libogg")) {
  if (!(Test-Path "libogg.zip")) {
    Write-Host "** libogg のソースコードを取得します。"
    Write-Host "http://downloads.xiph.org/releases/ogg/libogg-$LIBOGG_VER.zip"
    Invoke-WebRequest -Uri "http://downloads.xiph.org/releases/ogg/libogg-$LIBOGG_VER.zip" -OutFile "libogg.zip"
  } else {
    Write-Host "** libogg は既に取得済なので無視しました。"
    Write-Host ""
  }
  Write-Host "** libogg を展開します。"
  &$7Z x libogg.zip >$null
  Rename-Item "libogg-$LIBOGG_VER" "libogg"

  Write-Host "** libogg をビルドします。"  
  cd "libogg\win32\VS2015"
  &$PATCH --force libogg_static.vcxproj "$PATCH_PATH\libogg.patch"
  &$MSBUILD libogg_static.sln /p:Configuration=Release
  cd "$LIBRARY_PATH"

  Write-Host ""
} else {
  Write-Host "** libogg は既にビルド済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "libvorbis")) {
  if (!(Test-Path "libvorbis.zip")) {
    Write-Host "** libvorbis のソースコードを取得します。"
    Write-Host "http://downloads.xiph.org/releases/vorbis/libvorbis-$LIBVORBIS_VER.zip"
    Invoke-WebRequest -Uri "http://downloads.xiph.org/releases/vorbis/libvorbis-$LIBVORBIS_VER.zip" -OutFile "libvorbis.zip"
  } else {
    Write-Host "** libvorbis は既に取得済なので無視しました。"
    Write-Host ""
  }
  Write-Host "** libvorbis を展開します。"
  &$7Z x libvorbis.zip >$null
  Rename-Item "libvorbis-$LIBVORBIS_VER" "libvorbis"

  Write-Host "** libvorbis をビルドします。"  
  cd "libvorbis\win32\VS2010"
  &$PATCH libvorbis\libvorbis_static.vcxproj         "$PATCH_PATH\libvorbis.patch"
  &$PATCH libvorbisfile\libvorbisfile_static.vcxproj "$PATCH_PATH\libvorbisfile.patch"
  &$PATCH vorbisdec\vorbisdec_static.vcxproj         "$PATCH_PATH\vorbisdec.patch"
  &$PATCH vorbisenc\vorbisenc_static.vcxproj         "$PATCH_PATH\vorbisenc.patch"
  &$PATCH libogg.props                               "$PATCH_PATH\libogg.props.patch"
  &$MSBUILD vorbis_static.sln /p:Configuration=Release
  cd "$LIBRARY_PATH"

  Write-Host ""
} else {
  Write-Host "** libogg は既にビルド済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "freetype")) {
  if (!(Test-Path "freetype.zip")) {
    Write-Host "** freetype のソースコードを取得します。"
    Write-Host "https://download.savannah.gnu.org/releases/freetype/ft$FREETYPE_VER_NUM.zip"
    Invoke-WebRequest -Uri "https://download.savannah.gnu.org/releases/freetype/ft$FREETYPE_VER_NUM.zip" -OutFile "freetype.zip"
  } else {
    Write-Host "** freetype は既に取得済なので無視しました。"
    Write-Host ""
  }
  Write-Host "** freetype を展開します。"
  &$7Z x freetype.zip >$null
  Rename-Item "freetype-$FREETYPE_VER" "freetype"

  Write-Host "** freetype をビルドします。"  
  cd "freetype\builds\windows\vc2010"
  &$PATCH freetype.vcxproj "$PATCH_PATH\freetype.patch"
  &$MSBUILD freetype.sln /p:Configuration=Release
  &$MSBUILD freetype.sln /p:Configuration=Debug
  cd "$LIBRARY_PATH"

  Write-Host ""
} else {
  Write-Host "** freetype は既にビルド済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "dxlib")) {
  if (!(Test-Path "dxlib.zip")) {
    Write-Host "** DxLib のソースコードを取得します。"
    Write-Host "http://dxlib.o.oo7.jp/DxLib/DxLib_VC3_19d.zip"
    Invoke-WebRequest -Uri "http://dxlib.o.oo7.jp/DxLib/DxLib_VC3_19d.zip" -OutFile "dxlib.zip"
  } else {
    Write-Host "** DxLib は既に取得済なので無視しました。"
    Write-Host ""
  }

  Write-Host "** DxLib を展開します。"
  &$7Z x dxlib.zip >$null
  Rename-Item "DxLib_VC" "dxlib"

  Write-Host "** DxLib をリネームします。"
  cd "dxlib"
  Rename-Item "プロジェクトに追加すべきファイル_VC用" "include"
  cd "$LIBRARY_PATH"

  Write-Host ""
} else {
  Write-Host "** DxLib は既にビルド済なので無視しました。"
  Write-Host ""
}


if (!(Test-Path "spdlog")) {
  if (!(Test-Path "spdlog.zip")) {
    Write-Host "** spdlog のソースコードを取得します。"
    Write-Host "https://github.com/gabime/spdlog/archive/v$SPDLOG_VER.zip"
    Invoke-WebRequest -Uri "https://github.com/gabime/spdlog/archive/v$SPDLOG_VER.zip" -OutFile "spdlog.zip"
  } else {
    Write-Host "** spdlog は既に取得済なので無視しました。"
    Write-Host ""
  }

  Write-Host "** spdlog を展開します。"
  &$7Z x spdlog.zip >$null
  Rename-Item "spdlog-$SPDLOG_VER" "spdlog"
  cd "$LIBRARY_PATH"

  Write-Host "** spdlog はビルド不要なのでビルドはスキップしました。"
  Write-Host ""
} else {
  Write-Host "** spdlog は既に取得済なので無視しました。"
  Write-Host ""
}

if (!(Test-Path "fmt")) {
  if (!(Test-Path "fmt.zip")) {
    Write-Host "** fmt のソースコードを取得します。"
    Write-Host "https://github.com/fmtlib/fmt/releases/download/$FMT_VER/fmt-$FMT_VER.zip"
    Invoke-WebRequest -Uri "https://github.com/fmtlib/fmt/releases/download/$FMT_VER/fmt-$FMT_VER.zip" -OutFile "fmt.zip"
  } else {
    Write-Host "** fmt は既に取得済なので無視しました。"
    Write-Host ""
  }
  Write-Host "** fmt を展開します。"
  &$7Z x fmt.zip >$null
  Rename-Item "fmt-$FMT_VER" "fmt"

  Write-Host "** fmt はビルド不要なのでビルドはスキップしました。"
  Write-Host ""
} else {
  Write-Host "** fmt は既に取得済なので無視しました。"
  Write-Host ""
}

noBuild "http://us.un4seen.com/files/bass24.zip" "bass24"
noBuild "http://us.un4seen.com/files/z/0/bass_fx24.zip" "bass24_fx"
noBuild "http://us.un4seen.com/files/bassmix24.zip" "bass24_mix"

noBuild "https://github.com/mayah/tinytoml/archive/master.zip" "tinytoml"
noBuild "https://github.com/g-truc/glm/releases/download/$GLM_VER/glm-$GLM_VER.zip" "glm"

# Wacom機能が廃止されるまで臨時措置
noBuild "https://github.com/denisidoro/xWacom/archive/master.zip" "wacom"

pause