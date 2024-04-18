..  SPDX-License-Identifier: Marvell-MIT
    Copyright (c) 2023 Marvell.

Compilation
===========
DPU Accelerator Offloads(DAO) applications needs to be cross compiled for arm64.

Dependent Libraries
-------------------
Following are some external library dependencies.

- libdpdk (mandatory)

  OVS-offload is a DPDK based application which requires DPDK library

Following are the example steps to build and install DPDK for CN9K

.. code-block:: console

  # meson setup --cross=config/arm/arm64_cn9k_linux_gcc --prefix=/path/to/dpdk/build/prefix/ build
  # ninja -C build install

Following are the example steps to build and install DPDK for CN10K

.. code-block:: console

  # meson setup --cross=config/arm/arm64_cn10k_linux_gcc --prefix=/path/to/dpdk/build/prefix/ build
  # ninja -C build install


CN9K cross compilation
----------------------
Setup the toolchain and follow the below steps.

.. code-block:: console

 # PKG_CONFIG_LIBDIR=/path/to/dpdk/build/prefix/lib/pkgconfig/ meson setup --cross config/arm64_cn9k_linux_gcc build
 # ninja -C build

CN10K cross compilation
-----------------------
Setup the toolchain and follow the below steps.

.. code-block:: console

 # PKG_CONFIG_LIBDIR=/path/to/dpdk/build/prefix/lib/pkgconfig/ meson setup --cross config/arm64_cn10k_linux_gcc build
 # ninja -C build

.. note::

 To link dpdk library statically, meson option ``--prefer-static`` shall be
 used.

Compiling the documentation
---------------------------
Install ``sphinx-build`` package. If this utility is found in PATH then
documentation will be built by default.

Meson Options
-------------
 - kernel_dir: Path to the kernel for building kernel modules (octep_vdpa).
   Headers must be in $kernel_dir.
 - dma_stats: Enable DMA statistics for DAO library
 - virtio_debug: Enable virtio debug that perform descriptor validation, etc.
