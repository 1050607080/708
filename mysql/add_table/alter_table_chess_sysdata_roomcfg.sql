
alter table chess_sysdata.roomcfg modify column param varchar(256) DEFAULT '' COMMENT '�������';


alter table chess_sysdata.roomcfg add column robotminscore bigint(20) DEFAULT 0 NOT NULL COMMENT '��������С����' after showpic;


alter table chess_sysdata.roomcfg add column jettonminscore bigint(20) DEFAULT 0 NOT NULL COMMENT '��С��ע' after robotmaxscore;

alter table chess_sysdata.roomcfg add column exitchip int DEFAULT 0 NOT NULL COMMENT '�˳�����' after jettonminscore;
alter table chess_sysdata.roomcfg add column uproom tinyint DEFAULT 0 NOT NULL COMMENT '�Ƿ�ϳ���0���ҳ� 1ǿ�Ƹϳ� 2��ǿ�Ƹҳ�' after jettonminscore;
