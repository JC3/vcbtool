cd vcbtool-win64
cp ..\release\vcbtool.exe .
cp ..\font_3x5.png .
cp ..\font_4x5.png .
cp ..\font_5x7.png .
windeployqt vcbtool.exe --no-quick-import --no-system-d3d-compiler --no-translations --no-virtualkeyboard --no-opengl-sw --no-svg

