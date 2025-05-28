mkdir QScintilla_build
mkdir QScintilla_release
cd QScintilla_build
qmake ../QScintilla/src/qscintilla.pro
mingw32-make %1

::Copy files to release folder
RD /S /Q ..\QScintilla_release

mkdir ..\QScintilla_release\lib
mkdir ..\QScintilla_release\translations

copy release\*.a   ..\QScintilla_release\lib
copy release\*.dll ..\QScintilla_release\lib
robocopy /s /e ..\QScintilla\src\Qsci     ..\QScintilla_release\include\Qsci
copy           ..\QScintilla\src\*.qm     ..\QScintilla_release\translations\
robocopy /s /e ..\QScintilla\qsci         ..\QScintilla_release\qsci
robocopy /s /e ..\QScintilla\src\features ..\QScintilla_release\mkspecs\features
