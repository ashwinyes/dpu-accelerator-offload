/* SPDX-License-Identifier: Marvell-MIT
 * Copyright (c) 2023 Marvell.
 */

#ifndef __OOD_ETH_INIT_H__
#define __OOD_ETH_INIT_H__

#include <rte_ethdev.h>
#include <rte_mempool.h>

#define OOD_MAX_JUMBO_PKT_LEN  9600
#define OOD_MEMPOOL_CACHE_SIZE 256

#define OOD_MAX_PKT_BURST      32
#define OOD_BURST_TX_DRAIN_US  100 /* TX drain every ~100us */
#define OOD_MEMPOOL_CACHE_SIZE 256

/*
 * Configurable number of RX/TX ring descriptors
 */
#define OOD_RX_DESC_DEFAULT 1024
#define OOD_TX_DESC_DEFAULT 1024

#define OOD_NB_SOCKETS 1
/*
 * This expression is used to calculate the number of mbufs needed
 * depending on user input, taking  into account memory for rx and
 * tx hardware rings, cache per lcore and mtable per port per lcore.
 * RTE_MAX is used to ensure that OOD_NB_MBUF never goes below a minimum
 * value of 8192
 */
#define OOD_NB_MBUF(nports)                                                                        \
	RTE_MAX(((nports) * nb_rx_queue * nb_rxd + (nports) * n_tx_queue * nb_txd +                \
		 nb_lcores * OOD_MEMPOOL_CACHE_SIZE),                                              \
		8192u)

/* Forward declaration */
struct ood_main_cfg_data;

typedef struct ood_ethdev_param {
	int numa_on; /**< NUMA is enabled by default. */
	struct rte_mempool *pktmbuf_pool[RTE_MAX_ETHPORTS][OOD_NB_SOCKETS];
	/* list of enabled ports */
	uint16_t dest_port_tbl_lkp[RTE_MAX_ETHPORTS];
	uint16_t hw_func[RTE_MAX_ETHPORTS];
	uint16_t nb_ports;
} ood_ethdev_param_t;

int ood_ethdev_init(struct ood_main_cfg_data *ood_main_cfg);

#endif /* __OOD_ETH_INIT_H__ */
