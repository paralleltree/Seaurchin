# Seaurchin Bootstrap

## 概要

いろいろとややこしいSeaurchinの開発環境を一発で整えます。

## 要件

- Visual Studio 2017
  - Windows 10 SDK 10.0.17763.0
  - C++によるデスクトップ開発
  - VC++ 2017 version 15.9 v14.16 Libs for Spectre (x86 and x64)
  
## 使い方

1. bootstrap.batをダブルクリックして開く
2. Enterを押下
3. めっちゃ待つ
4. 完了したらEnterを押下

bootstrapが完了したらSeaurchin.slnが使えるようになります。

## 注意事項

- Seaurchin Bootstrapは実験版です。何かあっても責任は取れません。
- Seaurchin 0.45.0β 現在、Dataフォルダを除くフォルダは自動生成されません。そのため、リリース公開されている物から持ってくる必要があります。
- 初回ビルド完了後はdllが存在しない為Seaurchinは正常起動しません。依存しているdllをSeaurchin.exeの存在するディレクトリに入れる必要があります。全てのdllはlibraryディレクトリ内のどこかにあります。

## 備考

- 環境構築をやり直したい場合 ... libraryフォルダを削除する
- 特定のライブラリを導入し直したい（バージョン変更等）場合 ... libraryフォルダ内の該当するフォルダとzipファイルを削除