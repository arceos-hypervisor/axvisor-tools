//! CommandLine Interface and host daemon process for Equation OS.

mod hvc;
mod vmm;

#[allow(non_camel_case_types)]
#[allow(unused)]
mod ioctl;

use clap::{Args, Parser, Subcommand};

#[macro_use]
extern crate log;

#[derive(Parser, Debug)]
#[command(name = "axcli")]
#[command(about = "CommandLine Interface for AxVisor", long_about = None)]
#[command(args_conflicts_with_subcommands = true)]
#[command(flatten_help = true)]
struct CLI {
    #[command(subcommand)]
    subcmd: CLISubCmd,
}

#[derive(Subcommand, Debug)]
#[command(args_conflicts_with_subcommands = true)]
#[command(flatten_help = true)]
enum CLISubCmd {
    /// Subcommands related to hypervisor itself.
    Hv {
        #[command(subcommand)]
        subcmd: HvSubCmd,
    },
    /// Subcommands related to the management of the guest VM.
    VM {
        #[command(subcommand)]
        subcmd: VMSubCmd,
    },
}

#[derive(Subcommand, Debug)]
#[command(args_conflicts_with_subcommands = true)]
#[command(flatten_help = true)]
enum HvSubCmd {
    /// Enable arceos-hypervisor type1.5.
    Enable,
    /// Disable arceos-hypervisor type1.5.
    Disable,
}

#[derive(Subcommand, Debug)]
#[command(args_conflicts_with_subcommands = true)]
#[command(flatten_help = true)]
enum VMSubCmd {
    /// list the info of the VM
    List,
    /// Create a new instance.
    Create(VMCreateArgs),
    /// Remove a VM by its ID.
    Remove {
        /// VM ID to remove.
        #[arg(short, long)]
        vm_id: i32,
    },
}

#[derive(Debug, Args)]
struct VMCreateArgs {
    /// VM name.
    #[arg(short, long)]
    pub name: String,
    /// Configuration file path.
    #[arg(short, long)]
    pub cfg_path: String,
}

use libc::{c_void, sigaction, siginfo_t, SA_SIGINFO, SIGBUS};
use std::ptr;

extern "C" fn sigbus_handler(_sig: i32, info: *mut siginfo_t, context: *mut c_void) {
    unsafe {
        let ucontext = &*(context as *mut libc::ucontext_t);
        let pc = ucontext.uc_mcontext.gregs[libc::REG_RIP as usize];
        eprintln!(
            "Caught SIGBUS at address: {:?}, program counter: 0x{:x}",
            (*info).si_addr(),
            pc
        );
        std::process::exit(1);
    }
}

extern "C" fn sigsegv_handler(_sig: i32, info: *mut siginfo_t, context: *mut c_void) {
    unsafe {
        let ucontext = &*(context as *mut libc::ucontext_t);
        let pc = ucontext.uc_mcontext.gregs[libc::REG_RIP as usize];
        eprintln!(
            "Caught SIGSEGV at address: {:?}, program counter: 0x{:x}",
            (*info).si_addr(),
            pc
        );
        std::process::exit(1);
    }
}

fn install_signal_handlers() {
    unsafe {
        let mut sa: sigaction = std::mem::zeroed();

        // Install SIGBUS handler
        sa.sa_sigaction = sigbus_handler as usize;
        sa.sa_flags = SA_SIGINFO;
        sigaction(SIGBUS, &sa, ptr::null_mut());

        // Install SIGSEGV handler
        sa = std::mem::zeroed();
        sa.sa_sigaction = sigsegv_handler as usize;
        sa.sa_flags = SA_SIGINFO;
        sigaction(libc::SIGSEGV, &sa, ptr::null_mut());
    }
}

fn main() {
    // configure logger and set log level
    env_logger::Builder::new()
        .filter_level(log::LevelFilter::Debug)
        .init();

    install_signal_handlers();

    let cli = CLI::parse();
    match cli.subcmd {
        CLISubCmd::Hv { subcmd } => match subcmd {
            HvSubCmd::Enable => todo!(),
            HvSubCmd::Disable => todo!(),
        },
        CLISubCmd::VM { subcmd } => match subcmd {
            VMSubCmd::List => vmm::list_vms(),
            VMSubCmd::Create(args) => vmm::create_vm(args),
            VMSubCmd::Remove { vm_id } => vmm::remove_vm(vm_id as _),
        },
    }
}
