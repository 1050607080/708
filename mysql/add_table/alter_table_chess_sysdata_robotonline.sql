

alter table chess_sysdata.robotonline add online13 int not Null DEFAULT 0;
alter table chess_sysdata.robotonline add online14 int not Null DEFAULT 0;
alter table chess_sysdata.robotonline add online15 int not Null DEFAULT 0;

alter table chess_sysdata.robotonline add online16 int not Null DEFAULT 0;
alter table chess_sysdata.robotonline add online17 int not Null DEFAULT 0;


alter table chess_sysdata.robotonline add logintype int not Null DEFAULT 0 COMMENT '��¼���� 0 ȫ���¼ 1 ����ʱ�ε�¼';
alter table chess_sysdata.robotonline add entertime int not Null DEFAULT 0 COMMENT '��ʼʱ��';
alter table chess_sysdata.robotonline add leavetime int not Null DEFAULT 0 COMMENT '����ʱ��';
alter table chess_sysdata.robotonline add column roomid int DEFAULT 0 NOT NULL COMMENT '����id' after gametype;



