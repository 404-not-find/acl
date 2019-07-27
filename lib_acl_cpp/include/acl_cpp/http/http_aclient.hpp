﻿#pragma once
#include "../acl_cpp_define.hpp"
#include "../stream/aio_socket_stream.hpp"

struct HTTP_HDR;
struct HTTP_HDR_RES;
struct HTTP_RES;
struct HTTP_HDR_REQ;
struct HTTP_REQ;

struct ACL_ASTREAM;

namespace acl {

class aio_handle;
class aio_socket_stream;
class socket_stream;
class zlib_stream;
class websocket;
class polarssl_conf;
class polarssl_io;
class http_header;

/**
 * HTTP 客户端异步通信类，不仅支持标准 HTTP 通信协议，而且支持 Websocket 通信，
 * 对于 HTTP 协议及 Websocket 通信均支持 SSL 加密传输；
 * 另外，对于 HTTP 协议，根据用户设置，可以自动解压 GZIP 响应数据，这样在回调
 * 方法 on_http_res_body() 中收到的便是解压后的明文数据。
 */
class ACL_CPP_API http_aclient : public aio_open_callback
{
public:
	/**
	 * 构造函数
	 * @param handle {aio_handle&} 异步通信事件引擎句柄
	 * @param ssl_conf {polarssl_conf*} 非 NULL 时自动采用 SSL 通信方式
	 */
	http_aclient(aio_handle& handle, polarssl_conf* ssl_conf = NULL);
	virtual ~http_aclient(void);

	/**
	 * 当对象销毁时的回调方法，子类必须实现
	 */
	virtual void destroy(void) = 0;

	/**
	 * 获得 HTTP 请求头，以便于应用添加 HTTP 请求头中的字段内容
	 * @return {http_header&}
	 */
	http_header& request_header(void);

	/**
	 * 针对 HTTP 协议的响应数据是否自动进行解压
	 * @param on {bool}
	 * @return {http_aclient&}
	 */
	http_aclient& unzip_body(bool on);

	/**
	 * 是否针对 GZIP 压缩数据自动进行解压
	 * @return {bool}
	 */
	bool is_unzip_body(void) const
	{
		return unzip_;
	}

	/**
	 * 开始异步连接远程 WEB 服务器
	 * @param addr {const char*} 远程 WEB 服务器地址，格式为：
	 *  domain:port 或 ip:port, 当地址为域名时，内部自动进行异步域名解析
	 *  过程，但要求在程序开始时必须通过 aio_handle::set_dns() 设置过域名
	 *  服务器地址，如果地址为 IP 则不需要先设置域名服务器地址
	 * @param conn_timeout {int} 连接超时时间（秒）
	 * @param rw_timeout {int} 网络 IO 读写超时时间（秒）
	 * @return {bool} 返回 false 表示连接失败，返回 true 表示进入异步连接中
	 */
	bool open(const char* addr, int conn_timeout, int rw_timeout);

protected:
	/**
	 * 当连接成功后的回调方法，子类必须实现，子类应在该方法里构造 HTTP 请求
	 * 并调用 send_request 方法向 WEB 服务器发送 HTTP 请求
	 * @return {bool} 该方法如果返回 false 则内部会自动关闭连接
	 */
	virtual bool on_connect(void) = 0;

	/**
	 * 当连接超时后的回调方法
	 */
	virtual void on_connect_timeout(void) {}

	/**
	 * 当连接失败后的回调方法
	 */
	virtual void on_connect_failed(void) {}

	/**
	 * 当网络读超时时的回调方法
	 */
	virtual void on_read_timeout(void) {}

	/**
	 * 对于连接成功后连接关闭后的回调方法
	 */
	virtual void on_disconnect(void) {};

	/**
	 * 当接收到 WEB 服务端的响应头时的回调方法
	 * @param header {const http_header&}
	 * @return {bool} 返回 false 则将会关闭连接，否则继续读
	 */
	virtual bool on_http_res_hdr(const http_header& header)
	{
		(void) header;
		return true;
	}

	/**
	 * 当接收到 WEB 服务端的响应体时的回调方法，该方法可能会被多次回调
	 * 直到响应数据读完或出错
	 * @param data {char*} 读到的部分数据体内容
	 * @param dlen {size_t} 本次读到的 data 数据的长度
	 * @return {bool} 返回 false 则将会关闭连接，否则继续读
	 */
	virtual bool on_http_res_body(char* data, size_t dlen)
	{
		(void) data;
		(void) dlen;
		return true;
	}

	/**
	 * 当读完 HTTP 响应体或出错后的回调方法
	 * @param success {bool} 是否成功读完 HTTP 响应体数据
	 * @return {bool} 如果成功读完数据体后返回 false 则会关闭连接
	 */
	virtual bool on_http_res_finish(bool success)
	{
		(void) success;
		return true;
	}

	/**
	 * 当 websocket 握手成功后的回调方法
	 * @return {bool} 返回 false 表示需要关闭连接，否则继续
	 */
	virtual bool on_ws_handshake(void)
	{
		// 开始异步读 websocket 数据
		this->ws_read_wait(0);
		return true;
	}

