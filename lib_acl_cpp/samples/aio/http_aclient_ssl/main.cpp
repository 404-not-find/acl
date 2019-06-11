#include <iostream>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
//#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

class http_aio_client : public acl::http_aclient
{
public:
	http_aio_client(acl::aio_handle& handle, acl::polarssl_conf* ssl_conf,
		const char* host)
	: http_aclient(handle, ssl_conf)
	, host_(host)
	, debug_(false)
	, compressed_(false)
	, ws_mode_(false)
	{
	}

	~http_aio_client(void)
	{
		printf("delete http_aio_client and begin stop aio engine\r\n");
		handle_.stop();
	}

	http_aio_client& enable_debug(bool on)
	{
		debug_ = on;
		return *this;
	}

	http_aio_client& enable_websocket(bool on)
	{
		ws_mode_ = on;
		return *this;
	}

protected:
	// @override
	void destroy(void)
	{
		printf("http_aio_client will be deleted!\r\n");
		fflush(stdout);

		delete this;
	}

	// @override
	bool on_connect(void)
	{
		printf("--------------- connect server ok ------------\r\n");
		fflush(stdout);

		if (ws_mode_) {
			printf(">>> begin ws_handshake\r\n");
			this->ws_handshake();
		} else {
			printf(">>> begin send http request\r\n");
			this->send_request(NULL, 0);
		}
		return true;
	}

	// @override
	void on_disconnect(void)
	{
		printf("disconnect from server\r\n");
		fflush(stdout);
	}

	// @override
	void on_connect_timeout(void)
	{
		printf("connect timeout\r\n");
		fflush(stdout);
	}

	// @override
	void on_connect_failed(void)
	{
		printf("connect failed\r\n");
		fflush(stdout);
	}

	// @override
	bool on_http_res_hdr(const acl::http_header& header)
	{
		acl::string buf;
		header.build_response(buf);

		compressed_ = header.is_transfer_gzip();

		printf("---------------response header-----------------\r\n");
		printf("[%s]\r\n", buf.c_str());
		fflush(stdout);

		return true;
	}

	// @override
	bool on_http_res_body(char* data, size_t dlen)
	{
		if (debug_ && (!compressed_ || this->is_unzip_body())) {
			(void) write(1, data, dlen);
		} else {
			printf(">>>read body: %ld\r\n", dlen);
		}
		return true;
	}

	// @override
	bool on_http_res_finish(bool success)
	{
		printf("---------------response over-------------------\r\n");
		printf("http finish: keep_alive=%s, success=%s\r\n",
			keep_alive_ ? "true" : "false",
			success ? "ok" : "failed");
		fflush(stdout);

		return keep_alive_;
	}

protected:
	// @override
	bool on_ws_handshake(void)
	{
		printf(">>> websocket handshake ok\r\n");
		fflush(stdout);

		char buf[128];
		snprintf(buf, sizeof(buf), "hello, myname is zsx\r\n");
		size_t len = strlen(buf);

		if (this->ws_send_text(buf, len) == false) {
			return false;
		}

		// ��ʼ���� websocket �첽������
		this->ws_read_wait(0);
		return true;
	}

	// @override
	void on_ws_handshake_failed(int status)
	{
		printf(">>> websocket handshake failed, status=%d\r\n", status);
		fflush(stdout);
	}

	// @override
	bool on_ws_frame_text(void)
	{
		printf(">>> got frame text type\r\n");
		fflush(stdout);
		return true;
	}

	// @override
	bool on_ws_frame_binary(void)
	{
		printf(">>> got frame binaray type\r\n");
		fflush(stdout);
		return true;
	}

	// @override
	void on_ws_frame_closed(void)
	{
		printf(">>> got frame closed type\r\n");
		fflush(stdout);
	}

	// @override
	bool on_ws_frame_data(char* data, size_t dlen)
	{
		(void) write(1, data, dlen);
		return true;
	}

