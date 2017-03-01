// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <curl/curl.h>

#include <boost/intrusive_ptr.hpp>

#include "acconfig.h"

#include "common/ceph_argparse.h"
#include "global/global_init.h"
#include "global/signal_handler.h"
#include "common/config.h"
#include "common/errno.h"
#include "common/Timer.h"
#include "common/safe_io.h"
#include "include/compat.h"
#include "include/str_list.h"
#include "rgw_common.h"
#include "rgw_rados.h"
#include "rgw_user.h"
#include "rgw_period_pusher.h"
#include "rgw_realm_reloader.h"
#include "rgw_rest.h"
#include "rgw_rest_s3.h"
#include "rgw_rest_swift.h"
#include "rgw_rest_admin.h"
#include "rgw_rest_usage.h"
#include "rgw_rest_user.h"
#include "rgw_rest_bucket.h"
#include "rgw_rest_metadata.h"
#include "rgw_rest_log.h"
#include "rgw_rest_opstate.h"
#include "rgw_replica_log.h"
#include "rgw_rest_replica_log.h"
#include "rgw_rest_config.h"
#include "rgw_rest_realm.h"
#include "rgw_swift_auth.h"
#include "rgw_log.h"
#include "rgw_tools.h"
#include "rgw_resolve.h"

#include "rgw_request.h"
#include "rgw_process.h"
#include "rgw_frontend.h"
#if defined(WITH_RADOSGW_ASIO_FRONTEND)
#include "rgw_asio_frontend.h"
#endif /* WITH_RADOSGW_ASIO_FRONTEND */

#include <map>
#include <string>
#include <vector>

#include "include/types.h"
#include "common/BackTrace.h"

#define dout_subsys ceph_subsys_rgw

using namespace std;

static sig_t sighandler_alrm;

class RGWProcess;

static int signal_fd[2] = {0, 0};
static atomic_t disable_signal_fd;

<<<<<<< HEAD
void signal_shutdown()
=======
static void signal_shutdown();


#define SOCKET_BACKLOG 1024

struct RGWRequest
{
  uint64_t id;
  struct req_state *s;
  string req_str;
  RGWOp *op;
  utime_t ts;

  RGWRequest(uint64_t id) : id(id), s(NULL), op(NULL) {
  }

  virtual ~RGWRequest() {}

  void init_state(req_state *_s) {
    s = _s;
  }

  void log_format(struct req_state *s, const char *fmt, ...)
  {
#define LARGE_SIZE 1024
    char buf[LARGE_SIZE];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    log(s, buf);
  }

  void log_init() {
    ts = ceph_clock_now(g_ceph_context);
  }

  void log(struct req_state *s, const char *msg) {
    if (s->info.method && req_str.size() == 0) {
      req_str = s->info.method;
      req_str.append(" ");
      req_str.append(s->info.request_uri);
    }
    utime_t t = ceph_clock_now(g_ceph_context) - ts;
    dout(2) << "req " << id << ":" << t << ":" << s->dialect << ":" << req_str << ":" << (op ? op->name() : "") << ":" << msg << dendl;
  }
};

class RGWFrontendConfig {
  string config;
  map<string, string> config_map;
  int parse_config(const string& config, map<string, string>& config_map);
  string framework;
public:
  RGWFrontendConfig(const string& _conf) : config(_conf) {}
  int init() {
    int ret = parse_config(config, config_map);
    if (ret < 0)
      return ret;
    return 0;
  }
  bool get_val(const string& key, const string& def_val, string *out);
  bool get_val(const string& key, int def_val, int *out);

  map<string, string>& get_config_map() { return config_map; }

  string get_framework() { return framework; }
};


struct RGWFCGXRequest : public RGWRequest {
  FCGX_Request *fcgx;
  QueueRing<FCGX_Request *> *qr;

  RGWFCGXRequest(uint64_t req_id, QueueRing<FCGX_Request *> *_qr) : RGWRequest(req_id), qr(_qr) {
    qr->dequeue(&fcgx);
  }

  ~RGWFCGXRequest() {
    FCGX_Finish_r(fcgx);
    qr->enqueue(fcgx);
  }
};

struct RGWProcessEnv {
  RGWRados *store;
  RGWREST *rest;
  OpsLogSocket *olog;
  int port;
};

class RGWProcess {
  deque<RGWRequest *> m_req_queue;
protected:
  RGWRados *store;
  OpsLogSocket *olog;
  ThreadPool m_tp;
  Throttle req_throttle;
  RGWREST *rest;
  RGWFrontendConfig *conf;
  int sock_fd;

  struct RGWWQ : public ThreadPool::WorkQueue<RGWRequest> {
    RGWProcess *process;
    RGWWQ(RGWProcess *p, time_t timeout, time_t suicide_timeout, ThreadPool *tp)
      : ThreadPool::WorkQueue<RGWRequest>("RGWWQ", timeout, suicide_timeout, tp), process(p) {}

