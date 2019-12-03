#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/stream/aio_listen_stream.hpp"
#endif

namespace acl
{

aio_listen_stream::aio_listen_stream(aio_handle *handle)
	: aio_stream(handle)
	, accept_hooked_(false)
{
	addr_[0] = 0;
}

aio_listen_stream::~aio_listen_stream(void)
{
	accept_callbacks_.clear();
}

void aio_listen_stream::destroy(void)
{
	delete this;
}

void aio_listen_stream::add_accept_callback(aio_accept_callback* callback)
{
	std::list<aio_accept_callback*>::iterator it =
		accept_callbacks_.begin();
	for (; it != accept_callbacks_.end(); ++it) {
		if (*it == callback) {
			return;
		}
	}
	accept_callbacks_.push_back(callback);
}

bool aio_listen_stream::open(const char* addr, unsigned flag /* = 0 */)
{
	unsigned oflag = 0;
	if (flag & OPEN_FLAG_REUSEPORT) {
		oflag |= ACL_INET_FLAG_REUSEPORT;
	}
	if (flag & OPEN_FLAG_EXCLUSIVE) {
		oflag |= ACL_INET_FLAG_EXCLUSIVE;
	}
	ACL_VSTREAM *sstream = acl_vstream_listen_ex(addr, 128, oflag, 0, 0);
	if (sstream == NULL) {
		return false;
	}

	return open(sstream);
}

bool aio_listen_stream::open(ACL_SOCKET fd)
{
	unsigned fdtype = 0;
	int type = acl_getsocktype(fd);
	switch (type) {
#ifdef ACL_UNIX
	case AF_UNIX:
		fdtype |= ACL_VSTREAM_TYPE_LISTEN_UNIX;
		break;
#endif
	case AF_INET:
#ifdef AF_INET6
	case AF_INET6:
#endif
		fdtype |= ACL_VSTREAM_TYPE_LISTEN_INET;
		break;
	default: // xxx?
		fdtype |= ACL_VSTREAM_TYPE_LISTEN_INET;
		break;
	}

	ACL_VSTREAM* vstream = acl_vstream_fdopen(fd, 0, 0, -1, fdtype);
	return open(vstream);
}

bool aio_listen_stream::open(ACL_VSTREAM* vstream)
{
	ACL_ASTREAM* astream = acl_aio_open(handle_->get_handle(), vstream);
	return open(astream);
}

bool aio_listen_stream::open(ACL_ASTREAM* astream)
{
	if (astream == NULL) {
		return false;
	}

	ACL_VSTREAM* vstream = acl_aio_vstream(astream);
	if (vstream == NULL) {
		return false;
	}
	ACL_SOCKET fd = ACL_VSTREAM_SOCK(vstream);
	if (fd == ACL_SOCKET_INVALID) {
		return false;
	}
	(void) acl_getsockname(fd, addr_, sizeof(addr_));

	stream_ = astream;
	// ���û���� hook_error ���� handle �������첽������,
	// ͬʱ hook �رռ���ʱ�ص�����
	hook_error();

	// hook �����Ļص�����
	hook_accept();
	return true;
}

const char* aio_listen_stream::get_addr(void) const
{
	return addr_;
}

void aio_listen_stream::hook_accept(void)
{
	acl_assert(stream_);
	if (accept_hooked_) {
		return;
	}
	accept_hooked_ = true;

	acl_aio_ctl(stream_,
		ACL_AIO_CTL_ACCEPT_FN, accept_callback,
		ACL_AIO_CTL_CTX, this,
		ACL_AIO_CTL_END);
	acl_aio_accept(stream_);
}

int aio_listen_stream::accept_callback(ACL_ASTREAM* stream, void* ctx)
{
	aio_listen_stream* as = (aio_listen_stream*) ctx;
	std::list<aio_accept_callback*>::iterator it =
		as->accept_callbacks_.begin();
	aio_socket_stream* ss = NEW aio_socket_stream(as->handle_,
			stream, true);

	for (; it != as->accept_callbacks_.end(); ++it) {
		if ((*it)->accept_callback(ss) == false) {
			return -1;
		}
	}
	return 0;
}

}  // namespace acl
