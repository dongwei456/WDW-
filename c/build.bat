@echo off
rem Windows MSVC 编译脚本 (需先 vcvarsall.bat x64，并装好 libcurl)
rem 假设 curl 头文件在 C:\curl\include, lib 在 C:\curl\lib
cl /nologo /W3 /I C:\curl\include demo.c wdw.c /Fe:wdw_demo.exe /link C:\curl\lib\libcurl.lib ws2_32.lib wldap32.lib advapi32.lib