    bool _enqueue(RGWRequest *req) {
      process->m_req_queue.push_back(req);
      perfcounter->inc(l_rgw_qlen);
      dout(20) << "enqueued request req=" << hex << req << dec << dendl;
      _dump_queue();
      return true;
    }
    void _dequeue(RGWRequest *req) {
      assert(0);
    }
    bool _empty() {
      return process->m_req_queue.empty();
    }
    RGWRequest *_dequeue() {
      if (process->m_req_queue.empty())
	return NULL;
      RGWRequest *req = process->m_req_queue.front();
      process->m_req_queue.pop_front();
      dout(20) << "dequeued request req=" << hex << req << dec << dendl;
      _dump_queue();
      perfcounter->inc(l_rgw_qlen, -1);
      return req;
    }
    void _process(RGWRequest *req) {
      perfcounter->inc(l_rgw_qactive);
      process->handle_request(req);
      process->req_throttle.put(1);
      perfcounter->inc(l_rgw_qactive, -1);
    }
    void _dump_queue() {
      if (!g_conf->subsys.should_gather(ceph_subsys_rgw, 20)) {
        return;
      }
      deque<RGWRequest *>::iterator iter;
      if (process->m_req_queue.empty()) {
        dout(20) << "RGWWQ: empty" << dendl;
        return;
      }
      dout(20) << "RGWWQ:" << dendl;
      for (iter = process->m_req_queue.begin(); iter != process->m_req_queue.end(); ++iter) {
        dout(20) << "req: " << hex << *iter << dec << dendl;
      }
    }
    void _clear() {
      assert(process->m_req_queue.empty());
    }
  } req_wq;

public:
  RGWProcess(CephContext *cct, RGWProcessEnv *pe, int num_threads, RGWFrontendConfig *_conf)
    : store(pe->store), olog(pe->olog), m_tp(cct, "RGWProcess::m_tp", num_threads),
      req_throttle(cct, "rgw_ops", num_threads * 2),
      rest(pe->rest),
      conf(_conf),
      sock_fd(-1),
      req_wq(this, g_conf->rgw_op_thread_timeout,
	     g_conf->rgw_op_thread_suicide_timeout, &m_tp) {}
  virtual ~RGWProcess() {}
  virtual void run() = 0;
  virtual void handle_request(RGWRequest *req) = 0;

  void close_fd() {
    if (sock_fd >= 0) {
      ::close(sock_fd);
      sock_fd = -1;
    }
  }
};


class RGWFCGXProcess : public RGWProcess {
  int max_connections;
public:
  RGWFCGXProcess(CephContext *cct, RGWProcessEnv *pe, int num_threads, RGWFrontendConfig *_conf) :
    RGWProcess(cct, pe, num_threads, _conf),
    max_connections(num_threads + (num_threads >> 3)) /* have a bit more connections than threads so that requests
                                                       are still accepted even if we're still processing older requests */
    {}
  void run();
  void handle_request(RGWRequest *req);
};

void RGWFCGXProcess::run()
{
  string socket_path;
  string socket_port;
  string socket_host;

  conf->get_val("socket_path", "", &socket_path);
  conf->get_val("socket_port", g_conf->rgw_port, &socket_port);
  conf->get_val("socket_host", g_conf->rgw_host, &socket_host);

  if (socket_path.empty() && socket_port.empty() && socket_host.empty()) {
    socket_path = g_conf->rgw_socket_path;
    if (socket_path.empty()) {
      dout(0) << "ERROR: no socket server point defined, cannot start fcgi frontend" << dendl;
      return;
    }
  }

  if (!socket_path.empty()) {
    string path_str = socket_path;

    /* this is necessary, as FCGX_OpenSocket might not return an error, but rather ungracefully exit */
    int fd = open(path_str.c_str(), O_CREAT, 0644);
    if (fd < 0) {
      int err = errno;
      /* ENXIO is actually expected, we'll get that if we try to open a unix domain socket */
      if (err != ENXIO) {
        dout(0) << "ERROR: cannot create socket: path=" << path_str << " error=" << cpp_strerror(err) << dendl;
        return;
      }
    } else {
      close(fd);
    }

    const char *path = path_str.c_str();
    sock_fd = FCGX_OpenSocket(path, SOCKET_BACKLOG);
    if (sock_fd < 0) {
      dout(0) << "ERROR: FCGX_OpenSocket (" << path << ") returned " << sock_fd << dendl;
      return;
    }
    if (chmod(path, 0777) < 0) {
      dout(0) << "WARNING: couldn't set permissions on unix domain socket" << dendl;
    }
  } else if (!socket_port.empty()) {
    string bind = socket_host + ":" + socket_port;
    sock_fd = FCGX_OpenSocket(bind.c_str(), SOCKET_BACKLOG);
    if (sock_fd < 0) {
      dout(0) << "ERROR: FCGX_OpenSocket (" << bind.c_str() << ") returned " << sock_fd << dendl;
      return;
    }
  }

  m_tp.start();

  FCGX_Request fcgx_reqs[max_connections];

  QueueRing<FCGX_Request *> qr(max_connections);
  for (int i = 0; i < max_connections; i++) {
    FCGX_Request *fcgx = &fcgx_reqs[i];
    FCGX_InitRequest(fcgx, sock_fd, 0);
    qr.enqueue(fcgx);
  }

  for (;;) {
    RGWFCGXRequest *req = new RGWFCGXRequest(store->get_new_req_id(), &qr);
    dout(10) << "allocated request req=" << hex << req << dec << dendl;
    req_throttle.get(1);
    int ret = FCGX_Accept_r(req->fcgx);
    if (ret < 0) {
      delete req;
      dout(0) << "ERROR: FCGX_Accept_r returned " << ret << dendl;
      req_throttle.put(1);
      break;
    }

    req_wq.queue(req);
  }

  m_tp.drain(&req_wq);
  m_tp.stop();

  dout(20) << "cleaning up fcgx connections" << dendl;

  for (int i = 0; i < max_connections; i++) {
    FCGX_Finish_r(&fcgx_reqs[i]);
  }
}

