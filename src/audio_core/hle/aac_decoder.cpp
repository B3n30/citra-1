// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "audio_core/hle/aac_decoder.h"
#include "audio_core/hle/wmf_decoder.h"

namespace AudioCore::HLE {

class AACDecoder::Impl {
public:
    explicit Impl(Memory::MemorySystem& memory);
    ~Impl();
    std::optional<BinaryResponse> ProcessRequest(const BinaryRequest& request);

private:
    std::optional<BinaryResponse> Initalize(const BinaryRequest& request);

    void Clear();

    std::optional<BinaryResponse> Decode(const BinaryRequest& request);

    bool initalized = false;

    Memory::MemorySystem& memory;

    IMFTransform* transform = NULL;
    DWORD in_stream_id = 0;
    DWORD out_stream_id = 0;
};

AACDecoder::Impl::Impl(Memory::MemorySystem& memory) : memory(memory) {
    mf_coinit();
}

AACDecoder::Impl::~Impl() = default;

std::optional<BinaryResponse> AACDecoder::Impl::ProcessRequest(const BinaryRequest& request) {
    if (request.codec != DecoderCodec::AAC) {
        LOG_ERROR(Audio_DSP, "Got wrong codec {}", static_cast<u16>(request.codec));
        return {};
    }

    switch (request.cmd) {
    case DecoderCommand::Init: {
        return Initalize(request);
    }
    case DecoderCommand::Decode: {
        return Decode(request);
    }
    case DecoderCommand::Unknown: {
        BinaryResponse response;
        std::memcpy(&response, &request, sizeof(response));
        response.unknown1 = 0x0;
        return response;
    }
    default:
        LOG_ERROR(Audio_DSP, "Got unknown binary request: {}", static_cast<u16>(request.cmd));
        return {};
    }
}

std::optional<BinaryResponse> AACDecoder::Impl::Initalize(const BinaryRequest& request) {
    if (initalized) {
        Clear();
    }

    BinaryResponse response;
    std::memcpy(&response, &request, sizeof(response));
    response.unknown1 = 0x0;

    if (mf_decoder_init(&transform) != 0) {
        LOG_CRITICAL(Audio_DSP, "Can't init decoder");
        return response;
    }

    HRESULT hr = transform->GetStreamIDs(1, &in_stream_id, 1, &out_stream_id);
    if (hr == E_NOTIMPL) {
        // if not implemented, it means this MFT does not assign stream ID for you
        in_stream_id = 0;
        out_stream_id = 0;
    } else if (FAILED(hr)) {
        ReportError("Decoder failed to initialize the stream ID", hr);
        SafeRelease(&transform);
        return response;
    }

    select_input_mediatype(transform, in_stream_id);
    select_output_mediatype(transform, out_stream_id);

    initalized = true;
    return response;
}

void AACDecoder::Impl::Clear() {
    if (initalized) {
        mf_flush(&transform);
        mf_deinit(&transform);
    }
    initalized = false;
}

std::optional<BinaryResponse> AACDecoder::Impl::Decode(const BinaryRequest& request) {
    BinaryResponse response;
    response.codec = request.codec;
    response.cmd = request.cmd;
    response.size = request.size;
    response.num_channels = 2;
    response.num_samples = 1024;

    if (!initalized) {
        LOG_DEBUG(Audio_DSP, "Decoder not initalized");
        // This is a hack to continue games that are not compiled with the aac codec
        return response;
    }

    if (request.src_addr < Memory::FCRAM_PADDR ||
        request.src_addr + request.size > Memory::FCRAM_PADDR + Memory::FCRAM_SIZE) {
        LOG_ERROR(Audio_DSP, "Got out of bounds src_addr {:08x}", request.src_addr);
        return {};
    }
    u8* data = memory.GetFCRAMPointer(request.src_addr - Memory::FCRAM_PADDR);

    std::array<std::vector<u8>, 2> out_streams;

    IMFSample* sample = NULL;
    IMFSample* output = NULL;
    int status = 0;
    sample = create_sample((void*)data, request.size, 1, 0);
    send_sample(transform, in_stream_id, sample);
    // transform->GetOutputStatus(&flags);
    receive_sample(transform, out_stream_id, &output);

    IMFMediaBuffer* buf = NULL;
    output->ConvertToContiguousBuffer(&buf);
    BYTE* buffer = nullptr;
    DWORD max_length(0);
    DWORD current_length(0);
    buf->Lock(&buffer, &max_length, &current_length);
    out_streams[0].resize(current_length);

    // TODO: Figure out how to split it into it's own channels
    // TODO: Check if the result is s16
    memcpy(out_streams[0].data(), buffer, current_length);
    buf->Unlock();

    if (out_streams[0].size() != 0) {
        if (request.dst_addr_ch0 < Memory::FCRAM_PADDR ||
            request.dst_addr_ch0 + out_streams[0].size() >
                Memory::FCRAM_PADDR + Memory::FCRAM_SIZE) {
            LOG_ERROR(Audio_DSP, "Got out of bounds dst_addr_ch0 {:08x}", request.dst_addr_ch0);
            return {};
        }
        std::memcpy(memory.GetFCRAMPointer(request.dst_addr_ch0 - Memory::FCRAM_PADDR),
                    out_streams[0].data(), out_streams[0].size());
    }

    if (out_streams[1].size() != 0) {
        if (request.dst_addr_ch1 < Memory::FCRAM_PADDR ||
            request.dst_addr_ch1 + out_streams[1].size() >
                Memory::FCRAM_PADDR + Memory::FCRAM_SIZE) {
            LOG_ERROR(Audio_DSP, "Got out of bounds dst_addr_ch1 {:08x}", request.dst_addr_ch1);
            return {};
        }
        std::memcpy(memory.GetFCRAMPointer(request.dst_addr_ch1 - Memory::FCRAM_PADDR),
                    out_streams[1].data(), out_streams[1].size());
    }
    return response;
}

AACDecoder::AACDecoder(Memory::MemorySystem& memory) : impl(std::make_unique<Impl>(memory)) {}

AACDecoder::~AACDecoder() = default;

std::optional<BinaryResponse> AACDecoder::ProcessRequest(const BinaryRequest& request) {
    return impl->ProcessRequest(request);
}

} // namespace AudioCore::HLE
