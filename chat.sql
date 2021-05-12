-- 消息记录表
create table tb_im202105 (
	t_id		bigint primary key auto_increment,
	t_time		datetime,
	t_send_uid	bigint,        -- 发送者id
	t_recv_uid	bigint,        -- 接收者id
	t_type		int,           -- (ToRecver) 0命令消息 1广告信息 10文本消息 11图片消息 12音频消息 13视频消息    (不存档)(ToSender) 20无效消息 21成功发送 22对方已读
	t_state		int,           -- -1已拒收 1已记录 2已送至接收者 3接收者已读
	t_content	varchar(max),
);