struct RGWLoadGenRequest : public RGWRequest {
  string method;
  string resource;
  int content_length;
  atomic_t *fail_flag;


  RGWLoadGenRequest(uint64_t req_id, const string& _m, const  string& _r, int _cl,
                    atomic_t *ff) : RGWRequest(req_id), method(_m), resource(_r), content_length(_cl), fail_flag(ff) {}
};

class RGWLoadGenProcess : public RGWProcess {
  RGWAccessKey access_key;
public:
  RGWLoadGenProcess(CephContext *cct, RGWProcessEnv *pe, int num_threads, RGWFrontendConfig *_conf) :
    RGWProcess(cct, pe, num_threads, _conf) {}
  void run();
  void checkpoint();
  void handle_request(RGWRequest *req);
  void gen_request(const string& method, const string& resource, int content_length, atomic_t *fail_flag);

  void set_access_key(RGWAccessKey& key) { access_key = key; }
};

void RGWLoadGenProcess::checkpoint()
{
  m_tp.drain(&req_wq);
}

void RGWLoadGenProcess::run()
{
  m_tp.start(); /* start thread pool */

  int i;

  int num_objs;

  conf->get_val("num_objs", 1000, &num_objs);

  int num_buckets;
  conf->get_val("num_buckets", 1, &num_buckets);

  vector<string> buckets(num_buckets);

  atomic_t failed;

  for (i = 0; i < num_buckets; i++) {
    buckets[i] = "/loadgen";
    string& bucket = buckets[i];
    append_rand_alpha(NULL, bucket, bucket, 16);

    /* first create a bucket */
    gen_request("PUT", bucket, 0, &failed);
    checkpoint();
  }

  string *objs = new string[num_objs];

  if (failed.read()) {
    derr << "ERROR: bucket creation failed" << dendl;
    goto done;
  }

  for (i = 0; i < num_objs; i++) {
    char buf[16 + 1];
    gen_rand_alphanumeric(NULL, buf, sizeof(buf));
    buf[16] = '\0';
    objs[i] = buckets[i % num_buckets] + "/" + buf;
  }

  for (i = 0; i < num_objs; i++) {
    gen_request("PUT", objs[i], 4096, &failed);
  }

  checkpoint();

  if (failed.read()) {
    derr << "ERROR: bucket creation failed" << dendl;
    goto done;
  }

  for (i = 0; i < num_objs; i++) {
    gen_request("GET", objs[i], 4096, NULL);
  }

  checkpoint();

  for (i = 0; i < num_objs; i++) {
    gen_request("DELETE", objs[i], 0, NULL);
  }

  checkpoint();

  for (i = 0; i < num_buckets; i++) {
    gen_request("DELETE", buckets[i], 0, NULL);
  }

done:
  checkpoint();

  m_tp.stop();

  delete[] objs;

  signal_shutdown();
}

void RGWLoadGenProcess::gen_request(const string& method, const string& resource, int content_length, atomic_t *fail_flag)
{
  RGWLoadGenRequest *req = new RGWLoadGenRequest(store->get_new_req_id(), method, resource,
						 content_length, fail_flag);
  dout(10) << "allocated request req=" << hex << req << dec << dendl;
  req_throttle.get(1);
  req_wq.queue(req);
}

static void signal_shutdown()
>>>>>>> upstream/hammer
{
  if (!disable_signal_fd.read()) {
    int val = 0;
    int ret = write(signal_fd[0], (char *)&val, sizeof(val));
    if (ret < 0) {
      derr << "ERROR: " << __func__ << ": write() returned "
           << cpp_strerror(errno) << dendl;
    }
  }
}

static void wait_shutdown()
{
  int val;
  int r = safe_read_exact(signal_fd[1], &val, sizeof(val));
  if (r < 0) {
    derr << "safe_read_exact returned with error" << dendl;
  }
}

