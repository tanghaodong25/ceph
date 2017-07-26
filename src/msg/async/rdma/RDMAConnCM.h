// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2017 Mellanox Ltd.
 *
 * Author: Amir Vadai <amirva@mellanox.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef CEPH_MSG_RDMA_CONNECTED_SOCKET_CM_H
#define CEPH_MSG_RDMA_CONNECTED_SOCKET_CM_H

#include "common/ceph_context.h"
#include "common/debug.h"
#include "common/errno.h"
#include "msg/async/Stack.h"
#include "Infiniband.h"
#include "RDMAConnectedSocketImpl.h"
#include "Device.h"

#include <queue>

struct RDMAConnCMInfo {
  struct rdma_cm_id *id;
  uint32_t remote_qpn;
};

class RDMAConnCM : public RDMAConnMgr {
  class C_handle_cm_event : public EventCallback {
    RDMAConnCM *cmgr;
  public:
    C_handle_cm_event(RDMAConnCM *cmgr): cmgr(cmgr) {}
    void do_request(int fd) {
      cmgr->handle_cm_event();
    }
  };

public:
  RDMAConnCM(CephContext *cct, RDMAConnectedSocketImpl *sock,
	     Infiniband *ib, RDMADispatcher* s, RDMAWorker *w,
	     void *info);
  virtual ~RDMAConnCM();

  virtual int try_connect(const entity_addr_t&, const SocketOptions &opt) override;
  int alloc_resources();
  virtual void shutdown() override;

  virtual void cleanup() override;

  virtual ostream &print(ostream &o) const override;

  void handle_cm_event();
  virtual ibv_qp *qp_create(ibv_pd *pd, ibv_qp_init_attr *qpia) override;
  virtual void qp_to_err() override;
  virtual void qp_destroy() override;

private:
  int create_channel(); void cm_established(uint32_t qpn);

  struct rdma_cm_id *id;

  rdma_event_channel *channel;

  EventCallbackRef con_handler;
};


class RDMAServerConnCM : public RDMAServerSocketImpl {
  struct rdma_event_channel *channel;
  struct rdma_cm_id *listen_id;

 public:
  RDMAServerConnCM(CephContext *cct, Infiniband *ib, RDMADispatcher *s, RDMAWorker *w, entity_addr_t& a);
  ~RDMAServerConnCM();

  int listen(entity_addr_t &sa, const SocketOptions &opt);
  virtual int accept(ConnectedSocket *s, const SocketOptions &opts, entity_addr_t *out, Worker *w) override;
  virtual void abort_accept() override;
  virtual int fd() const override;
  void handle_cm_event();
};

#endif
