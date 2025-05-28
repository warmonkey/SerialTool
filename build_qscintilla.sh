#!/bin/bash
set -euo pipefail

# Usage: ./build_qscintilla.sh -j8

mkdir -p QScintilla_build QScintilla_release

# Build
pushd QScintilla_build
qmake ../QScintilla/src/qscintilla.pro
make $1
popd

# Install
rm -rf QScintilla_release

mkdir -p \
  QScintilla_release/lib \
  QScintilla_release/include \
  QScintilla_release/qsci \
  QScintilla_release/translations \
  QScintilla_release/mkspecs/features

cp QScintilla_build/release/*.a   QScintilla_release/lib/
cp QScintilla_build/release/*.dll QScintilla_release/lib/
cp -r QScintilla/src/Qsci     QScintilla_release/include/
cp -r QScintilla/qsci         QScintilla_release/
cp    QScintilla/src/*.qm     QScintilla_release/translations/
cp -r QScintilla/src/features QScintilla_release/mkspecs/