	// @override
	bool on_ws_frame_finish(void)
	{
		printf(">>> frame finish\r\n");
		fflush(stdout);
		return true;
	}

private:
	acl::string host_;
	bool debug_;
	bool compressed_;
	bool ws_mode_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		" -s server_addr\r\n"
		" -D [if in debug mode, default: false]\r\n"
		" -c cocorrent\r\n"
		" -t connect_timeout[default: 5]\r\n"
		" -i rw_timeout[default: 5]\r\n"
		" -Z [enable_gzip, default: false]\r\n"
		" -U [enable_ungip response, default: false]\r\n"
		" -K [keep_alive, default: false]\r\n"
		" -S polarssl_lib_path[default: none]\n"
		" -N name_server[default: 8.8.8.8:53]\r\n"
		" -W [if using websocket, default: no]\r\n"
		, procname);
}

int main(int argc, char* argv[])
{
	acl::polarssl_conf* ssl_conf = NULL;
	int  ch, conn_timeout = 5, rw_timeout = 5;
	acl::string addr("127.0.0.1:80"), name_server("8.8.8.8:53");
	acl::string host("www.baidu.com"), ssl_lib_path;
	bool enable_gzip = false, keep_alive = false, debug = false;
	bool ws_enable = false, enable_ungip = false;

	while ((ch = getopt(argc, argv, "hs:S:N:H:t:i:ZUKDW")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			addr = optarg;
			break;
		case 'S':
			ssl_lib_path = optarg;
			break;
		case 'N':
			name_server = optarg;
			break;
		case 'H':
			host = optarg;
			break;
		case 't':
			conn_timeout = atoi(optarg);
			break;
		case 'i':
			rw_timeout = atoi(optarg);
			break;
		case 'Z':
			enable_gzip = true;
			break;
		case 'U':
			enable_ungip = true;
			break;
		case 'K':
			keep_alive = true;
			break;
		case 'D':
			debug = true;
			break;
		case 'W':
			ws_enable = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	// ��������� SSL ���ӿ⣬������ SSL ����ģʽ
	if (!ssl_lib_path.empty()) {
		if (access(ssl_lib_path.c_str(), R_OK) == 0) {
			// ���� libpolarssl.so ��ȫ·��
			acl::polarssl_conf::set_libpath(ssl_lib_path);
			// ��̬���� libpolarssl.so ��
			acl::polarssl_conf::load();
			// ����ȫ�� SSL ������
			ssl_conf = new acl::polarssl_conf;
		} else {
			printf("disable ssl, %s not found\r\n",
				ssl_lib_path.c_str());
		}
	}

	// ���� AIO �¼�����
	acl::aio_handle handle(acl::ENGINE_KERNEL);

	// ���� DNS ������������ַ
	handle.set_dns(name_server.c_str(), 5);

	// ��ʼ�첽����Զ�� WEB ������
	http_aio_client* conn = new http_aio_client(handle, ssl_conf, host);
	if (!conn->open(addr, conn_timeout, rw_timeout)) {
		printf("connect %s error\r\n", addr.c_str());
		fflush(stdout);

		delete conn;
		return 1;
	}

	(*conn).enable_debug(debug)		// �Ƿ����õ��Է�ʽ
		.enable_websocket(ws_enable);	// �Ƿ����� websocket
	conn->unzip_body(enable_ungip);		// ��� HTTP �Ƿ��Զ���ѹ

	// ���� HTTP ����ͷ��Ҳ�ɽ��˹��̷��� conn->on_connect() ��
	acl::http_header& head = conn->request_header();
	head.set_url("/")
		.set_content_length(0)
		.set_host(host)
		.accept_gzip(enable_gzip)
		.set_keep_alive(keep_alive);

	acl::string buf;
	head.build_request(buf);
	printf("---------------request header-----------------\r\n");
	printf("[%s]\r\n", buf.c_str());
	fflush(stdout);

	// ��ʼ AIO �¼�ѭ������
	while (true) {
		// ������� false ���ʾ���ټ�������Ҫ�˳�
		if (!handle.check()) {
			break;
		}
	}

	handle.check();
	delete ssl_conf;
	return 0;
}
