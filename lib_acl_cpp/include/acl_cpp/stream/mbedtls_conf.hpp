#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/thread_mutex.hpp"
#include "sslbase_conf.hpp"
#include <vector>

namespace acl
{

/**
 * SSL ֤��У�鼶�����Ͷ���
 */
typedef enum
{
	MBEDTLS_VERIFY_NONE,	// ��У��֤��
	MBEDTLS_VERIFY_OPT,	// ѡ����У�飬����������ʱ�����ֺ�У��
	MBEDTLS_VERIFY_REQ	// Ҫ��������ʱУ��
} mbedtls_verify_t;

class mbedtls_io;

/**
 * SSL ���Ӷ���������࣬�������һ���������Ϊȫ�ֶ���������ÿһ�� SSL
 * ���Ӷ������֤�����ã����������ȫ���Ե�֤�顢��Կ����Ϣ��ÿһ�� SSL ����
 * (mbedtls_io) ���ñ������setup_certs ��������ʼ�������֤�顢��Կ����Ϣ
 */
class ACL_CPP_API mbedtls_conf : public sslbase_conf
{
public:
	mbedtls_conf(void);
	~mbedtls_conf(void);

	/**
	 * ���� CA ��֤��(ÿ������ʵ��ֻ�����һ�α�����)
	 * @param ca_file {const char*} CA ֤���ļ�ȫ·��
	 * @param ca_path {const char*} ��� CA ֤���ļ�����Ŀ¼
	 * @return {bool} ����  CA ��֤���Ƿ�ɹ�
	 * ע����� ca_file��ca_path ���ǿգ�������μ�������֤��
	 */
	bool load_ca(const char* ca_file, const char* ca_path);

	/**
	 * ���һ�������/�ͻ����Լ���֤�飬���Զ�ε��ñ��������ض��֤��
	 * @param crt_file {const char*} ֤���ļ�ȫ·�����ǿ�
	 * @return {bool} ���֤���Ƿ�ɹ�
	 */
	bool add_cert(const char* crt_file);

	/**
	 * ��ӷ����/�ͻ��˵���Կ(ÿ������ʵ��ֻ�����һ�α�����)
	 * @param key_file {const char*} ��Կ�ļ�ȫ·�����ǿ�
	 * @param key_pass {const char*} ��Կ�ļ������룬û����Կ�����д NULL
	 * @return {bool} �����Ƿ�ɹ�
	 */
	bool set_key(const char* key_file, const char* key_pass = NULL);

	/**
	 * ��Ϊ�����ģʽʱ�Ƿ����ûỰ���湦�ܣ���������� SSL ����Ч��
	 * @param on {bool}
	 * ע���ú������Է����ģʽ��Ч
	 */
	void enable_cache(bool on);

	/**
	 * �����������������ض���
	 * @return {void*}������ֵΪ entropy_context ����
	 */
	void* get_entropy(void)
	{
		return entropy_;
	}

	/**
	 * mbedtls_io::open �ڲ�����ñ�����������װ��ǰ SSL ���Ӷ����֤��
	 * @param ssl {void*} SSL ���Ӷ���Ϊ ssl_context ����
	 * @param server_side {bool} �����Ӷ����Ƿ�Ϊ���������
	 * @return {bool} ���� SSL �����Ƿ�ɹ�
	 */
	bool setup_certs(void* ssl, bool server_side);

public:
	/**
	 * �������ȵ��ô˺������� libpolarssl.so ��ȫ·��
	 * @param path {const char*} libpolarssl.so ��ȫ·��
	 */
	static void set_libpath(const char* path);

	/**
	 * ������ʽ���ñ���������̬���� polarssl ��̬��
	 */
	static void load(void);

public:
	// @override sslbase_conf
	sslbase_io* open(bool server_side, bool nblock);

private:
	friend class mbedtls_io;

	bool has_inited_;
	thread_mutex lock_;

	void* entropy_;
	void* cacert_;
	void* pkey_;
	void* cert_chain_;
	void* cache_;
	mbedtls_verify_t verify_mode_;

private:
	void init_once(void);
	void free_ca(void);
};

} // namespace acl
