// Copyright (c) 2026 I/O Coin Developers
// DIONS 2.0 - SBPF FFI C++ Header
//
// C++ interface to the Rust SBPF (Solana BPF VM) library.

#ifndef DIONS2_SBPF_FFI_H
#define DIONS2_SBPF_FFI_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

/// Opaque handle to SBPF VM instance
typedef struct SbpfVmHandle SbpfVmHandle;

/// Result codes for SBPF operations
typedef enum SbpfResult {
    SBPF_SUCCESS = 0,
    SBPF_INVALID_PROGRAM = 1,
    SBPF_VERIFICATION_FAILED = 2,
    SBPF_EXECUTION_FAILED = 3,
    SBPF_OUT_OF_MEMORY = 4,
    SBPF_INVALID_PARAMETER = 5,
} SbpfResult;

/// Execution result structure
typedef struct SbpfExecutionResult {
    SbpfResult result;
    uint64_t return_value;
    uint64_t instruction_count;
} SbpfExecutionResult;

/// Create a new SBPF VM handle
SbpfVmHandle* sbpf_vm_create(void);

/// Destroy a SBPF VM handle
void sbpf_vm_destroy(SbpfVmHandle* handle);

/// Execute SBPF bytecode
SbpfExecutionResult sbpf_execute(
    SbpfVmHandle* handle,
    const uint8_t* program,
    size_t program_len,
    const uint8_t* input,
    size_t input_len,
    uint64_t instruction_limit
);

/// Get SBPF library version
const char* sbpf_version(void);

/// Check if SBPF is available
bool sbpf_available(void);

#ifdef __cplusplus
}
#endif

// C++ wrapper
#ifdef __cplusplus

#include <memory>
#include <vector>
#include <string>

namespace dions2 {

/**
 * SBPF VM Wrapper - Executes Solana BPF programs
 */
class SBPFExecutor {
private:
    SbpfVmHandle* handle_;

public:
    SBPFExecutor() : handle_(nullptr) {
#ifdef HAVE_SBPF
        handle_ = sbpf_vm_create();
#endif
    }

    ~SBPFExecutor() {
#ifdef HAVE_SBPF
        if (handle_) {
            sbpf_vm_destroy(handle_);
        }
#endif
    }

    // Non-copyable
    SBPFExecutor(const SBPFExecutor&) = delete;
    SBPFExecutor& operator=(const SBPFExecutor&) = delete;

    // Check if initialized
    bool IsValid() const {
        return handle_ != nullptr;
    }

    // Execute BPF program (ELF format)
    SbpfExecutionResult Execute(
        const std::vector<uint8_t>& program,
        const std::vector<uint8_t>& input = {},
        uint64_t instruction_limit = 1000000
    ) {
#ifdef HAVE_SBPF
        if (!handle_) {
            return {SBPF_INVALID_PARAMETER, 0, 0};
        }

        return sbpf_execute(
            handle_,
            program.data(),
            program.size(),
            input.empty() ? nullptr : input.data(),
            input.size(),
            instruction_limit
        );
#else
        return {SBPF_INVALID_PARAMETER, 0, 0};
#endif
    }

    // Get version string
    static std::string GetVersion() {
#ifdef HAVE_SBPF
        const char* v = sbpf_version();
        return v ? std::string(v) : "unknown";
#else
        return "not available";
#endif
    }

    // Check availability
    static bool IsAvailable() {
#ifdef HAVE_SBPF
        return sbpf_available();
#else
        return false;
#endif
    }
};

} // namespace dions2

#endif // __cplusplus

#endif // DIONS2_SBPF_FFI_H
