// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include <string.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/utility/string_ref.hpp>

#include "civetweb/civetweb.h"
#include "rgw_civetweb.h"


#define dout_subsys ceph_subsys_rgw

size_t RGWCivetWeb::write_data(const char *buf, const size_t len)
{
<<<<<<< HEAD
  auto to_sent = len;
  while (to_sent) {
    const int ret = mg_write(conn, buf, len);
    if (ret < 0 || ! ret) {
      /* According to the documentation of mg_write() it always returns -1 on
       * error. The details aren't available, so we will just throw EIO. Same
       * goes to 0 that is associated with writing to a closed connection. */
      throw rgw::io::Exception(EIO, std::system_category());
    } else {
      to_sent -= static_cast<size_t>(ret);
    }
  }
  return len;
}

RGWCivetWeb::RGWCivetWeb(mg_connection* const conn)
  : conn(conn),
    explicit_keepalive(false),
    explicit_conn_close(false),
    txbuf(*this)
=======
  if (!header_done) {
    header_data.append(buf, len);
    return len;
  }
  if (!sent_header) {
    data.append(buf, len);
    return len;
  }
  int r = mg_write(conn, buf, len);
  if (r == 0) {
    /* didn't send anything, error out */
    return -EIO;
  }
  return r;
}

RGWMongoose::RGWMongoose(mg_connection *_conn, int _port) : conn(_conn), port(_port), status_num(0), header_done(false),
                                                 sent_header(false), has_content_length(false),
                                                 explicit_keepalive(false), explicit_conn_close(false)
>>>>>>> upstream/hammer
{
    sockaddr *lsa = mg_get_local_addr(conn);
    switch(lsa->sa_family) {
    case AF_INET:
	port = ntohs(((struct sockaddr_in*)lsa)->sin_port);
	break;
    case AF_INET6:
	port = ntohs(((struct sockaddr_in6*)lsa)->sin6_port);
	break;
    default:
	port = -1;
    }
}

size_t RGWCivetWeb::read_data(char *buf, size_t len)
{
  const int ret = mg_read(conn, buf, len);
  if (ret < 0) {
    throw rgw::io::Exception(EIO, std::system_category());
  }
  return ret;
}

void RGWCivetWeb::flush()
{
  txbuf.pubsync();
}

size_t RGWCivetWeb::complete_request()
{
<<<<<<< HEAD
=======
  if (!sent_header) {
    if (!has_content_length) {

      header_done = false; /* let's go back to writing the header */

      /*
       * Status 204 should not include a content-length header
       * RFC7230 says so
       *
       * Same goes for status 304: Not Modified
       *
       * 'If a cache uses a received 304 response to update a cache entry,'
       * 'the cache MUST update the entry to reflect any new field values'
       * 'given in the response.'
       *
       */
      if (status_num == 204 || status_num == 304) {
        has_content_length = true;
      } else if (0 && data.length() == 0) {
        has_content_length = true;
        print("Transfer-Enconding: %s\r\n", "chunked");
        data.append("0\r\n\r\n", sizeof("0\r\n\r\n")-1);
      } else {
        int r = send_content_length(data.length());
        if (r < 0)
	  return r;
      }
    }

    complete_header();
  }

  if (data.length()) {
    int r = write_data(data.c_str(), data.length());
    if (r < 0)
      return r;
    data.clear();
  }

>>>>>>> upstream/hammer
  return 0;
}

