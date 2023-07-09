cd vcbtool-win64
cp ..\release\vcbtool.exe .
cp ..\font_3x4.png .
cp ..\font_3x5.png .
cp ..\font_4x5.png .
cp ..\font_5x7.png .
cp ..\font_fixedsys.png .
cp ..\font_topaz.png .
cp ..\font_topaz_sans.png .
cp ..\fonts.json .
cp ..\stylesheet-dev.css .
windeployqt vcbtool.exe --no-quick-import --no-system-d3d-compiler --no-translations --no-virtualkeyboard --no-opengl-sw --no-svg

