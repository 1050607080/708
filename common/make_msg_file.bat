echo ����cpp����

for %%a in (cs_msg\*.proto) do .\tools\protoc -I=.\cs_msg\ --cpp_out=.\cs_msg\cpp\ %%a

pause