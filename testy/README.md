
How to build fro a Visual Studio command prompt
cl  -I ..\ -I ..\core  -I ..\svg /EHsc <filename>.cpp

And that's it

cl  -I ..\ -I ..\core  -I ..\svg /EHsc xmlpull.cpp

svgimage
cl  /EHsc  /Zc:__cplusplus /std:c++20 /MT /out:svgimage.exe -I ..\ -I ..\core  -I ..\svg  ..\core\chromiumbase64.c ..\core\fastavxbase64.c svgimage.cpp blend2d.lib  /link /LIBPATH:"..\lib\Release"