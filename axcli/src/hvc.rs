use std::arch::asm;

use axhvc::HyperCallCode;

#[inline(always)]
fn trigger_hypercall(
    code: HyperCallCode,
    arg1: u64,
    arg2: u64,
    arg3: u64,
    arg4: u64,
    arg5: u64,
    arg6: u64,
) -> isize {
    let result: isize;
    unsafe {
        asm!(
            "vmcall",
            in("rax") code as u64,
            in("rdi") arg1,
            in("rsi") arg2,
            in("rdx") arg3,
            in("rcx") arg4,
            in("r8") arg5,
            in("r9") arg6,
            lateout("rax") result,
            options(nostack, preserves_flags)
        );
    }
    result
}

#[allow(unused)]
pub fn hvc_debug() {
    let result = trigger_hypercall(HyperCallCode::HDebug, 0, 0, 0, 0, 0, 0);
    info!("hvc_debug result: {:#x}", result);
}

pub fn hvc_init_shim(ksched_shm_base: u64, mode: u64) {
    let result = trigger_hypercall(HyperCallCode::HInitShim, ksched_shm_base, mode, 0, 0, 0, 0);
    info!("hvc_init_shim result: {:#x}", result);
}

pub fn hvc_setup_instance(instance_id: u64, entry: u64, stack: u64) -> isize {
    info!(
        "[*] Setting up instance ID: {}, entry: {:#x}, stack: {:#x}",
        instance_id, entry, stack
    );

    trigger_hypercall(
        HyperCallCode::HSetupInstance,
        instance_id,
        entry,
        stack,
        0,
        0,
        0,
    )
}

pub fn hvc_daemon_shmat(
    instance_id: u64,
    process_id: u64,
    shmkey: u64,
    host_shmaddr: u64,
    guest_shmaddr: u64,
    shmsize: u64,
    shmflg: u64,
) -> isize {
    let packed_meta = ((process_id & 0xffff_ffff) << 32) | (shmkey & 0xffff_ffff);
    debug!(
        "[*] Attaching shm instance ID:{}, process ID: {}, key: {:#x}, host_addr: {:#x}, guest_addr: {:#x}, size: {:#x}, flags: {:#x}",
        instance_id, process_id, shmkey, host_shmaddr, guest_shmaddr, shmsize, shmflg
    );
    trigger_hypercall(
        HyperCallCode::HIVCSHMAt,
        instance_id,
        packed_meta,
        host_shmaddr,
        guest_shmaddr,
        shmsize,
        shmflg,
    )
}
