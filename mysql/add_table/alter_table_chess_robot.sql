


alter table chess.robot add column logintype int DEFAULT 0 NOT NULL COMMENT '��¼���� 0 ȫ���¼ 1 ����ʱ�ε�¼' after gametype;

alter table chess.robot add column roomid int DEFAULT 0 NOT NULL COMMENT '����id' after gametype;