static int signal_fd_init()
{
  return socketpair(AF_UNIX, SOCK_STREAM, 0, signal_fd);
}

static void signal_fd_finalize()
{
  close(signal_fd[0]);
  close(signal_fd[1]);
}

static void handle_sigterm(int signum)
{
  dout(1) << __func__ << dendl;
#if defined(WITH_RADOSGW_FCGI_FRONTEND)
  FCGX_ShutdownPending();
#endif

  // send a signal to make fcgi's accept(2) wake up.  unfortunately the
  // initial signal often isn't sufficient because we race with accept's
  // check of the flag wet by ShutdownPending() above.
  if (signum != SIGUSR1) {
    signal_shutdown();

    // safety net in case we get stuck doing an orderly shutdown.
    uint64_t secs = g_ceph_context->_conf->rgw_exit_timeout_secs;
    if (secs)
      alarm(secs);
    dout(1) << __func__ << " set alarm for " << secs << dendl;
  }

}

static void godown_alarm(int signum)
{
  _exit(0);
}

<<<<<<< HEAD
=======
static int process_request(RGWRados *store, RGWREST *rest, RGWRequest *req, RGWClientIO *client_io, OpsLogSocket *olog)
{
  int ret = 0;

  client_io->init(g_ceph_context);

  req->log_init();

  dout(1) << "====== starting new request req=" << hex << req << dec << " =====" << dendl;
  perfcounter->inc(l_rgw_req);

  RGWEnv& rgw_env = client_io->get_env();

  struct req_state rstate(g_ceph_context, &rgw_env);

  struct req_state *s = &rstate;

  RGWObjectCtx rados_ctx(store, s);
  s->obj_ctx = &rados_ctx;

  s->req_id = store->unique_id(req->id);
  s->trans_id = store->unique_trans_id(req->id);

  req->log_format(s, "initializing for trans_id = %s", s->trans_id.c_str());

  RGWOp *op = NULL;
  int init_error = 0;
  bool should_log = false;
  RGWRESTMgr *mgr;
  RGWHandler *handler = rest->get_handler(store, s, client_io, &mgr, &init_error);
  if (init_error != 0) {
    abort_early(s, NULL, init_error);
    goto done;
  }

  should_log = mgr->get_logging();

  req->log(s, "getting op");
  op = handler->get_op(store);
  if (!op) {
    abort_early(s, NULL, -ERR_METHOD_NOT_ALLOWED);
    goto done;
  }
  req->op = op;

  req->log(s, "authorizing");
  ret = handler->authorize();
  if (ret < 0) {
    dout(10) << "failed to authorize request" << dendl;
    abort_early(s, op, ret);
    goto done;
  }

  if (s->user.suspended) {
    dout(10) << "user is suspended, uid=" << s->user.user_id << dendl;
    abort_early(s, op, -ERR_USER_SUSPENDED);
    goto done;
  }
  req->log(s, "reading permissions");
  ret = handler->read_permissions(op);
  if (ret < 0) {
    abort_early(s, op, ret);
    goto done;
  }

  req->log(s, "init op");
  ret = op->init_processing();
  if (ret < 0) {
    abort_early(s, op, ret);
    goto done;
  }

  req->log(s, "verifying op mask");
  ret = op->verify_op_mask();
  if (ret < 0) {
    abort_early(s, op, ret);
    goto done;
  }

  req->log(s, "verifying op permissions");
  ret = op->verify_permission();
  if (ret < 0) {
    if (s->system_request) {
      dout(2) << "overriding permissions due to system operation" << dendl;
    } else {
      abort_early(s, op, ret);
      goto done;
    }
  }

  req->log(s, "verifying op params");
  ret = op->verify_params();
  if (ret < 0) {
    abort_early(s, op, ret);
    goto done;
  }

  req->log(s, "executing");
  op->pre_exec();
  op->execute();
  op->complete();
done:
  int r = client_io->complete_request();
  if (r < 0) {
    dout(0) << "ERROR: client_io->complete_request() returned " << r << dendl;
  }
  if (should_log) {
    rgw_log_op(store, s, (op ? op->name() : "unknown"), olog);
  }

  int http_ret = s->err.http_ret;

  req->log_format(s, "http status=%d", http_ret);

  if (handler)
    handler->put_op(op);
  rest->put_handler(handler);

  dout(1) << "====== req done req=" << hex << req << dec << " http_status=" << http_ret << " ======" << dendl;

  return (ret < 0 ? ret : s->err.ret);
}

void RGWFCGXProcess::handle_request(RGWRequest *r)
{
  RGWFCGXRequest *req = static_cast<RGWFCGXRequest *>(r);
  FCGX_Request *fcgx = req->fcgx;
  RGWFCGX client_io(fcgx);

 
  int ret = process_request(store, rest, req, &client_io, olog);
  if (ret < 0) {
    /* we don't really care about return code */
    dout(20) << "process_request() returned " << ret << dendl;
  }

  FCGX_Finish_r(fcgx);

  delete req;
}

