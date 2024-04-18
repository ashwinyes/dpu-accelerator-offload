..  SPDX-License-Identifier: Marvell-MIT
    Copyright (c) 2024 Marvell.

************
Virtio-l2fwd
************

The ``dpdk-virtio-l2fwd`` EP application is a Dataplane development kit(DPDK) application that
allows to excercise virtio usecase of forwarding traffic between Virtio net device and
DPDK ethdev device. Virtio net device is emulated using ``virtio-net`` DAO library.
Application maps a ``virtio-net` device to a ``rte-ethdev`` device 1:1.

The application is dependent on below libraries for its functionality.
* ``rte_dma`` library to use DPI HW and transfer data between Host and Octeon memory.
* ``rte_ethdev`` library to receive and send traffic to RPM interfaces
* ``dao_dma`` library as a wrapper to ``rte_dma`` library for request management.
* ``dao_virtio_netdev`` library to enqueue / dequeue packets to / from host.

Application created lcores are below:

* One lcore as service lcore to do ``dao_virtio_netdev_desc_manage()`` API call per virtio dev.
* One or more lcores as worker cores to do ``rte_eth_rx_burst()`` on ethdev's and enqueue packets
  to Host using ``dao_virtio_net_enqueue_burst()``
* One or more lcores as worker cores to do ``dao_virtio_net_dequeue_burst()`` on virtio-net devices
  and enqueue packets to Host using ``rte_eth_tx_burst()``

Apart from application created lcore's, virtio library creates a control lcore for management
purposes.

Setting up EP environment
-------------------------

Setup SDP PF/VF count in EBF menu
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Number of virtio devices is equal to number of SDP VF's enabled. So make sure that config is setup
correctly in EBF menu.

Setup huge pages for DPDK application
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Setup enough hugepages and a mount point for the same in order for the dpdk-virtio-l2fwd application
to run.

Bind required DMA devices to vfio-pci
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
``virtio-l2fwd`` application needs two DMA devices per lcore one for DEV2MEM and another for
MEM2DEV and two more for control lcore. Control lcore is created by virtio library to 
handle control commands. Below is sample code to bind DMA VF's to vfio-pci.

.. code-block:: bash

   DPI_PF=`lspci -d :a080 | awk -e '{print $1}'`
   
   # Enhance DPI engine FIFO size and MRRS
   echo 0x10101010 > /sys/bus/pci/drivers/octeontx2-dpi/module/parameters/eng_fifo_buf
   echo 512 > /sys/bus/pci/drivers/octeontx2-dpi/module/parameters/mrrs
   echo 256 > /sys/bus/pci/drivers/octeontx2-dpi/module/parameters/mps

   echo $DPI_PF > /sys/bus/pci/devices/$DPI_PF/driver/unbind
   echo octeontx2-dpi > /sys/bus/pci/devices/$DPI_PF/driver_override
   echo $DPI_PF > /sys/bus/pci/drivers_probe

   echo 32 >/sys/bus/pci/devices/$DPI_PF/sriov_numvfs
   DPI_VF=`lspci -d :a081 | awk -e '{print $1}' | head -22`
   dpdk-devbind.py -b vfio-pci $DPI_VF

Bind required RPM VF's to vfio-pci
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Application maps ``virtio-net`` device to ``rte-ethdev`` 1:1.

Sample code to map CN10K ethdev's to vfio-pci.

.. code-block:: bash

   ETH_PF=0002:02:00.0
   ETH_PF_NAME=enP2p2s0
   VF_CNT=1
    
   dpdk-devbind.py -b rvu_nicpf $ETH_PF
   echo $VF_CNT > /sys/bus/pci/devices/$ETH_PF/sriov_numvfs
    
   ETH_VF=`lspci -d :a064 | awk -e '{print $1}'`
    
   dpdk-devbind.py -u $ETH_VF
   dpdk-devbind.py -b vfio-pci $ETH_VF

Bind PEM BAR4 and DPI BAR0 platform devices to vfio-platform
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Virtio library uses ``pem0-bar4-mem`` and ``dpi_sdp_regs`` platform devices via ``vfio-platform``.
Hence enable ``vfio-platform`` in kernel build.

* Use ``vfio-platform.reset_required=0`` in kernel commandline if ``vfio-platform`` is inbuilt
  kernel or pass ``reset_required=0`` as module parameter while doing loading ``vfio-platform``
  kernel module.

* Bind ``pem0-bar4-mem`` and ``dpi_sdp_regs`` to vfio-platform.

Sample code to bind platform devices to vfio-platform.

.. code-block:: bash

   # Platform device suffixes to search for
   pem_sfx="pem0-bar4-mem"
   sdp_sfx="dpi_sdp_regs"

   # Loop through devices
   for dev_path in /sys/bus/platform/devices/*; do
       if [[ -d "$dev_path" && "$dev_path" =~ $pem_sfx || "$dev_path" =~ $sdp_sfx ]]; then
           # Get device name from path
           dev_name=$(basename "$dev_path")

           # Bind the device to vfio-platform driver
           echo "vfio-platform" | tee "$dev_path/driver_override" > /dev/null
           echo "$dev_name" | tee "/sys/bus/platform/drivers/vfio-platform/bind" > /dev/null

           echo "Device $dev_name configured."
       fi
   done

Running the EP firmware application
-----------------------------------

The application as number of command line options:

.. code-block:: console

   dpdk-virtio-l2fwd [EAL Options] -- -p <PORTMASK_L[,PORTMASK_H]> -v <VIRTIOMASK_L[,VIRTIOMASK_H]> [other application options]

EAL Options
~~~~~~~~~~~

The following are the EAL command-line options that can be used in conjunction
with the ``dpdk-virtio-l2fwd`` application.
See the DPDK Getting Started Guides for more information on these options.

*   ``-c <COREMASK>`` or ``-l <CORELIST>``

        Set the hexadecimal bitmask of the cores to run on. The corelist is a
        list of cores to use.

Application Options
~~~~~~~~~~~~~~~~~~~

The following are the application command-line options:

* ``-p PORTMASK_L[,PORTMASK_H]``

        Hexadecimal bitmask of ``rte_ethdev`` ports to configure. Comma seperated
        64 bit mask to support upto 128 eth devices. This is mandatory option.

* ``-v VIRTIOMASK_L[,VIRTIOMASK_H]``

        Hexadecimal bitmask of virtio-net devices to configure. Comma seperated
        64 bit mask to support 128 virtio-net devices. This is a mandatory option.

* ``-P``

        Enable promisc mode. Default is promisc mode disabled.

* ``-d <n>``

        Set DMA flush threshold. Default value is 8. Value indicates max number of pointers 
        to cache when requested through ``dao_dma_*()`` API, before doing DMA submit via
        ``rte_dma_*`` API.
* ``-f``

        Disable auto free. Auto free of mbufs by DPI post outbound DMA to Host memory is enabled
        by default. This option disables it for debug purposes.

* ``-s``

        Enable graph stats. Default value is disable. Giving this option multiple times dumps stats
        in verbose.

* ``-y <n>``
        
        Override PCI device info in DMA device vchan config. For debug purposes only.


* ``--eth-config (port,lcore_mask)[,(port,lcore_mask)]``

        Config to indicate on which lcores Rx polling would happen for a given ``rte_ethdev`` port.
        Default config is, all the configured ethdev ports would be polled for Rx on half of the 
        lcore's that are detected and available excluding 1 service lcore.

* ``--virtio-config (dev,lcore_mask)[,(dev,lcore_mask)]``

        Config to indicate on which lcores deq polling would happen for a given ``virtio-net`` port.
        Default config is, all the configured virtio-net devices would be polled for pkts from host
        on half of the lcore's that are detected and available excluding 1 service lcore.

* ``l2fwd-map (eX,vY)[,eX,vY]``

        Config to map one ``rte-ethdev`` port to one ``virtio-net`` device 1:1.
        By default, ethdev 0 is mapped to virtio-netdev 0, ethdev 1 is mapped to virtio-netdev 1 and
        so on.

* ``--max-pkt-len <PKTLEN>``

        Set MTU on all the ethdev devices to <PKTLEN>. Default MTU configured is 1500B.

* ``--pool-buf-len``

        Set max pkt mbuf buffer len. Default is set to RTE_MBUF_DEFAULT_BUF_SIZE. 

* ``--per-port-pool``

        Enable per port pool. When provided, enables creates one pktmbuf pool per
        ethdev/virtio-netdev port.
        Default is one pktmbuf pool for all ethdev's and one pktmbuf pool for all virtio-net
        devices.

* ``--pcap-enable``

        Enable packet capture feature in ``librte_graph``. Default is disabled.

* ``--pcap-num-cap <n>``

        Number of packets to capture via packet capture feature of ``librte_graph``.

* ``pcap-file-name <name>``

        Pcap file name to use.

Example EP firmware command
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Example to command to run ``dpdk-virtio-l2fwd`` on 1 ethdev and 1 virtio-net dev port
with 2 lcores on ethdev-rx, 2 lcores on ethdev-tx, 1 lcore for service core.


.. code-block:: console

   DPI_ALLOW='-a 0000:06:00.1 -a 0000:06:00.2 -a 0000:06:00.3 -a 0000:06:00.4 -a 0000:06:00.5 -a 0000:06:00.6 -a 0000:06:00.7 -a 0000:06:01.0 -a 0000:06:01.1 -a 0000:06:01.2 -a 0000:06:01.3 -a 0000:06:01.4 -a 0000:06:01.5 -a 0000:06:01.6 -a 0000:06:01.7 -a 0000:06:02.0 -a 0000:06:02.1 -a 0000:06:02.2 -a 0000:06:02.3 -a 0000:06:02.4 -a 0000:06:02.5 -a 0000:06:02.6'
   
   dpdk-virtio-l2fwd -l 2-7 -a 0002:02:00.1 $DPI_ALLOW -- -p 0x1 -v 0x1

If ``dpdk-virtio-l2fwd`` is not build with static linking to DPDK, we need to explictly load
node library and PMD libraries for the application to function.

.. code-block:: console

   DPI_ALLOW='-a 0000:06:00.1 -a 0000:06:00.2 -a 0000:06:00.3 -a 0000:06:00.4 -a 0000:06:00.5 -a 0000:06:00.6 -a 0000:06:00.7 -a 0000:06:01.0 -a 0000:06:01.1 -a 0000:06:01.2 -a 0000:06:01.3 -a 0000:06:01.4 -a 0000:06:01.5 -a 0000:06:01.6 -a 0000:06:01.7 -a 0000:06:02.0 -a 0000:06:02.1 -a 0000:06:02.2 -a 0000:06:02.3 -a 0000:06:02.4 -a 0000:06:02.5 -a 0000:06:02.6'

   dpdk-virtio-l2fwd -d librte_node.so -d librte_net_cnxk.so -d librte_mempool_cnxk.so -d librte_dma_cnxk.so -d librte_mempool_ring.so -l 2-7 -a 0002:02:00.1 $DPI_ALLOW -- -p 0x1 -v 0x1

Setting up Host environment
---------------------------

Host requirements
~~~~~~~~~~~~~~~~~
Host needs Linux Kernel version of >= 6.5 (for example latest ubuntu version supports 6.5)
IOMMU should always be on if we need to use VF's with Guest. (x86 intel_iommu=on)

Build KMOD specifically for Host with native compilation(For example x86)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Not providing 'kernel_dir' option would pick /lib/modules/`uname -r`/source  as kernel source

.. code-block:: console

   git clone https://sj1git1.cavium.com/IP/SW/dataplane/dpu-offload
   git checkout dpu-offload-devel

   meson build
   ninja -C build

Bind PEM PF and VF to Host Octeon VDPA driver
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
On Host, we need to bind host PF and VF devices provided by CN10K to ``octep_vdpa`` driver and
then bind the VDPA devices to ``vhost_vdpa`` devices to be available for DPDK or guest.

.. code-block:: console

   modprobe vfio-pci
   modprobe vdpa
   modprobe vhost-vdpa

   insmod octep_vdpa.ko

   HOST_PF=`lspci -Dn -d :b900 | head -1 | cut -f 1 -d " "`
   VF_CNT=1
   VF_CNT_MAX=`cat /sys/bus/pci/devices/$HOST_PF/sriov_totalvfs`
   VF_CNT=$((VF_CNT >VF_CNT_MAX ? VF_CNT_MAX : VF_CNT))

   echo $HOST_PF > /sys/bus/pci/devices/$HOST_PF/driver/unbind
   echo octep_vdpa > /sys/bus/pci/devices/$HOST_PF/driver_override
   echo $HOST_PF > /sys/bus/pci/drivers_probe
   echo $VF_CNT >/sys/bus/pci/devices/$HOST_PF/sriov_numvfs

   SDP_VFS=`lspci -Dn -d :b903 | cut -f 1 -d " "`
   for dev in $SDP_VFS
   do
       vdev=`ls /sys/bus/pci/devices/$dev | grep vdpa`
       while [[ "$vdev" == "" ]]
       do
           echo "Waiting for vdpa device for $dev"
           sleep 1
           vdev=`ls /sys/bus/pci/devices/$dev | grep vdpa`
       done
    echo $vdev >/sys/bus/vdpa/drivers/virtio_vdpa/unbind
    echo $vdev > /sys/bus/vdpa/drivers/vhost_vdpa/bind
   done

Tune MRRS and MPS of PEM PF/VF on Host for performance
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Tune MRRS and MPS on Host PF in order to increase virtio performance.
Example code to do the same if the PEM PF and its bridge devices seen on host are ``0003:01:00.0``
and ``0003:00:00.0``.

.. code-block:: console

   setpci -s 0003:00:00.0 78.w=$(printf %x $((0x$(setpci -s 0003:00:00.0 78.w)|0x20)))
   setpci -s 0003:01:00.0 78.w=$(printf %x $((0x$(setpci -s 0003:01:00.0 78.w)|0x20)))

Running DPDK testpmd on Host virtio device
------------------------------------------

Setup huge pages for DPDK application
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Need to enable sufficient enough hugepages for DPDK application to run.

Increase ulimit for 'max locked memory' to unlimited
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
DPDK application needs to be able to lock memory that is DMA mapped on host. So increase the ulimit
to max for locked memory.

.. code-block:: console

   ulimit -l unlimited

Example command for DPDK testpmd on host with vhost-vdpa device
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Below is example to launch ``dpdk-testpmd`` application on host using ``vhost-vdpa`` device.

.. code-block:: console

   ./dpdk-testpmd -c 0xfff000 --socket-mem 1024 --proc-type auto --file-prefix=virtio-user0 --no-pci --vdev=net_virtio_user0,path=/dev/vhost-vdpa-0,mrg_rxbuf=1,packed_vq=1,in_order=1,queue_size=4096 -- -i --txq=1 --rxq=1 --nb-cores=1 --portmask 0x1 --port-topology=loop

Running DPDK testpmd on virtio-net device on guest
--------------------------------------------------

Host requirements for running Guest
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Follow `Setting up Host environment`_ as a first step.

* Install qemu related packages on host
* Get qemu-8.1.1 and apply below patches on top of it.
* https://patchwork.kernel.org/project/qemu-devel/patch/d01d0de97688c5587935da753c63f0441808cb9d.1691766252.git.yin31149@gmail.com/
* https://patchwork.kernel.org/project/qemu-devel/patch/20240102111432.36817-1-schalla@marvell.com/
* https://patchwork.kernel.org/project/qemu-devel/patch/20240220070935.1617570-1-schalla@marvell.com/ (Note: This patch is not required if the host has page size of 4K)

Build Qemu
^^^^^^^^^^

.. code-block:: console

   wget https://download.qemu.org/qemu-8.1.1.tar.xz
   tar xvJf qemu-8.1.1.tar.xz
   cd qemu-8.1.1
   /* Apply above mentioned patches */
   ./configure
   make

Prepare the Ubuntu cloud image for guest
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Below is the example to prepare ubuntu clound image for ARM guest.

.. code-block:: console

   wget https://cloud-images.ubuntu.com/mantic/current/mantic-server-cloudimg-arm64.img
   virt-customize -a mantic-server-cloudimg-arm64.img --root-password password:a
   mkdir mnt_img

   cat mount_img.sh
   #!/bin/bash
   modprobe nbd max_part=8
   qemu-nbd --connect=/dev/nbd0 $1
   sleep 2
   fdisk /dev/nbd0 -l
   mount /dev/nbd0p1 mnt_img

   # Copy required files to mnt_img/root for example dpdk-testpmd and usertools from dpdk
   cat unmount_img.sh
   #!/bin/bash
   umount mnt_img
   qemu-nbd --disconnect /dev/nbd0
   #rmmod nbd

Launch guest using Qemu
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: console

   ulimit -l unlimited
   cd qemu-8.1.1
   ./build/qemu-system-aarch64  -hda /home/cavium/ws/mantic-server-cloudimg-arm64_vm1.img -name vm1 \
   -netdev type=vhost-vdpa,vhostdev=/dev/vhost-vdpa-0,id=vhost-vdpa1 -device \
   virtio-net-pci,netdev=vhost-vdpa1,disable-modern=off,page-per-vq=on,packed=on,mrg_rxbuf=on,mq=on,rss=on,rx_queue_size=1024,tx_queue_size=1024,disable-legacy=on -enable-kvm -nographic -m 2G -cpu host -smp 3 -machine virt,gic_version=3 -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd

Launch dpdk-testpmd on guest
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Below code block shows method to bind device to vfio-pci to use with DPDK testpmd in guest.

.. code-block:: console

   modprobe vfio-pci
   echo 1 > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode
   # On 106xx $VIRTIO_NETDEV_BDF would come as 0000:00:01.0
   ./usertools/dpdk-devbind.py -b vfio-pci $VIRTIO_NETDEV_BDF
   echo 256 > /proc/sys/vm/nr_hugepages
   ./dpdk-testpmd -c 0x3 -a $VIRTIO_NETDEV_BDF -- -i --nb-cores=1 --txq=1 --rxq=1


Using VDPA device as Kernel virtio-net device on guest
------------------------------------------------------

* Follow `Setting up Host environment`_, `Host requirements for running Guest`_,
  `Prepare the Ubuntu cloud image for guest`_ and `Launch guest using Qemu`_
* Probe virtio_net kernel module if not present already and check for the virtio network interface using ifconfig/ip tool.

Using VDPA device as Kernel virtio-net device on host
-----------------------------------------------------

Run the code block below to create a virtio device on host for each VF using virtio_vdpa.

.. code-block:: console

   modprobe vfio-pci
   modprobe vdpa
   insmod octep_vdpa.ko
   HOST_PF=`lspci -Dn -d :b900 | head -1 | cut -f 1 -d " "`
   VF_CNT=1
   VF_CNT_MAX=`cat /sys/bus/pci/devices/$HOST_PF/sriov_totalvfs`
   VF_CNT=$((VF_CNT >VF_CNT_MAX ? VF_CNT_MAX : VF_CNT))

   echo $HOST_PF > /sys/bus/pci/devices/$HOST_PF/driver/unbind
   echo octep_vdpa > /sys/bus/pci/devices/$HOST_PF/driver_override
   echo $HOST_PF > /sys/bus/pci/drivers_probe
   echo $VF_CNT >/sys/bus/pci/devices/$HOST_PF/sriov_numvfs

   modprobe virtio_vdpa
   modprobe virtio_net
