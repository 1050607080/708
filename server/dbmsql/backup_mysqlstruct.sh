#!/bin/sh
# �������ݿ�ṹ

mysqldump -uroot -p123456 -A -d > ./all.sql
mysqldump -uroot -p123456 chess_sysdata > ./chess_sysdata.sql

