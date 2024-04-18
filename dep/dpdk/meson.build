# SPDX-License-Identifier: Marvell-MIT
# Copyright (c) 2023 Marvell.

dpdk_version = []
libdpdk_dep = dependency('libdpdk', method : 'pkg-config', required:false)
if not libdpdk_dep.found()
	error('Missing mandatory dependency "dpdk"')
endif

dpdk_version = libdpdk_dep.version()
if libdpdk_dep.version().version_compare('<23.11')
	error('DPDK version ' + dpdk_version +  ' not supported')
endif

DAO_BUILD_CONF.set('DAO_LIBDPDK_DEP', '1')
DAO_DPDK_LIB_DEPS += libdpdk_dep
