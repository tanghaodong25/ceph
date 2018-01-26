#include "RDMAStack.h"

#define dout_subsys ceph_subsys_ms
#undef dout_prefix
#define dout_prefix *_dout << " RDMACMConnectedSocketImpl "

RDMACMConnectedSocketImpl::RDMACMConnectedSocketImpl(CephContext *cct, Infiniband* ib, RDMADispatcher* s,
						 RDMAWorker *w, RDMACMInfo *info) : RDMAConnectedSocketImpl(cct, ib, s, w, info), cm_con_handler(new C_handle_cm_connection(this))
{
  notify_fd = eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK);
  if (info) {
    cm_id = info->cm_id;
    cm_channel = info->cm_channel; 
    remote_qpn = info->qp_num;

    alloc_resource();
    local_qpn = qp->get_local_qp_number();
    my_msg.qpn = local_qpn;
    worker->center.submit_to(worker->center.get_id(), [this]() {
      worker->center.create_file_event(cm_channel->fd, EVENT_READABLE, cm_con_handler);
    }, false);
    is_server = true;
  } else {
    cm_channel = rdma_create_event_channel(); 
    rdma_create_id(cm_channel, &cm_id, NULL, RDMA_PS_TCP);
    is_server = false;
  }
}

RDMACMConnectedSocketImpl::~RDMACMConnectedSocketImpl() {
  ldout(cct, 20) << __func__ << " destruct." << dendl;
  std::unique_lock<std::mutex> l(close_mtx);
  close_condition.wait(l, [&] { return closed; });

  rdma_destroy_id(cm_id);
  rdma_destroy_event_channel(cm_channel);
}

int RDMACMConnectedSocketImpl::try_connect(const entity_addr_t& peer_addr, const SocketOptions &opts) {
  worker->center.create_file_event(cm_channel->fd, EVENT_READABLE, cm_con_handler);
  if (rdma_resolve_addr(cm_id, NULL, const_cast<struct sockaddr*>(peer_addr.get_sockaddr()), 5000)) {
    lderr(cct) << __func__ << " failed to resolve addr" << dendl; 
    return -1;
  }
  return 0;
}

void RDMACMConnectedSocketImpl::close() {
  error = ECONNRESET;
  active = false;
  rdma_disconnect(cm_id);
  close_notify();
}

void RDMACMConnectedSocketImpl::shutdown() {
  error = ECONNRESET;
  active = false;
}

void RDMACMConnectedSocketImpl::handle_cm_connection() {
  struct rdma_cm_event *event;
  rdma_get_cm_event(cm_channel, &event);
  ldout(cct, 20) << __func__ << " event name: " << rdma_event_str(event->event) << dendl;
  struct rdma_conn_param cm_params;
  switch(event->event) {
    case RDMA_CM_EVENT_ADDR_RESOLVED: 
      if (rdma_resolve_route(cm_id, 2000)) {
        lderr(cct) << __func__ << " failed to resolve rdma addr" << dendl;
        notify();
      }
      break;

    case RDMA_CM_EVENT_ROUTE_RESOLVED:
      alloc_resource();

      memset(&cm_params, 0, sizeof(cm_params));
      if (!qp) {
        lderr(cct) << __func__ << " failed to create qp" << dendl; 
        notify();
      }
      local_qpn = qp->get_local_qp_number();
      my_msg.qpn = local_qpn;
      
      cm_params.retry_count = 7;
      cm_params.qp_num = local_qpn;
      if (rdma_connect(cm_id, &cm_params)) {
        lderr(cct) << __func__ << " failed to connect remote rdma port" << dendl; 
        connected = -ECONNREFUSED;
        notify();
      }
      break;

    case RDMA_CM_EVENT_ESTABLISHED:
      if (!is_server) {
        remote_qpn = event->param.conn.qp_num;
        activate();
        notify();
      }
      break;

    case RDMA_CM_EVENT_ADDR_ERROR:
    case RDMA_CM_EVENT_ROUTE_ERROR:
    case RDMA_CM_EVENT_CONNECT_ERROR:
    case RDMA_CM_EVENT_UNREACHABLE:
    case RDMA_CM_EVENT_REJECTED:
      lderr(cct) << __func__ << " rdma connection rejected" << dendl;
      notify();
      break;

    case RDMA_CM_EVENT_DISCONNECTED:
      close_notify();
      if (!error) {
        error = ECONNRESET;
        notify();
      }
      break;

    case RDMA_CM_EVENT_DEVICE_REMOVAL:
      break;

    default:
      assert(0 == "unhandled event");
      break;
  }
  rdma_ack_cm_event(event);
}

uint32_t RDMACMConnectedSocketImpl::get_local_qpn() {
  return local_qpn;
}

void RDMACMConnectedSocketImpl::activate() {
  ldout(cct, 30) << __func__ << dendl;
  active = true;
  connected = 1;
}

void RDMACMConnectedSocketImpl::alloc_resource() {
  ldout(cct, 30) << __func__ << dendl;
  qp = infiniband->create_queue_pair(cct, dispatcher->get_tx_cq(), dispatcher->get_rx_cq(), IBV_QPT_RC, cm_id);
  dispatcher->register_qp(qp, this);
  dispatcher->perf_logger->inc(l_msgr_rdma_created_queue_pair);
  dispatcher->perf_logger->inc(l_msgr_rdma_active_queue_pair);
}

void RDMACMConnectedSocketImpl::close_notify() {
  ldout(cct, 30) << __func__ << dendl;
  worker->center.delete_file_event(cm_channel->fd, EVENT_READABLE);
  std::unique_lock<std::mutex> l(close_mtx);
  if (!closed) {
    closed = true;
    close_condition.notify_all();
  }
}