void RGWCivetWeb::init_env(CephContext *cct)
{
  env.init(cct);
<<<<<<< HEAD
  const struct mg_request_info* info = mg_get_request_info(conn);

  if (! info) {
=======
  struct mg_request_info *info = mg_get_request_info(conn);

  if (!info)
>>>>>>> upstream/hammer
    return;
  }

  for (int i = 0; i < info->num_headers; i++) {
    const struct mg_request_info::mg_header* header = &info->http_headers[i];
    const boost::string_ref name(header->name);
    const auto& value = header->value;

    if (boost::algorithm::iequals(name, "content-length")) {
      env.set("CONTENT_LENGTH", value);
      continue;
    }
    if (boost::algorithm::iequals(name, "content-type")) {
      env.set("CONTENT_TYPE", value);
      continue;
    }
<<<<<<< HEAD
    if (boost::algorithm::iequals(name, "connection")) {
      explicit_keepalive = boost::algorithm::iequals(value, "keep-alive");
      explicit_conn_close = boost::algorithm::iequals(value, "close");
=======

    if (strcasecmp(header->name, "connection") == 0) {
      explicit_keepalive = (strcasecmp(header->value, "keep-alive") == 0);
      explicit_conn_close = (strcasecmp(header->value, "close") == 0);
>>>>>>> upstream/hammer
    }

    static const boost::string_ref HTTP_{"HTTP_"};

    char buf[name.size() + HTTP_.size() + 1];
    auto dest = std::copy(std::begin(HTTP_), std::end(HTTP_), buf);
    for (auto src = name.begin(); src != name.end(); ++src, ++dest) {
      if (*src == '-') {
        *dest = '_';
      } else {
        *dest = std::toupper(*src);
      }
    }
    *dest = '\0';

<<<<<<< HEAD
    env.set(buf, value);
=======
    env.set(buf, header->value);
>>>>>>> upstream/hammer
  }

  env.set("REQUEST_METHOD", info->request_method);
  env.set("REQUEST_URI", info->uri);
  env.set("SCRIPT_URI", info->uri); /* FIXME */
  if (info->query_string) {
    env.set("QUERY_STRING", info->query_string);
  }
  if (info->remote_user) {
    env.set("REMOTE_USER", info->remote_user);
  }

  if (port <= 0)
    lderr(cct) << "init_env: bug: invalid port number" << dendl;
  char port_buf[16];
  snprintf(port_buf, sizeof(port_buf), "%d", port);
  env.set("SERVER_PORT", port_buf);
<<<<<<< HEAD
  if (info->is_ssl) {
=======

  if (info->is_ssl) {
    if (port == 0) {
      strcpy(port_buf,"443");
    }
>>>>>>> upstream/hammer
    env.set("SERVER_PORT_SECURE", port_buf);
  }
}

<<<<<<< HEAD
size_t RGWCivetWeb::send_status(int status, const char *status_name)
=======
int RGWMongoose::send_status(int status, const char *status_name)
>>>>>>> upstream/hammer
{
  mg_set_http_status(conn, status);

  static constexpr size_t STATUS_BUF_SIZE = 128;

<<<<<<< HEAD
  char statusbuf[STATUS_BUF_SIZE];
  const auto statuslen = snprintf(statusbuf, sizeof(statusbuf),
                                  "HTTP/1.1 %d %s\r\n", status, status_name);
=======
  snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\n", status, status_name);
>>>>>>> upstream/hammer

  return txbuf.sputn(statusbuf, statuslen);
}

<<<<<<< HEAD
size_t RGWCivetWeb::send_100_continue()
{
  const char HTTTP_100_CONTINUE[] = "HTTP/1.1 100 CONTINUE\r\n\r\n";
  const size_t sent = txbuf.sputn(HTTTP_100_CONTINUE,
                                  sizeof(HTTTP_100_CONTINUE) - 1);
  flush();
  return sent;
=======
  status_num = status;
  mg_set_http_status(conn, status_num);

  return 0;
>>>>>>> upstream/hammer
}

size_t RGWCivetWeb::send_header(const boost::string_ref& name,
                                const boost::string_ref& value)
{
  static constexpr char HEADER_SEP[] = ": ";
  static constexpr char HEADER_END[] = "\r\n";

  size_t sent = 0;

  sent += txbuf.sputn(name.data(), name.length());
  sent += txbuf.sputn(HEADER_SEP, sizeof(HEADER_SEP) - 1);
  sent += txbuf.sputn(value.data(), value.length());
  sent += txbuf.sputn(HEADER_END, sizeof(HEADER_END) - 1);

  return sent;
}

<<<<<<< HEAD
size_t RGWCivetWeb::dump_date_header()
=======
static void dump_date_header(bufferlist &out)
{
  char timestr[TIME_BUF_SIZE];
  const time_t gtime = time(NULL);
  struct tm result;
  struct tm const * const tmp = gmtime_r(&gtime, &result);

  if (tmp == NULL)
    return;

  if (strftime(timestr, sizeof(timestr), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", tmp))
    out.append(timestr);
}

int RGWMongoose::complete_header()
>>>>>>> upstream/hammer
{
  char timestr[TIME_BUF_SIZE];

  const time_t gtime = time(nullptr);
  struct tm result;
  struct tm const* const tmp = gmtime_r(&gtime, &result);

  if (nullptr == tmp) {
    return 0;
  }

<<<<<<< HEAD
  if (! strftime(timestr, sizeof(timestr),
                 "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", tmp)) {
    return 0;
  }
=======
  dump_date_header(header_data);

  if (explicit_keepalive)
    header_data.append("Connection: Keep-Alive\r\n");
  else if (explicit_conn_close)
    header_data.append("Connection: close\r\n");
>>>>>>> upstream/hammer

  return txbuf.sputn(timestr, strlen(timestr));
}

size_t RGWCivetWeb::complete_header()
{
  size_t sent = dump_date_header();

  if (explicit_keepalive) {
    constexpr char CONN_KEEP_ALIVE[] = "Connection: Keep-Alive\r\n";
    sent += txbuf.sputn(CONN_KEEP_ALIVE, sizeof(CONN_KEEP_ALIVE) - 1);
  } else if (explicit_conn_close) {
    constexpr char CONN_KEEP_CLOSE[] = "Connection: close\r\n";
    sent += txbuf.sputn(CONN_KEEP_CLOSE, sizeof(CONN_KEEP_CLOSE) - 1);
  }

  static constexpr char HEADER_END[] = "\r\n";
  sent += txbuf.sputn(HEADER_END, sizeof(HEADER_END) - 1);

  flush();
  return sent;
}

size_t RGWCivetWeb::send_content_length(uint64_t len)
{
  static constexpr size_t CONLEN_BUF_SIZE = 128;

  char sizebuf[CONLEN_BUF_SIZE];
  const auto sizelen = snprintf(sizebuf, sizeof(sizebuf),
                                "Content-Length: %" PRIu64 "\r\n", len);
  return txbuf.sputn(sizebuf, sizelen);
}
