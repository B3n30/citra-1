// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/service.h"

namespace Service {
namespace NWM {

/// Sequence number of the 802.11 authentication frames.
enum class AuthenticationSeq : u16 { SEQ1 = 1, SEQ2 = 2 };

enum class AuthAlgorithm : u16 { OpenSystem = 0 };

enum class AuthStatus : u16 { Successful = 0 };

enum class AssocStatus : u16 { Successful = 0 };

struct AuthenticationFrame {
    u16_le auth_algorithm = static_cast<u16>(AuthAlgorithm::OpenSystem);
    u16_le auth_seq;
    u16_le status_code = static_cast<u16>(AuthStatus::Successful);
};

static_assert(sizeof(AuthenticationFrame) == 6, "AuthenticationFrame has wrong size");

struct AssociationResponseFrame {
    u16_le capabilities;
    u16_le status_code;
    u16_le assoc_id;
};

static_assert(sizeof(AssociationResponseFrame) == 6, "AssociationResponseFrame has wrong size");

/// Generates an 802.11 authentication frame, starting at the frame body.
std::vector<u8> GenerateAuthenticationFrame(AuthenticationSeq seq);

/// Returns the sequence number from the body of an Authentication frame.
AuthenticationSeq GetAuthenticationSeqNumber(const std::vector<u8>& body);

/// Generates an 802.11 association response frame with the specified status, association id and
/// network id, starting at the frame body.
std::vector<u8> GenerateAssocResponseFrame(AssocStatus status, u16 association_id, u32 network_id);

/// Returns a tuple of (association status, association id) from the body of an AssociationResponse
/// frame.
std::tuple<AssocStatus, u16> GetAssociationResult(const std::vector<u8>& body);

} // namespace NWM
} // namespace Service