void RGWLoadGenProcess::handle_request(RGWRequest *r)
{
  RGWLoadGenRequest *req = static_cast<RGWLoadGenRequest *>(r);

  RGWLoadGenRequestEnv env;

  utime_t tm = ceph_clock_now(NULL);

  env.port = 80;
  env.content_length = req->content_length;
  env.content_type = "binary/octet-stream";
  env.request_method = req->method;
  env.uri = req->resource;
  env.set_date(tm);
  env.sign(access_key);

  RGWLoadGenIO client_io(&env);

  int ret = process_request(store, rest, req, &client_io, olog);
  if (ret < 0) {
    /* we don't really care about return code */
    dout(20) << "process_request() returned " << ret << dendl;

    if (req->fail_flag) {
      req->fail_flag->inc();
    }
  }

  delete req;
}


static int civetweb_callback(struct mg_connection *conn) {
  struct mg_request_info *req_info = mg_get_request_info(conn);
  RGWProcessEnv *pe = static_cast<RGWProcessEnv *>(req_info->user_data);
  RGWRados *store = pe->store;
  RGWREST *rest = pe->rest;
  OpsLogSocket *olog = pe->olog;

  RGWRequest *req = new RGWRequest(store->get_new_req_id());
  RGWMongoose client_io(conn, pe->port);

  client_io.init(g_ceph_context);


  int ret = process_request(store, rest, req, &client_io, olog);
  if (ret < 0) {
    /* we don't really care about return code */
    dout(20) << "process_request() returned " << ret << dendl;
  }

  delete req;

// Mark as processed
  return 1;
}

>>>>>>> upstream/hammer
#ifdef HAVE_CURL_MULTI_WAIT
static void check_curl()
{
}
#else
static void check_curl()
{
  derr << "WARNING: libcurl doesn't support curl_multi_wait()" << dendl;
  derr << "WARNING: cross zone / region transfer performance may be affected" << dendl;
}
#endif

class C_InitTimeout : public Context {
public:
  C_InitTimeout() {}
  void finish(int r) override {
    derr << "Initialization timeout, failed to initialize" << dendl;
    exit(1);
  }
};

static int usage()
{
  cerr << "usage: radosgw [options...]" << std::endl;
  cerr << "options:\n";
  cerr << "  --rgw-region=<region>     region in which radosgw runs\n";
  cerr << "  --rgw-zone=<zone>         zone in which radosgw runs\n";
  cerr << "  --rgw-socket-path=<path>  specify a unix domain socket path\n";
  cerr << "  -m monaddress[:port]      connect to specified monitor\n";
  cerr << "  --keyring=<path>          path to radosgw keyring\n";
  cerr << "  --logfile=<logfile>       file to log debug output\n";
  cerr << "  --debug-rgw=<log-level>/<memory-level>  set radosgw debug level\n";
  generic_server_usage();

  return 0;
}

static RGWRESTMgr *set_logging(RGWRESTMgr *mgr)
{
  mgr->set_logging(true);
  return mgr;
}

RGWRealmReloader *preloader = NULL;

static void reloader_handler(int signum)
{
  if (preloader) {
    bufferlist bl;
    bufferlist::iterator p = bl.begin();
    preloader->handle_notify(RGWRealmNotify::Reload, p);
  }
  sighup_handler(signum);
}

/*
 * start up the RADOS connection and then handle HTTP messages as they come in
 */
