
How to build fro a Visual Studio command prompt
cl  -I ..\ -I ..\core  -I ..\svg /EHsc <filename>.cpp

And that's it

cl  -I ..\ -I ..\core  -I ..\svg /EHsc xmlpull.cpp

svgimage
cl  /EHsc  /Zc:__cplusplus /std:c++14 /MT  -I..\..\ -I..\..\app -I ..\..\svg   svgimage.cpp blend2d.lib  /link /LIBPATH:"..\..\lib\Release"