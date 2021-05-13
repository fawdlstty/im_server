-- ��Ϣ��¼��
create table tb_im_202105 (
	t_id		bigint primary key auto_increment,
	t_send_uid	bigint,			-- ������id
	t_recv_uid	bigint,			-- ������id
	t_type		int,			-- (ToRecver) 0������Ϣ 1�����Ϣ 10�ı���Ϣ 11ͼƬ��Ϣ 12��Ƶ��Ϣ 13��Ƶ��Ϣ    (���浵)(ToSender) 20��Ч��Ϣ 21�ɹ����� 22�Է��Ѷ�
	t_state		int,			-- -1�Ѿ��� 1�Ѽ�¼ 2������������ 3�������Ѷ�
	t_send_time	datetime,		-- ����ʱ��
	t_recv_time	datetime null,	-- �ʹ�ʱ��
	t_read_time	datetime null,	-- �Ѷ�ʱ��
	t_content	varchar(max),
);

-- ���ϸ��¼���ǰ��֪ͨ����������Ϣ��
	-- TODO: �л��·�ʱ���ϸ���δ����д��˱�
	-- TODO: ���ʹ�ѽ�����sender�û�δ����ʱ��д��˱�
create table tm_im_pending (
	t_id		bigint primary key auto_increment,
	t_uid		bigint,			-- Ŀ���û�
	t_month		int,			-- �·�
	t_type		int,			-- ��Ϣ����  0δ����Ϣ 1�ѽ���֪ͨ 2�Ѷ�֪ͨ
	t_content	varchar(max),	-- ��ϢID�б�
);
