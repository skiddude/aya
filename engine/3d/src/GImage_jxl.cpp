/**
  @file GImage_jxl.cpp
  @author PATRICK STAR
  @created 2025
 */

#include "GImage.hpp"
#include "BinaryOutput.hpp"
#include "platform.hpp"

#include <jxl/encode.h>
#include <jxl/decode.h>
#include <jxl/thread_parallel_runner.h>
#include <jxl/memory_manager.h>
#include <stdexcept>
#include <vector>

namespace G3D
{

void GImage::encodeJXL(BinaryOutput& out) const
{
    if (!(m_channels == 1 || m_channels == 3 || m_channels == 4))
    {
        throw GImage::Error("Unsupported number of channels for JXL.");
    }
    if (m_width <= 0 || m_height <= 0)
    {
        throw GImage::Error("Invalid dimensions for JXL.");
    }

    JxlEncoder* encoder = JxlEncoderCreate(nullptr);
    if (!encoder)
    {
        throw GImage::Error("Failed to create JXL encoder.");
    }

    void* runner = JxlThreadParallelRunnerCreate(nullptr, JxlThreadParallelRunnerDefaultNumWorkerThreads());
    if (!runner)
    {
        JxlEncoderDestroy(encoder);
        throw GImage::Error("Failed to create JXL thread runner.");
    }
    JxlEncoderSetParallelRunner(encoder, JxlThreadParallelRunner, runner);

    // Create frame settings
    JxlEncoderFrameSettings* frame_settings = JxlEncoderFrameSettingsCreate(encoder, nullptr);
    if (!frame_settings)
    {
        JxlEncoderDestroy(encoder);
        JxlThreadParallelRunnerDestroy(runner);
        throw GImage::Error("Failed to create JXL frame settings.");
    }

    JxlPixelFormat pixel_format = {static_cast<uint32_t>(m_channels), JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};

    if (JxlEncoderAddImageFrame(frame_settings, &pixel_format, m_byte, m_width * m_height * m_channels) != JXL_ENC_SUCCESS)
    {
        JxlEncoderDestroy(encoder);
        JxlThreadParallelRunnerDestroy(runner);
        throw GImage::Error("Failed to add image frame to JXL encoder.");
    }

    JxlEncoderCloseInput(encoder);

    std::vector<uint8_t> output(4096);
    uint8_t* next_out = output.data();
    size_t avail_out = output.size();
    JxlEncoderStatus status;

    while ((status = JxlEncoderProcessOutput(encoder, &next_out, &avail_out)) == JXL_ENC_NEED_MORE_OUTPUT)
    {
        size_t offset = output.size() - avail_out;
        output.resize(output.size() * 2);
        next_out = output.data() + offset;
        avail_out = output.size() - offset;
    }

    if (status != JXL_ENC_SUCCESS)
    {
        JxlEncoderDestroy(encoder);
        JxlThreadParallelRunnerDestroy(runner);
        throw GImage::Error("Error during JXL encoding.");
    }

    out.writeBytes(output.data(), output.size() - avail_out);

    JxlEncoderDestroy(encoder);
    JxlThreadParallelRunnerDestroy(runner);
}

void GImage::decodeJXL(BinaryInput& input)
{
    std::vector<uint8_t> input_data(input.getLength());
    input.readBytes(input_data.data(), input_data.size());

    JxlDecoder* decoder = JxlDecoderCreate(nullptr);
    if (!decoder)
    {
        throw GImage::Error("Failed to create JXL decoder.");
    }

    // Subscribe to the events we need
    if (JxlDecoderSubscribeEvents(decoder, JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS)
    {
        JxlDecoderDestroy(decoder);
        throw GImage::Error("Failed to subscribe to JXL decoder events.");
    }

    if (JxlDecoderSetInput(decoder, input_data.data(), input_data.size()) != JXL_DEC_SUCCESS)
    {
        JxlDecoderDestroy(decoder);
        throw GImage::Error("Failed to set JXL input data.");
    }
    JxlDecoderCloseInput(decoder);

    JxlBasicInfo info;
    JxlPixelFormat pixel_format;
    bool info_retrieved = false;
    bool buffer_set = false;

    // Process decoder events
    JxlDecoderStatus status;
    while ((status = JxlDecoderProcessInput(decoder)) != JXL_DEC_SUCCESS)
    {
        if (status == JXL_DEC_BASIC_INFO)
        {
            if (JxlDecoderGetBasicInfo(decoder, &info) != JXL_DEC_SUCCESS)
            {
                JxlDecoderDestroy(decoder);
                throw GImage::Error("Failed to get JXL basic info.");
            }

            m_width = info.xsize;
            m_height = info.ysize;
            m_channels = (info.alpha_bits > 0) ? 4 : 3;
            info_retrieved = true;
        }
        else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER)
        {
            if (!info_retrieved)
            {
                JxlDecoderDestroy(decoder);
                throw GImage::Error("Need output buffer before basic info retrieved.");
            }

            pixel_format = {static_cast<uint32_t>(m_channels), JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};
            m_byte = (uint8_t*)m_memMan->alloc(m_width * m_height * m_channels);

            if (!m_byte)
            {
                JxlDecoderDestroy(decoder);
                throw GImage::Error("Failed to allocate memory for decoded JXL image.");
            }

            if (JxlDecoderSetImageOutBuffer(decoder, &pixel_format, m_byte, m_width * m_height * m_channels) != JXL_DEC_SUCCESS)
            {
                JxlDecoderDestroy(decoder);
                throw GImage::Error("Failed to set JXL output buffer.");
            }
            buffer_set = true;
        }
        else if (status == JXL_DEC_FULL_IMAGE)
        {
            // Image successfully decoded
            break;
        }
        else if (status == JXL_DEC_ERROR)
        {
            JxlDecoderDestroy(decoder);
            throw GImage::Error("Error during JXL decoding.");
        }
        else
        {
            JxlDecoderDestroy(decoder);
            throw GImage::Error("Unexpected JXL decoder status.");
        }
    }

    if (!info_retrieved || !buffer_set)
    {
        JxlDecoderDestroy(decoder);
        throw GImage::Error("JXL decoding completed without retrieving all necessary data.");
    }

    JxlDecoderDestroy(decoder);
}


} // namespace G3D