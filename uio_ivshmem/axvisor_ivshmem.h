#pragma once

/*
 * Interface that the IVSHMEM PCI/UIO driver (uio_ivshmem.c) exposes to
 * axvisor.ko. The driver is compiled into axvisor.ko together with the
 * IVC driver (ivc/kernel_driver/ links uio_ivshmem.c into its build).
 *
 * These functions are only called by main.c within the same module, so
 * they are deliberately not exported (no EXPORT_SYMBOL). External users
 * keep talking to axvisor.ko through the device interfaces (/dev/axivc*
 * misc devices and /dev/uioN), which are unchanged by this consolidation.
 */

int axvisor_ivshmem_register(void);
void axvisor_ivshmem_unregister(void);