#ifdef BUILDING_FOR_EMBEDDED
extern "C" int cephd_rgw(int argc, const char **argv)
#else
int main(int argc, const char **argv)
#endif
{
  // dout() messages will be sent to stderr, but FCGX wants messages on stdout
  // Redirect stderr to stdout.
  TEMP_FAILURE_RETRY(close(STDERR_FILENO));
  if (TEMP_FAILURE_RETRY(dup2(STDOUT_FILENO, STDERR_FILENO)) < 0) {
    int err = errno;
    cout << "failed to redirect stderr to stdout: " << cpp_strerror(err)
         << std::endl;
    return ENOSYS;
  }

  /* alternative default for module */
  vector<const char *> def_args;
  def_args.push_back("--debug-rgw=1/5");
  def_args.push_back("--keyring=$rgw_data/keyring");

  vector<const char*> args;
  argv_to_vec(argc, argv, args);
  env_to_vec(args);

  // First, let's determine which frontends are configured.
  int flags = CINIT_FLAG_UNPRIVILEGED_DAEMON_DEFAULTS;
  global_pre_init(&def_args, args, CEPH_ENTITY_TYPE_CLIENT, CODE_ENVIRONMENT_DAEMON,
          flags);

  list<string> frontends;
  get_str_list(g_conf->rgw_frontends, ",", frontends);
  multimap<string, RGWFrontendConfig *> fe_map;
  list<RGWFrontendConfig *> configs;
  if (frontends.empty()) {
    frontends.push_back("fastcgi");
  }
  for (list<string>::iterator iter = frontends.begin(); iter != frontends.end(); ++iter) {
    string& f = *iter;

    if (f.find("civetweb") != string::npos) {
      // If civetweb is configured as a frontend, prevent global_init() from
      // dropping permissions by setting the appropriate flag.
      flags |= CINIT_FLAG_DEFER_DROP_PRIVILEGES;
      if (f.find("port") != string::npos) {
        // check for the most common ws problems
        if ((f.find("port=") == string::npos) ||
            (f.find("port= ") != string::npos)) {
          derr << "WARNING: civetweb frontend config found unexpected spacing around 'port' "
               << "(ensure civetweb port parameter has the form 'port=80' with no spaces "
               << "before or after '=')" << dendl;
        }
      }
    }

    RGWFrontendConfig *config = new RGWFrontendConfig(f);
    int r = config->init();
    if (r < 0) {
      delete config;
      cerr << "ERROR: failed to init config: " << f << std::endl;
      return EINVAL;
    }

    configs.push_back(config);

    string framework = config->get_framework();
    fe_map.insert(pair<string, RGWFrontendConfig*>(framework, config));
  }

  // Now that we've determined which frontend(s) to use, continue with global
  // initialization. Passing false as the final argument ensures that
  // global_pre_init() is not invoked twice.
  // claim the reference and release it after subsequent destructors have fired
  auto cct = global_init(&def_args, args, CEPH_ENTITY_TYPE_CLIENT,
			 CODE_ENVIRONMENT_DAEMON,
			 flags, "rgw_data", false);

  for (std::vector<const char*>::iterator i = args.begin(); i != args.end(); ++i) {
    if (ceph_argparse_flag(args, i, "-h", "--help", (char*)NULL)) {
      usage();
      return 0;
    }
  }

  // maintain existing region root pool for new multisite objects
  if (!g_conf->rgw_region_root_pool.empty()) {
    const char *root_pool = g_conf->rgw_region_root_pool.c_str();
    if (g_conf->rgw_zonegroup_root_pool.empty()) {
      g_conf->set_val_or_die("rgw_zonegroup_root_pool", root_pool);
    }
    if (g_conf->rgw_period_root_pool.empty()) {
      g_conf->set_val_or_die("rgw_period_root_pool", root_pool);
    }
    if (g_conf->rgw_realm_root_pool.empty()) {
      g_conf->set_val_or_die("rgw_realm_root_pool", root_pool);
    }
  }

  // for region -> zonegroup conversion (must happen before common_init_finish())
  if (!g_conf->rgw_region.empty() && g_conf->rgw_zonegroup.empty()) {
    g_conf->set_val_or_die("rgw_zonegroup", g_conf->rgw_region.c_str());
  }

  check_curl();

  if (g_conf->daemonize) {
    global_init_daemonize(g_ceph_context);
  }
  Mutex mutex("main");
  SafeTimer init_timer(g_ceph_context, mutex);
  init_timer.init();
  mutex.Lock();
  init_timer.add_event_after(g_conf->rgw_init_timeout, new C_InitTimeout);
  mutex.Unlock();

  // Enable the perf counter before starting the service thread
  g_ceph_context->enable_perf_counter();

  common_init_finish(g_ceph_context);

  int r = rgw_tools_init(g_ceph_context);
  if (r < 0) {
    derr << "ERROR: unable to initialize rgw tools" << dendl;
    return -r;
  }

  rgw_init_resolver();
  
  curl_global_init(CURL_GLOBAL_ALL);
  
#if defined(WITH_RADOSGW_FCGI_FRONTEND)
  FCGX_Init();
#endif

<<<<<<< HEAD
  RGWRados *store = RGWStoreManager::get_storage(g_ceph_context,
      g_conf->rgw_enable_gc_threads, g_conf->rgw_enable_lc_threads, g_conf->rgw_enable_quota_threads,
      g_conf->rgw_run_sync_thread);
=======
  int r = 0;
  RGWRados *store = RGWStoreManager::get_storage(g_ceph_context,
      g_conf->rgw_enable_gc_threads, g_conf->rgw_enable_quota_threads);
>>>>>>> upstream/hammer
  if (!store) {
    mutex.Lock();
    init_timer.cancel_all_events();
    init_timer.shutdown();
    mutex.Unlock();

    derr << "Couldn't init storage provider (RADOS)" << dendl;
    return EIO;
  }
  r = rgw_perf_start(g_ceph_context);
  if (r < 0) {
    derr << "ERROR: failed starting rgw perf" << dendl;
    return -r;
  }

  rgw_rest_init(g_ceph_context, store, store->get_zonegroup());

  mutex.Lock();
  init_timer.cancel_all_events();
  init_timer.shutdown();
  mutex.Unlock();

  rgw_user_init(store);
  rgw_bucket_init(store->meta_mgr);
  rgw_log_usage_init(g_ceph_context, store);

  RGWREST rest;

  list<string> apis;

  get_str_list(g_conf->rgw_enable_apis, apis);

  map<string, bool> apis_map;
  for (list<string>::iterator li = apis.begin(); li != apis.end(); ++li) {
    apis_map[*li] = true;
  }

  // S3 website mode is a specialization of S3
  const bool s3website_enabled = apis_map.count("s3website") > 0;
  // Swift API entrypoint could placed in the root instead of S3
  const bool swift_at_root = g_conf->rgw_swift_url_prefix == "/";
  if (apis_map.count("s3") > 0 || s3website_enabled) {
    if (! swift_at_root) {
      rest.register_default_mgr(set_logging(new RGWRESTMgr_S3(s3website_enabled)));
    } else {
      derr << "Cannot have the S3 or S3 Website enabled together with "
           << "Swift API placed in the root of hierarchy" << dendl;
      return EINVAL;
    }
  }

  if (apis_map.count("swift") > 0) {
    RGWRESTMgr_SWIFT* const swift_resource = new RGWRESTMgr_SWIFT;

    if (! g_conf->rgw_cross_domain_policy.empty()) {
      swift_resource->register_resource("crossdomain.xml",
                          set_logging(new RGWRESTMgr_SWIFT_CrossDomain));
    }

    swift_resource->register_resource("healthcheck",
                          set_logging(new RGWRESTMgr_SWIFT_HealthCheck));

    swift_resource->register_resource("info",
                          set_logging(new RGWRESTMgr_SWIFT_Info));

    if (! swift_at_root) {
      rest.register_resource(g_conf->rgw_swift_url_prefix,
                          set_logging(swift_resource));
    } else {
      if (store->get_zonegroup().zones.size() > 1) {
        derr << "Placing Swift API in the root of URL hierarchy while running"
             << " multi-site configuration requires another instance of RadosGW"
             << " with S3 API enabled!" << dendl;
      }

      rest.register_default_mgr(set_logging(swift_resource));
    }
  }

  if (apis_map.count("swift_auth") > 0) {
    rest.register_resource(g_conf->rgw_swift_auth_entry,
               set_logging(new RGWRESTMgr_SWIFT_Auth));
  }

  if (apis_map.count("admin") > 0) {
    RGWRESTMgr_Admin *admin_resource = new RGWRESTMgr_Admin;
    admin_resource->register_resource("usage", new RGWRESTMgr_Usage);
    admin_resource->register_resource("user", new RGWRESTMgr_User);
    admin_resource->register_resource("bucket", new RGWRESTMgr_Bucket);
  
    /*Registering resource for /admin/metadata */
    admin_resource->register_resource("metadata", new RGWRESTMgr_Metadata);
    admin_resource->register_resource("log", new RGWRESTMgr_Log);
    admin_resource->register_resource("opstate", new RGWRESTMgr_Opstate);
    admin_resource->register_resource("replica_log", new RGWRESTMgr_ReplicaLog);
    admin_resource->register_resource("config", new RGWRESTMgr_Config);
    admin_resource->register_resource("realm", new RGWRESTMgr_Realm);
    rest.register_resource(g_conf->rgw_admin_entry, admin_resource);
  }

  /* Header custom behavior */
  rest.register_x_headers(g_conf->rgw_log_http_headers);

  OpsLogSocket *olog = NULL;

  if (!g_conf->rgw_ops_log_socket_path.empty()) {
    olog = new OpsLogSocket(g_ceph_context, g_conf->rgw_ops_log_data_backlog);
    olog->init(g_conf->rgw_ops_log_socket_path);
  }

  r = signal_fd_init();
  if (r < 0) {
    derr << "ERROR: unable to initialize signal fds" << dendl;
    exit(1);
  }

  init_async_signal_handler();
  register_async_signal_handler(SIGHUP, reloader_handler);
  register_async_signal_handler(SIGTERM, handle_sigterm);
  register_async_signal_handler(SIGINT, handle_sigterm);
  register_async_signal_handler(SIGUSR1, handle_sigterm);
  sighandler_alrm = signal(SIGALRM, godown_alarm);

<<<<<<< HEAD
=======
  list<string> frontends;
  get_str_list(g_conf->rgw_frontends, ",", frontends);

  multimap<string, RGWFrontendConfig *> fe_map;
  list<RGWFrontendConfig *> configs;
  if (frontends.empty()) {
    frontends.push_back("fastcgi");
  }
  for (list<string>::iterator iter = frontends.begin(); iter != frontends.end(); ++iter) {
    string& f = *iter;

    if (f.find("civetweb") != string::npos) {
      if (f.find("port") != string::npos) {
	// check for the most common ws problems
	if ((f.find("port=") == string::npos) ||
	    (f.find("port= ") != string::npos)) {
	  derr << "WARNING: civetweb frontend config found unexpected spacing around 'port' (ensure civetweb port parameter has the form 'port=80' with no spaces before or after '=')" << dendl;
	}
      }
    }

    RGWFrontendConfig *config = new RGWFrontendConfig(f);
    int r = config->init();
    if (r < 0) {
      cerr << "ERROR: failed to init config: " << f << std::endl;
      return EINVAL;
    }

    configs.push_back(config);

    string framework = config->get_framework();
    fe_map.insert(pair<string, RGWFrontendConfig*>(framework, config));
  }

>>>>>>> upstream/hammer
  list<RGWFrontend *> fes;

  for (multimap<string, RGWFrontendConfig *>::iterator fiter = fe_map.begin();
       fiter != fe_map.end(); ++fiter) {
    RGWFrontendConfig *config = fiter->second;
    string framework = config->get_framework();
    RGWFrontend *fe = NULL;

    if (framework == "civetweb" || framework == "mongoose") {
      int port;
      config->get_val("port", 80, &port);
      std::string uri_prefix;
      config->get_val("prefix", "", &uri_prefix);

      RGWProcessEnv env = { store, &rest, olog, 0, uri_prefix };

      fe = new RGWCivetWebFrontend(env, config);
    }
    else if (framework == "loadgen") {
      int port;
      config->get_val("port", 80, &port);
      std::string uri_prefix;
      config->get_val("prefix", "", &uri_prefix);

      RGWProcessEnv env = { store, &rest, olog, port, uri_prefix };

      fe = new RGWLoadGenFrontend(env, config);
    }
#if defined(WITH_RADOSGW_ASIO_FRONTEND)
    else if ((framework == "asio") &&
	cct->check_experimental_feature_enabled("rgw-asio-frontend")) {
      int port;
      config->get_val("port", 80, &port);
      std::string uri_prefix;
      config->get_val("prefix", "", &uri_prefix);
      RGWProcessEnv env{ store, &rest, olog, port, uri_prefix };
      fe = new RGWAsioFrontend(env);
    }
#endif /* WITH_RADOSGW_ASIO_FRONTEND */
#if defined(WITH_RADOSGW_FCGI_FRONTEND)
    else if (framework == "fastcgi" || framework == "fcgi") {
      std::string uri_prefix;
      config->get_val("prefix", "", &uri_prefix);
      RGWProcessEnv fcgi_pe = { store, &rest, olog, 0, uri_prefix };

      fe = new RGWFCGXFrontend(fcgi_pe, config);
    }
#endif /* WITH_RADOSGW_FCGI_FRONTEND */

    if (fe == NULL) {
      dout(0) << "WARNING: skipping unknown framework: " << framework << dendl;
      continue;
    }

    dout(0) << "starting handler: " << fiter->first << dendl;
    int r = fe->init();
    if (r < 0) {
      derr << "ERROR: failed initializing frontend" << dendl;
      return -r;
    }
    r = fe->run();
    if (r < 0) {
      derr << "ERROR: failed run" << dendl;
      return -r;
    }

    fes.push_back(fe);
  }

  // add a watcher to respond to realm configuration changes
  RGWPeriodPusher pusher(store);
  RGWFrontendPauser pauser(fes, &pusher);
  RGWRealmReloader reloader(store, &pauser);

  preloader = &reloader;

  RGWRealmWatcher realm_watcher(g_ceph_context, store->realm);
  realm_watcher.add_watcher(RGWRealmNotify::Reload, reloader);
  realm_watcher.add_watcher(RGWRealmNotify::ZonesNeedPeriod, pusher);

  wait_shutdown();

  derr << "shutting down" << dendl;

  for (list<RGWFrontend *>::iterator liter = fes.begin(); liter != fes.end();
       ++liter) {
    RGWFrontend *fe = *liter;
    fe->stop();
  }

  for (list<RGWFrontend *>::iterator liter = fes.begin(); liter != fes.end();
       ++liter) {
    RGWFrontend *fe = *liter;
    fe->join();
    delete fe;
  }

  for (list<RGWFrontendConfig *>::iterator liter = configs.begin();
       liter != configs.end(); ++liter) {
    RGWFrontendConfig *fec = *liter;
    delete fec;
  }

  unregister_async_signal_handler(SIGHUP, reloader_handler);
  unregister_async_signal_handler(SIGTERM, handle_sigterm);
  unregister_async_signal_handler(SIGINT, handle_sigterm);
  unregister_async_signal_handler(SIGUSR1, handle_sigterm);
  shutdown_async_signal_handler();

  rgw_log_usage_finalize();

  delete olog;

  RGWStoreManager::close_storage(store);

  rgw_tools_cleanup();
  rgw_shutdown_resolver();
  curl_global_cleanup();

  rgw_perf_stop(g_ceph_context);

  dout(1) << "final shutdown" << dendl;
<<<<<<< HEAD
=======
  g_ceph_context->put();
>>>>>>> upstream/hammer

  signal_fd_finalize();

  return 0;
}
