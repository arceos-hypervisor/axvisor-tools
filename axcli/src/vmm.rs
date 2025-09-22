//! Virtual Machine Management
use std::ffi::CStr;

use axvmconfig::AxVMCrateConfig;
use libc::{c_char, c_int};

use crate::ioctl::iow;
use crate::VMCreateArgs;

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct AxCreateVMArg {
    pub vm_id: u64,
    pub kernel_image_addr: u64,
    pub kernel_image_size: u64,
    pub bios_image_addr: u64,
    pub bios_image_size: u64,
    pub initrd_image_addr: u64,
    pub initrd_image_size: u64,
    pub cfg_file_addr: u64,
    pub cfg_file_size: u64,
}

const JAILHOUSE_DEVICE_NAME: &CStr =
    unsafe { CStr::from_bytes_with_nul_unchecked(b"/dev/jailhouse\0") };

const AX_CREATE_VM: u64 = iow::<AxCreateVMArg>(0, 0x11);

fn open_jailhouse_dev() -> Result<c_int, String> {
    let fd = unsafe {
        libc::open(
            JAILHOUSE_DEVICE_NAME.as_ptr() as *const c_char,
            libc::O_RDWR,
        )
    };

    if fd < 0 {
        return Err(format!(
            "Failed to open {}, error {}",
            JAILHOUSE_DEVICE_NAME.to_string_lossy(),
            std::io::Error::last_os_error()
        ));
    }

    Ok(fd)
}

pub fn list_vms() {
    unimplemented!("Listing all VMs...");
    // Implementation for listing VMs goes here.
}

pub fn create_vm(args: VMCreateArgs) {
    println!(
        "Creating VM with name: {} and config: {}",
        args.name, args.cfg_path
    );
    let vm_config_file_raw =
        std::fs::read_to_string(args.cfg_path.clone()).expect("Failed to read VM config file");
    let vm_create_config = AxVMCrateConfig::from_toml(vm_config_file_raw.as_str())
        .expect(format!("Failed to resolve VM config {}", &args.cfg_path).as_str());
    println!("VM Config: {:?}", vm_create_config);
    // Implementation for creating a VM goes here.

    let bios_buffer = if let Some(bios_path) = vm_create_config.kernel.bios_path {
        if !std::path::Path::new(&bios_path).exists() {
            panic!("BIOS file not found at path: {}", bios_path);
        }

        std::fs::read(&bios_path)
            .expect(format!("Failed to read BIOS file at path: {}", bios_path).as_str())
    } else {
        warn!("BIOS path is not specified in the configuration.");
        Vec::new()
    };

    let ramdisk_buffer = if let Some(ramdisk_path) = vm_create_config.kernel.ramdisk_path {
        if !std::path::Path::new(&ramdisk_path).exists() {
            panic!("Initrd file not found at path: {}", ramdisk_path);
        }

        std::fs::read(&ramdisk_path)
            .expect(format!("Failed to read initrd file at path: {}", ramdisk_path).as_str())
    } else {
        warn!("Initrd path is not specified in the configuration.");
        Vec::new()
    };

    let kernel_image_buffer = std::fs::read(&vm_create_config.kernel.kernel_path).expect(
        format!(
            "Failed to read kernel file at path: {}",
            vm_create_config.kernel.kernel_path
        )
        .as_str(),
    );

    let mut create_arg = AxCreateVMArg {
        vm_id: 0, // Let the kernel assign an ID
        kernel_image_addr: kernel_image_buffer.as_ptr() as u64,
        kernel_image_size: kernel_image_buffer.len() as u64,
        bios_image_addr: if bios_buffer.is_empty() {
            0
        } else {
            bios_buffer.as_ptr() as u64
        },
        bios_image_size: bios_buffer.len() as u64,
        initrd_image_addr: if ramdisk_buffer.is_empty() {
            0
        } else {
            ramdisk_buffer.as_ptr() as u64
        },
        initrd_image_size: ramdisk_buffer.len() as u64,
        cfg_file_addr: vm_config_file_raw.as_bytes().as_ptr() as u64,
        cfg_file_size: vm_config_file_raw.as_bytes().len() as u64,
    };

    let fd = open_jailhouse_dev().expect("Failed to open jailhouse device");

    let ret = unsafe { libc::ioctl(fd, AX_CREATE_VM as libc::c_ulong, &mut create_arg as *mut _) };

    if ret < 0 {
        panic!("Failed to create VM: {}", std::io::Error::last_os_error());
    }

    if create_arg.vm_id == 0 {
        panic!("VM ID was not assigned by the kernel");
    }

    info!("VM created successfully, ID: {}", create_arg.vm_id);
}

pub fn remove_vm(vm_id: i32) {
    unimplemented!("Removing VM with ID: {}", vm_id);
    // Implementation for removing a VM goes here.
}
