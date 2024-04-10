## Process Retransmission
Main state machine
|   |   |
|---|---|
rx2retrans_release_upd (RELEASE) | [qpn, latest_acked_req] 
rx2retrans_req (RETRANS) | [qpn, psn] 
timter2retrans_req (RETRANS) | [qpn]
tx2retrans_insertRequest (INSERT) | [qpn, psn, opCode, localAddr, remote Addr, length]

## Retrans Pointer Table
Indexed by `qpn`, contains the pointer to the first (`head`) and last (`tail`) entry of the qpn in `Retrans Meta Table`
|   |   |
|---|---|
pointerReqFifo | [qpn, lock]
pointerUdpFifo | [qpn, entry]
pointerRspFifo | [entry]
entry | [head, tail, valid]

## Retrans Meta Table
Stores the actual request if retransmission is triggered. A linked list, indexed by entries in `Retrans Pointer Table` or the "previous" entry in the table. `next` is a pointer to the table, indicating the next request of the same qpn.
|   |   |
|---|---|
meta_upd_req | [idx, entry, write, append]
meta_rsp | [entry]
entry | [psn, next, opCode, localAddr, remoteAddr, length]

## Freelist Handler
A FIFO that stores all available index of `Retrans Meta Table`

# Transport Timer