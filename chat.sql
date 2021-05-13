-- 消息记录表
create table tb_im_202105 (
	t_id		bigint primary key auto_increment,
	t_send_uid	bigint,			-- 发送者id
	t_recv_uid	bigint,			-- 接收者id
	t_type		int,			-- (ToRecver) 0命令消息 1广告信息 10文本消息 11图片消息 12音频消息 13视频消息    (不存档)(ToSender) 20无效消息 21成功发送 22对方已读
	t_state		int,			-- -1已拒收 1已记录 2已送至接收者 3接收者已读
	t_send_time	datetime,		-- 发送时间
	t_recv_time	datetime null,	-- 送达时间
	t_read_time	datetime null,	-- 已读时间
	t_content	varchar(max),
);

-- （上个月及更前及通知）待处理消息表
	-- TODO: 切换月份时，上个月未处理写入此表
	-- TODO: 已送达、已接收在sender用户未上线时，写入此表
create table tm_im_pending (
	t_id		bigint primary key auto_increment,
	t_uid		bigint,			-- 目标用户
	t_month		int,			-- 月份
	t_type		int,			-- 消息类型  0未读信息 1已接收通知 2已读通知
	t_content	varchar(max),	-- 消息ID列表
);