	/**
	 * 当 websocket 握手失败后的回调方法
	 * @param status {int} 服务器返回的 HTTP 响应状态码
	 */
	virtual void on_ws_handshake_failed(int status) { (void) status; }

	/**
	 * 当读到一个 text 类型的帧时的回调方法
	 * @return {bool} 返回 true 表示继续读，否则则要求关闭连接
	 */
	virtual bool on_ws_frame_text(void) { return true; }

	/**
	 * 当读到一个 binary 类型的帧时的回调方法
	 * @return {bool} 返回 true 表示继续读，否则则要求关闭连接
	 */
	virtual bool on_ws_frame_binary(void) { return true; }

	/**
	 * 当读到一个关闭帧数据时的回调方法
	 */
	virtual void on_ws_frame_closed(void) {}

	/**
	 * 在 websocket 通信方式，当读到数据体时的回调方法
	 * @param data {char*} 读到的数据地址
	 * @param dlen {size_t} 读到的数据长度
	 * @return {bool} 返回 true 表示继续读，否则则要求关闭连接
	 */
	virtual bool on_ws_frame_data(char* data, size_t dlen)
	{
		(void) data;
		(void) dlen;
		return true;
	}

	/**
	 * 当读完一帧数据时的回调方法
	 * @return {bool} 返回 true 表示继续读，否则则要求关闭连接
	 */
	virtual bool on_ws_frame_finish(void) { return true; }

public:
	/**
	 * 向 WEB 服务器发送 HTTP 请求，内部在发送后会自动开始读 HTTP 响应过程
	 * @param body {const void*} HTTP 请求的数据体，当为 NULL 时，内部会自
	 *  动采用 HTTP GET 方法
	 * @param len {size_t} body 非 NULL 时表示数据体的长度
	 */
	void send_request(const void* body, size_t len);

	/**
	 * 与服务器进行 WEBSOCKET 握手
	 */
	void ws_handshake(void);

	/**
	 * 开始异步读 websocket 数据
	 * @param timeout {int} 读超时时间
	 */
	void ws_read_wait(int timeout = 0);

	/**
	 * 异步发送一个 FRAME_TEXT 类型的数据帧
	 * @param data {char*} 内部可能因添加掩码原因被改变内容
	 * @param len {size_t} data 数据长度
	 * @return {bool}
	 */
	bool ws_send_text(char* data, size_t len);

	/**
	 * 异步发送一个 FRAME_BINARY 类型的数据帧
	 * @param data {void*} 内部可能因添加掩码原因被改变内容
	 * @param len {size_t} data 数据长度
	 * @return {bool}
	 */
	bool ws_send_binary(void* data, size_t len);

	/**
	 * 异步发送一个 FRAME_PING 类型的数据帧
	 * @param data {void*} 内部可能因添加掩码原因被改变内容
	 * @param len {size_t} data 数据长度
	 * @return {bool}
	 */
	bool ws_send_ping(void* data, size_t len);

	/**
	 * 异步发送一个 FRAME_PONG 类型的数据帧
	 * @param data {void*} 内部可能因添加掩码原因被改变内容
	 * @param len {size_t} data 数据长度
	 * @return {bool}
	 */
	bool ws_send_pong(void* data, size_t len);

protected:
	// @override dummy
	bool open_callback(void) { return true; }

	// @override
	bool timeout_callback(void);

	// @override
	void close_callback(void);

	// @override
	bool read_wakeup(void);

	// @override
	bool read_callback(char* data, int len);

protected:
	unsigned           status_;
	aio_handle&        handle_;
	polarssl_conf*     ssl_conf_;
	int                rw_timeout_;
	aio_socket_stream* conn_;
	socket_stream*     stream_;
	http_header*       header_;
	HTTP_HDR_RES*      hdr_res_;
	HTTP_RES*          http_res_;
	bool               keep_alive_;
	websocket*         ws_in_;
	websocket*         ws_out_;
	string*            buff_;
	bool               unzip_;		// 是否自动解压响应体数据
	zlib_stream*       zstream_;		// 解压对象
	int                gzip_header_left_;	// gzip 传输时压缩头部长度

	bool handle_connect(ACL_ASTREAM* stream);
	bool handle_ssl_handshake(void);

	bool handle_res_hdr(int status);

	bool handle_res_body(char* data, int dlen);
	bool res_plain(char* data, int dlen);
	bool res_unzip(zlib_stream& zstream, char* data, int dlen);

	bool handle_res_body_finish(char* data, int dlen);
	bool res_plain_finish(char* data, int dlen);
	bool res_unzip_finish(zlib_stream& zstream, char* data, int dlen);

	bool handle_websocket(void);
	bool handle_ws_data(void);
	bool handle_ws_ping(void);
	bool handle_ws_pong(void);
	bool handle_ws_other(void);

private:
	static int connect_callback(ACL_ASTREAM* stream, void* ctx);
	static int http_res_hdr_cllback(int status, void* ctx);
	static int http_res_callback(int status, char* data, int dlen, void* ctx);
};

} // namespace acl
