#pragma once
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
class websocket;
class polarssl_conf;
class polarssl_io;
class http_header;

/**
 * HTTP �ͻ����첽ͨ���֧࣬�� SSL ���ܴ���
 */
class ACL_CPP_API http_aclient : public aio_open_callback
{
public:
	/**
	 * ���캯��
	 * @param handle {aio_handle&} �첽ͨ���¼�������
	 * @param ssl_conf {polarssl_conf*} �� NULL ʱ�Զ����� SSL ͨ�ŷ�ʽ
	 */
	http_aclient(aio_handle& handle, polarssl_conf* ssl_conf = NULL);
	virtual ~http_aclient(void);

	/**
	 * ����������ʱ�Ļص��������������ʵ��
	 */
	virtual void destroy(void) = 0;

	/**
	 * ��� HTTP ����ͷ���Ա���Ӧ����� HTTP ����ͷ�е��ֶ�����
	 * @return {http_header&}
	 */
	http_header& request_header(void);

	/**
	 * ��ʼ�첽����Զ�� WEB ������
	 * @param addr {const char*} Զ�� WEB ��������ַ����ʽΪ��
	 *  domain:port �� ip:port, ����ַΪ����ʱ���ڲ��Զ������첽��������
	 *  ���̣���Ҫ���ڳ���ʼʱ����ͨ�� aio_handle::set_dns() ���ù�����
	 *  ��������ַ�������ַΪ IP ����Ҫ������������������ַ
	 * @param conn_timeout {int} ���ӳ�ʱʱ�䣨�룩
	 * @param rw_timeout {int} ���� IO ��д��ʱʱ�䣨�룩
	 * @return {bool} ���� false ��ʾ����ʧ�ܣ����� true ��ʾ�����첽������
	 */
	bool open(const char* addr, int conn_timeout, int rw_timeout);

protected:
	/**
	 * �����ӳɹ���Ļص��������������ʵ�֣�����Ӧ�ڸ÷����ﹹ�� HTTP ����
	 * ������ send_request ������ WEB ���������� HTTP ����
	 * @return {bool} �÷���������� false ���ڲ����Զ��ر�����
	 */
	virtual bool on_connect(void) = 0;

	/**
	 * �����ӳ�ʱ��Ļص�����
	 */
	virtual void on_connect_timeout(void) {}

	/**
	 * ������ʧ�ܺ�Ļص�����
	 */
	virtual void on_connect_failed(void) {}

	/**
	 * ���������ʱʱ�Ļص�����
	 */
	virtual void on_read_timeout(void) {}

	/**
	 * �������ӳɹ������ӹرպ�Ļص�����
	 */
	virtual void on_disconnect(void) {};

	/**
	 * �����յ� WEB ����˵���Ӧͷʱ�Ļص�����
	 * @param header {const http_header&}
	 * @return {bool} ���� false �򽫻�ر����ӣ����������
	 */
	virtual bool on_http_res_hdr(const http_header& header)
	{
		(void) header;
		return true;
	}

	/**
	 * �����յ� WEB ����˵���Ӧ��ʱ�Ļص��������÷������ܻᱻ��λص�
	 * ֱ����Ӧ���ݶ�������
	 * @param data {char*} �����Ĳ�������������
	 * @param dlen {size_t} ���ζ����� data ���ݵĳ���
	 * @return {bool} ���� false �򽫻�ر����ӣ����������
	 */
	virtual bool on_http_res_body(char* data, size_t dlen)
	{
		(void) data;
		(void) dlen;
		return true;
	}

	/**
	 * ������ HTTP ��Ӧ�������Ļص�����
	 * @param success {bool} �Ƿ�ɹ����� HTTP ��Ӧ������
	 * @return {bool} ����ɹ�����������󷵻� false ���ر�����
	 */
	virtual bool on_http_res_finish(bool success)
	{
		(void) success;
		return true;
	}

	/**
	 * ������һ�� text ���͵�֡ʱ�Ļص�����
	 * @return {bool} ���� true ��ʾ��������������Ҫ��ر�����
	 */
	virtual bool on_ws_frame_text(void) { return true; }

	/**
	 * ������һ�� binary ���͵�֡ʱ�Ļص�����
	 * @return {bool} ���� true ��ʾ��������������Ҫ��ر�����
	 */
	virtual bool on_ws_frame_binary(void) { return true; }

	/**
	 * ������һ���ر�֡����ʱ�Ļص�����
	 */
	virtual void on_ws_frame_closed(void) {}

	/**
	 * �� websocket ͨ�ŷ�ʽ��������������ʱ�Ļص�����
	 * @param data {char*} ���������ݵ�ַ
	 * @param dlen {size_t} ���������ݳ���
	 * @return {bool} ���� true ��ʾ��������������Ҫ��ر�����
	 */
	virtual bool on_ws_frame_data(char* data, size_t dlen)
	{
		(void) data;
		(void) dlen;
		return true;
	}

	/**
	 * ������һ֡����ʱ�Ļص�����
	 * @return {bool} ���� true ��ʾ��������������Ҫ��ر�����
	 */
	virtual bool on_ws_frame_finish(void) { return true; }

protected:
	/**
	 * �� WEB ���������� HTTP �����ڲ��ڷ��ͺ���Զ���ʼ�� HTTP ��Ӧ����
	 * @param body {const void*} HTTP ����������壬��Ϊ NULL ʱ���ڲ�����
	 *  ������ HTTP GET ����
	 * @param len {size_t} body �� NULL ʱ��ʾ������ĳ���
	 */
	void send_request(const void* body, size_t len);

	/**
	 * ����������� WEBSOCKET ����
	 */
	void ws_handshake(void);

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
