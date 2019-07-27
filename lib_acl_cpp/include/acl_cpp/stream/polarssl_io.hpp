﻿#pragma once
#include "../acl_cpp_define.hpp"
#include "stream_hook.hpp"

struct ACL_VSTREAM;

namespace acl {

class polarssl_conf;
class atomic_long;

/**
 * stream/aio_stream 流对象底层 IO 处理过程的处理类，该类对象中的读写的过程将会替代
 * stream/aio_stream 流对象中 默认的底层 IO 过程；该类对象必须是动态创建的(即为堆对象)，
 * stream/aio_stream 流对象通过调用本类对象的 destroy()　方法释放本类对象
 */
class ACL_CPP_API polarssl_io : public stream_hook
{
public:
	/**
	 * 构造函数
	 * @param conf {polarssl_conf&} 对每一个 SSL 连接进行配置的类对象
	 * @param server_side {bool} 是否为服务端模式，因为客户端模式与服务端
	 *  模式的握手方法不同，所以通过此参数来进行区分
	 * @param nblock {bool} 是否为非阻塞模式
	 */
	polarssl_io(polarssl_conf& conf, bool server_side, bool nblock = false);

	/**
	 * 销毁 SSL IO 对象
	 */
	virtual void destroy(void);

	/**
	 * 调用此方法进行 SSL 握手，在非阻塞 IO 模式下该函数需要与 handshake_ok()
	 * 函数组合使用来判断 SSL 握手是否成功
	 * @return {bool}
	 *  1、返回 false 表示握手失败，需要关闭连接；
	 *  2、当返回 true 时：
	 *  2.1、如果为阻塞 IO 模式则表示 SSL 握手成功
	 *  2.2、在非阻塞 IO 模式下仅代表本次握手过程中 IO 是成功的，还需要调用
	 *       handshake_ok() 函数判断 SSL 握手是否成功
	 */
	bool handshake(void);

	/**
	 * 判断 SSL 握手是否成功
	 * @return {bool}
	 */
	bool handshake_ok(void)
	{
		return handshake_ok_;
	}

	/**
	 * 设置套接字为阻塞模式/非阻塞模式
	 * @param yes {bool} 当为 false 时则设为阻塞模式，否则设为非阻塞模式
	 * @return {polarssl_io&}
	 */
	polarssl_io& set_non_blocking(bool yes);

	/**
	 * 判断当前设置的 SSL IO 是否阻塞模式还是非阻塞模式
	 * @return {bool} 返回 true 则表示为非阻塞模式，否则为阻塞模式
	 */
	bool is_non_blocking(void) const
	{
		return nblock_;
	}


	/**
	 * 检查对方证书是否有效（一般不必调用此函数）
	 * @return {bool}
	 */
	bool check_peer(void);

private:
	~polarssl_io(void);

	virtual bool open(ACL_VSTREAM* s);
	virtual bool on_close(bool alive);
	virtual int read(void* buf, size_t len);
	virtual int send(const void* buf, size_t len);

private:
	polarssl_conf& conf_;
	atomic_long* refers_;
	bool  server_side_;
	bool  nblock_;
	bool  handshake_ok_;
	void* ssl_;
	void* ssn_;
	void* rnd_;
	ACL_VSTREAM* stream_;

	static int sock_read(void *ctx, unsigned char *buf, size_t len);
	static int sock_send(void *ctx, const unsigned char *buf, size_t len);
};

} // namespace acl
