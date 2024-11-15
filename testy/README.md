
How to build fro a Visual Studio command prompt

* Specify include directory paths
* Use exception handling
* If rendering, include blend2d library


cl  -I..\..\ -I..\..\app -I ..\..\svg /EHsc xmlpull.cpp

svgimage<p>
cl  /EHsc  /Zc:__cplusplus /std:c++14 /MT  -I..\..\ -I..\..\app -I ..\..\svg   svgimage.cpp blend2d.lib  /link /LIBPATH:"..\..\lib\Release"
