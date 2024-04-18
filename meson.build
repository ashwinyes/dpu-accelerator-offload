# SPDX-License-Identifier: Marvell-MIT
# Copyright (c) 2023 Marvell.

project(
	'DPU Accelerator Offloads',
	'C',
	meson_version: '>= 0.63.0',
	version: '24.04.0',
	default_options: ['warning_level=2', 'werror=true', 'buildtype=release']
)

# Build Configuration Data
DAO_BUILD_CONF = configuration_data()
# DPU Accelerator Offloads sources
DAO_SOURCES = []
# DPU Accelerator Offloads include directories
DAO_INCLUDES = []
# Mandatory dpdk library dependency
DAO_DPDK_LIB_DEPS = []
# DPU Accelerator Offloads static libraries
DAO_STATIC_LIBS = []
# Enabled/disabled static dao applicaitons/libraries/tests
DAO_MODULES_ENABLED = []
DAO_MODULES_DISABLED = []

subdir('buildtools')
subdir('dep')
subdir('config')

# Mandatory dependency
if host_machine.cpu_family() == 'aarch64'
	if DAO_BUILD_CONF.has('DAO_LIBDPDK_DEP')
		subdir('lib')
		subdir('app')
		subdir('tests')
	endif
endif

subdir('doc')
subdir('kmod')

# Dump the enable modules status
msg = '\n===============\nEnabled Modules\n===============\n'
last = ''
foreach comp : DAO_MODULES_ENABLED
	if last != comp[0]
		msg += '\n' + comp[0] + ':'
	endif
	msg += '\n\t'
	foreach elem : comp[1]
		msg += elem + ', '
	endforeach
	last = comp[0]
endforeach
msg += '\n\n================\nDisabled Modules\n================\n'
last = ''
foreach comp : DAO_MODULES_DISABLED
	if last != comp[0]
		msg += '\n' + comp[0] + ':'
	endif
	msg += '\n\t'
	foreach elem : comp[1]
		msg += elem + ', '
	endforeach
	last = comp[0]
endforeach
msg += '\n\n================\n'
message(msg)
