-- ��Ϣ��¼��
create table tb_im202105 (
	t_id		bigint primary key auto_increment,
	t_time		datetime,
	t_send_uid	bigint,        -- ������id
	t_recv_uid	bigint,        -- ������id
	t_type		int,           -- (ToRecver) 0������Ϣ 1�����Ϣ 10�ı���Ϣ 11ͼƬ��Ϣ 12��Ƶ��Ϣ 13��Ƶ��Ϣ    (���浵)(ToSender) 20��Ч��Ϣ 21�ɹ����� 22�Է��Ѷ�
	t_state		int,           -- -1�Ѿ��� 1�Ѽ�¼ 2������������ 3�������Ѷ�
	t_content	varchar(max),
);
