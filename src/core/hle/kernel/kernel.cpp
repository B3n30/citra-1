// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/archives.h"
#include "common/serialization/atomic.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/config_mem.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/ipc_debugger/recorder.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/memory.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/hle/kernel/shared_page.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/kernel/timer.h"

namespace Kernel {

KernelSystem* g_kernel;

/// Initialize the kernel
KernelSystem::KernelSystem(Memory::MemorySystem& memory, Core::Timing& timing,
                           std::function<void()> prepare_reschedule_callback, u32 system_mode)
    : memory(memory), timing(timing),
      prepare_reschedule_callback(std::move(prepare_reschedule_callback)) {
    MemoryInit(system_mode);

    resource_limits = std::make_unique<ResourceLimitList>(*this);
    thread_manager = std::make_unique<ThreadManager>(*this);
    timer_manager = std::make_unique<TimerManager>(timing);
    ipc_recorder = std::make_unique<IPCDebugger::Recorder>();
}

/// Shutdown the kernel
KernelSystem::~KernelSystem() = default;

ResourceLimitList& KernelSystem::ResourceLimit() {
    return *resource_limits;
}

const ResourceLimitList& KernelSystem::ResourceLimit() const {
    return *resource_limits;
}

u32 KernelSystem::GenerateObjectID() {
    return next_object_id++;
}

std::shared_ptr<Process> KernelSystem::GetCurrentProcess() const {
    return current_process;
}

void KernelSystem::SetCurrentProcess(std::shared_ptr<Process> process) {
    current_process = process;
    SetCurrentMemoryPageTable(&process->vm_manager.page_table);
}

void KernelSystem::SetCurrentMemoryPageTable(Memory::PageTable* page_table) {
    memory.SetCurrentPageTable(page_table);
    if (current_cpu != nullptr) {
        current_cpu->PageTableChanged(); // notify the CPU the page table in memory has changed
    }
}

void KernelSystem::SetCPU(std::shared_ptr<ARM_Interface> cpu) {
    current_cpu = cpu;
    thread_manager->SetCPU(*cpu);
}

ThreadManager& KernelSystem::GetThreadManager() {
    return *thread_manager;
}

const ThreadManager& KernelSystem::GetThreadManager() const {
    return *thread_manager;
}

TimerManager& KernelSystem::GetTimerManager() {
    return *timer_manager;
}

const TimerManager& KernelSystem::GetTimerManager() const {
    return *timer_manager;
}

SharedPage::Handler& KernelSystem::GetSharedPageHandler() {
    return *shared_page_handler;
}

const SharedPage::Handler& KernelSystem::GetSharedPageHandler() const {
    return *shared_page_handler;
}

IPCDebugger::Recorder& KernelSystem::GetIPCRecorder() {
    return *ipc_recorder;
}

const IPCDebugger::Recorder& KernelSystem::GetIPCRecorder() const {
    return *ipc_recorder;
}

void KernelSystem::AddNamedPort(std::string name, std::shared_ptr<ClientPort> port) {
    named_ports.emplace(std::move(name), std::move(port));
}

template <class Archive>
void KernelSystem::serialize(Archive& ar, const unsigned int file_version)
{
    ar & named_ports;
    // TODO: CPU
    // NB: subsystem references and prepare_reschedule_callback are constant
    ar & *resource_limits.get();
    ar & next_object_id;
    //ar & *timer_manager.get();
    ar & next_process_id;
    ar & process_list;
    ar & current_process;
    // ar & *thread_manager.get();
    //ar & *config_mem_handler.get();
    //ar & *shared_page_handler.get();
}

SERIALIZE_IMPL(KernelSystem)

} // namespace Kernel
