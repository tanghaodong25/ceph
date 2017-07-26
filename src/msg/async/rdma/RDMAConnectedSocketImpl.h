// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2016 XSKY <haomai@xsky.com>
 *
 * Author: Haomai Wang <haomaiwang@gmail.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef CEPH_MSG_RDMA_CONNECTED_SOCKET_IMPL_H
#define CEPH_MSG_RDMA_CONNECTED_SOCKET_IMPL_H

#include "common/ceph_context.h"
#include "common/debug.h"
#include "common/errno.h"
#include "msg/async/Stack.h"
#include "Infiniband.h"

class RDMAWorker;
class RDMADispatcher;
class RDMAConnectedSocketImpl;

typedef Infiniband::QueuePair QueuePair;

class RDMAConnMgr {
  friend class RDMAConnectedSocketImpl;

 private:
  atomic_t refs;

 protected:
  CephContext *cct;
  RDMAConnectedSocketImpl *socket;
  Infiniband* infiniband;
  RDMADispatcher* dispatcher;
  RDMAWorker* worker;

  Device *ibdev = nullptr;
  int ibport = -1;

  bool is_server;
  bool active;// qp is active ?
  int connected;

 public:
  RDMAConnMgr(CephContext *cct, RDMAConnectedSocketImpl *sock,
	      Infiniband* ib, RDMADispatcher* s, RDMAWorker *w, int r);
  virtual ~RDMAConnMgr() { };

  virtual void put();

  QueuePair* get_qp() { return qp; }

  virtual ostream &print(ostream &out) const = 0;

  virtual void cleanup() = 0;
  virtual int try_connect(const entity_addr_t&, const SocketOptions &opt) = 0;
  virtual int activate();

  int create_queue_pair();

  virtual ibv_qp *qp_create(ibv_pd *pd, ibv_qp_init_attr *qpia) = 0;
  virtual void qp_to_err() = 0;
  virtual void qp_destroy() = 0;

  void post_read();

  virtual void shutdown();
  void close();

  virtual void set_orphan() { socket = nullptr; put(); };
  QueuePair *qp = nullptr;
};
inline ostream& operator<<(ostream& out, const RDMAConnMgr &m)
{
    return m.print(out);
}

class RDMAConnectedSocketImpl : public ConnectedSocketImpl {
  friend class RDMAConnMgr;

 protected:
  CephContext *cct;
  Infiniband* infiniband;
  RDMADispatcher* dispatcher;
  RDMAWorker* worker;

 public:
  typedef Infiniband::MemoryManager::Chunk Chunk;
  typedef Infiniband::CompletionChannel CompletionChannel;
  typedef Infiniband::CompletionQueue CompletionQueue;

 private:
  RDMAConnMgr *cmgr;
  std::vector<Chunk*> buffers;
  int notify_fd = -1;
  bufferlist pending_bl;

  Mutex lock;
  std::vector<ibv_wc> wc;

  ssize_t read_buffers(char* buf, size_t len);
  int post_work_request(std::vector<Chunk*>&);

 public:
  int error;
  uint32_t local_qpn = 0;
  uint32_t remote_qpn = 0;

  RDMAConnectedSocketImpl(CephContext *cct, Infiniband* ib, RDMADispatcher* s,
                          RDMAWorker *w, void *info = nullptr);
  virtual ~RDMAConnectedSocketImpl();
  
  RDMAConnMgr* get_cmgr() { return cmgr; }
  ostream &print(ostream &out) const {
    out << "socket {lqpn: " << local_qpn << " rqpn: " << remote_qpn << " ";
    if (cmgr)
      out << *cmgr;
    else
      out << "cmgr=NULL";
    out << "}";

    return out;
  };

  Device *get_device() { return cmgr->ibdev; };

  void pass_wc(std::vector<ibv_wc> &&v);
  void get_wc(std::vector<ibv_wc> &w);
  virtual int is_connected() override { return cmgr->connected; }

  virtual ssize_t read(char* buf, size_t len) override;
  virtual ssize_t zero_copy_read(bufferptr &data) override;
  virtual ssize_t send(bufferlist &bl, bool more) override;
  virtual void shutdown() override { cmgr->shutdown(); };
  virtual void close() override { cmgr->close(); };
  virtual int fd() const override { return notify_fd; }
  void fault();
  ssize_t submit(bool more);
  void fin();
  void abort_connection();
  int get_gid_idx();
  void register_qp(QueuePair *qp);
  void notify();

  int try_connect(const entity_addr_t &sa, const SocketOptions &opt) { return cmgr->try_connect(sa, opt); };
};
inline ostream& operator<<(ostream& out, const RDMAConnectedSocketImpl &s)
{
  return s.print(out);
}


class RDMAServerSocketImpl : public ServerSocketImpl {
 protected:
  CephContext *cct;
  Device *ibdev;
  int ibport;
  Infiniband* infiniband;
  RDMADispatcher *dispatcher;
  RDMAWorker *worker;
  entity_addr_t sa;

 public:
  RDMAServerSocketImpl(CephContext *cct, Infiniband* i, RDMADispatcher *s, RDMAWorker *w, entity_addr_t& a);

  static RDMAServerSocketImpl *factory(CephContext *cct, Infiniband *ib,
                                       RDMADispatcher *s, RDMAWorker *w,
                                       entity_addr_t& a);

  virtual int listen(entity_addr_t &sa, const SocketOptions &opt) = 0;
  virtual int accept(ConnectedSocket *s, const SocketOptions &opts, entity_addr_t *out, Worker *w) = 0;
  virtual void abort_accept() = 0;
  virtual int fd() const = 0;
};

#endif

