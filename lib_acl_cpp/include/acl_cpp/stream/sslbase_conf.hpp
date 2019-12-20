#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl
{

class sslbase_io;

class ACL_CPP_API sslbase_conf : public noncopyable
{
public:
	sslbase_conf(void) {}
	virtual ~sslbase_conf(void) {}

	/**
	 * ���鷽�������� SSL IO ����
	 * @param server_side {bool} �Ƿ�Ϊ�����ģʽ����Ϊ�ͻ���ģʽ������
	 *  ģʽ�����ַ�����ͬ������ͨ���˲�������������
	 * @param nblock {bool} �Ƿ�Ϊ������ģʽ
	 * @return {sslbase_io*}
	 */
	virtual sslbase_io* open(bool server_side, bool nblock) = 0;
};

} // namespace acl
