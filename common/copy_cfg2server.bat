rem  ���������ļ���serverĿ¼��

rem  ��������ļ�
del /Q ..\server\tdr_auto\src\*.h
del /Q ..\server\tdr_auto\src\*.cpp


rem ����������

xcopy cs_msg\cpp\*.* ..\server\servers\common\src\pb  /Y /E

pause