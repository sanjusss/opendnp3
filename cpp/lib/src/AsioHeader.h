#ifndef OPENDNP3_ASIOHEADER_H
#define OPENDNP3_ASIOHEADER_H

#ifdef ASIO_STANDALONE
#include <asio.hpp>
#define ASIO asio
typedef std::error_code ASIO_ERROR;
#else
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#define ASIO boost::asio
typedef boost::system::error_code ASIO_ERROR;
#endif

#endif // OPENDNP3_ASIOHEADER_H