/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "ahal_AudioExtn"
#include <dlfcn.h>
#include "AudioExtn.h"
#define AUDIO_OUTPUT_BIT_WIDTH ((config_->offload_info.bit_width == 32) ? 24:config_->offload_info.bit_width)

static batt_listener_init_t batt_listener_init;
static batt_listener_deinit_t batt_listener_deinit;
static batt_prop_is_charging_t batt_prop_is_charging;
static bool battery_listener_enabled;
static void *batt_listener_lib_handle;

int AudioExtn::audio_extn_parse_compress_metadata(struct audio_config *config_, qal_snd_dec_t *qal_snd_dec, str_parms *parms, uint32_t *sr, uint16_t *ch) {
   int ret = 0;
   char value[32];
   *sr = 0;
   *ch = 0;
   uint16_t flac_sample_size = ((config_->offload_info.bit_width == 32) ? 24:config_->offload_info.bit_width);

   if (config_->offload_info.format == AUDIO_FORMAT_FLAC) {
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_FLAC_MIN_BLK_SIZE, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->flac_dec.min_blk_size = atoi(value);
            //out->is_compr_metadata_avail = true; check about this 
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_FLAC_MAX_BLK_SIZE, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->flac_dec.max_blk_size = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_FLAC_MIN_FRAME_SIZE, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->flac_dec.min_frame_size = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_FLAC_MAX_FRAME_SIZE, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->flac_dec.max_frame_size = atoi(value);
        }
        qal_snd_dec->flac_dec.sample_size = flac_sample_size;
        ALOGD("FLAC metadata: sample_size %d min_blk_size %d, max_blk_size %d min_frame_size %d max_frame_size %d",
              qal_snd_dec->flac_dec.sample_size,
              qal_snd_dec->flac_dec.min_blk_size,
              qal_snd_dec->flac_dec.max_blk_size,
              qal_snd_dec->flac_dec.min_frame_size,
              qal_snd_dec->flac_dec.max_frame_size);
    }

    else if (config_->offload_info.format == AUDIO_FORMAT_ALAC) {
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_FRAME_LENGTH, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.frame_length = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_COMPATIBLE_VERSION, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.compatible_version = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_BIT_DEPTH, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.bit_depth = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_PB, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.pb = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_MB, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.mb = atoi(value);
        }

        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_KB, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.kb = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_NUM_CHANNELS, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.num_channels = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_MAX_RUN, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.max_run = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_MAX_FRAME_BYTES, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.max_frame_bytes = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_AVG_BIT_RATE, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.avg_bit_rate = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_SAMPLING_RATE, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.sample_rate = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_ALAC_CHANNEL_LAYOUT_TAG, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->alac_dec.channel_layout_tag = atoi(value);
        }
        *sr = qal_snd_dec->alac_dec.sample_rate;
        *ch = qal_snd_dec->alac_dec.num_channels;
        ALOGD("ALAC CSD values: frameLength %d bitDepth %d numChannels %d"
                " maxFrameBytes %d, avgBitRate %d, sampleRate %d",
                qal_snd_dec->alac_dec.frame_length,
                qal_snd_dec->alac_dec.bit_depth,
                qal_snd_dec->alac_dec.num_channels,
                qal_snd_dec->alac_dec.max_frame_bytes,
                qal_snd_dec->alac_dec.avg_bit_rate,
                qal_snd_dec->alac_dec.sample_rate);
    }

    else if (config_->offload_info.format == AUDIO_FORMAT_APE) {
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_COMPATIBLE_VERSION, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->ape_dec.compatible_version = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_COMPRESSION_LEVEL, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->ape_dec.compression_level = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_FORMAT_FLAGS, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->ape_dec.format_flags = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_BLOCKS_PER_FRAME, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->ape_dec.blocks_per_frame = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_FINAL_FRAME_BLOCKS, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->ape_dec.final_frame_blocks = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_TOTAL_FRAMES, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->ape_dec.total_frames = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_BITS_PER_SAMPLE, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->ape_dec.bits_per_sample = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_NUM_CHANNELS, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->ape_dec.num_channels = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_SAMPLE_RATE, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->ape_dec.sample_rate = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_APE_SEEK_TABLE_PRESENT, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->ape_dec.seek_table_present = atoi(value);
        }
        *sr = qal_snd_dec->ape_dec.sample_rate;
        *ch = qal_snd_dec->ape_dec.num_channels;
        ALOGD("APE CSD values: compatibleVersion %d compressionLevel %d"
                " formatFlags %d blocksPerFrame %d finalFrameBlocks %d"
                " totalFrames %d bitsPerSample %d numChannels %d"
                " sampleRate %d seekTablePresent %d",
                qal_snd_dec->ape_dec.compatible_version,
                qal_snd_dec->ape_dec.compression_level,
                qal_snd_dec->ape_dec.format_flags,
                qal_snd_dec->ape_dec.blocks_per_frame,
                qal_snd_dec->ape_dec.final_frame_blocks,
                qal_snd_dec->ape_dec.total_frames,
                qal_snd_dec->ape_dec.bits_per_sample,
                qal_snd_dec->ape_dec.num_channels,
                qal_snd_dec->ape_dec.sample_rate,
                qal_snd_dec->ape_dec.seek_table_present);
    }
    else if (config_->offload_info.format == AUDIO_FORMAT_VORBIS) {
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_VORBIS_BITSTREAM_FMT, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->vorbis_dec.bit_stream_fmt = atoi(value);
        }
    }
    else if (config_->offload_info.format == AUDIO_FORMAT_WMA || config_->offload_info.format == AUDIO_FORMAT_WMA_PRO) {
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_FORMAT_TAG, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->wma_dec.fmt_tag = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_AVG_BIT_RATE, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->wma_dec.avg_bit_rate = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_BLOCK_ALIGN, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->wma_dec.super_block_align = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_BIT_PER_SAMPLE, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->wma_dec.bits_per_sample = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_CHANNEL_MASK, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->wma_dec.channelmask = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->wma_dec.encodeopt = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION1, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->wma_dec.encodeopt1 = atoi(value);
        }
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION2, value, sizeof(value));
        if (ret >= 0) {
            qal_snd_dec->wma_dec.encodeopt2 = atoi(value);
        }
        ALOGD("WMA params: fmt %x, bit rate %x, balgn %x, sr %d, chmsk %x"
                " encop %x, op1 %x, op2 %x",
                qal_snd_dec->wma_dec.fmt_tag,
                qal_snd_dec->wma_dec.avg_bit_rate,
                qal_snd_dec->wma_dec.super_block_align,
                qal_snd_dec->wma_dec.bits_per_sample,
                qal_snd_dec->wma_dec.channelmask,
                qal_snd_dec->wma_dec.encodeopt,
                qal_snd_dec->wma_dec.encodeopt1,
                qal_snd_dec->wma_dec.encodeopt2);
    }

    else if ((config_->offload_info.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_AAC ||
             (config_->offload_info.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_AAC_ADTS ||
             (config_->offload_info.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_AAC_ADIF ||
             (config_->offload_info.format & AUDIO_FORMAT_MAIN_MASK) == AUDIO_FORMAT_AAC_LATM) {

       qal_snd_dec->aac_dec.audio_obj_type = 29;
       qal_snd_dec->aac_dec.pce_bits_size = 0;
       ALOGD("AAC params: aot %d pce %d", qal_snd_dec->aac_dec.audio_obj_type, qal_snd_dec->aac_dec.pce_bits_size);
       ALOGD("format %x", config_->offload_info.format);
    }
    return 0;
}

int AudioExtn::get_controller_stream_from_params(struct str_parms *parms,
                                           int *controller, int *stream) {

    str_parms_get_int(parms, "controller", controller);
    str_parms_get_int(parms, "stream", stream);
    if (*controller < 0 || *controller >= MAX_CONTROLLERS ||
           *stream < 0 || *stream >= MAX_STREAMS_PER_CONTROLLER) {
        *controller = 0;
        *stream = 0;
        return -EINVAL;
    }
    return 0;
}

// START: BATTERY_LISTENER ==================================================
#ifdef __LP64__
#define BATTERY_LISTENER_LIB_PATH "/vendor/lib64/libbatterylistener.so"
#else
#define BATTERY_LISTENER_LIB_PATH "/vendor/lib/libbatterylistener.so"
#endif

void AudioExtn::battery_listener_feature_init(bool is_feature_enabled) {
    battery_listener_enabled = is_feature_enabled;
    if (is_feature_enabled) {
        batt_listener_lib_handle = dlopen(BATTERY_LISTENER_LIB_PATH, RTLD_NOW);

        if (!batt_listener_lib_handle) {
            ALOGE("%s: dlopen failed", __func__);
            goto feature_disabled;
        }
        if (!(batt_listener_init = (batt_listener_init_t)dlsym(
                            batt_listener_lib_handle, "battery_properties_listener_init")) ||
                !(batt_listener_deinit =
                     (batt_listener_deinit_t)dlsym(
                        batt_listener_lib_handle, "battery_properties_listener_deinit")) ||
                !(batt_prop_is_charging =
                     (batt_prop_is_charging_t)dlsym(
                        batt_listener_lib_handle, "battery_properties_is_charging"))) {
             ALOGE("%s: dlsym failed", __func__);
                goto feature_disabled;
        }
        ALOGD("%s: ---- Feature BATTERY_LISTENER is enabled ----", __func__);
        return;
    }

    feature_disabled:
    if (batt_listener_lib_handle) {
        dlclose(batt_listener_lib_handle);
        batt_listener_lib_handle = NULL;
    }

    batt_listener_init = NULL;
    batt_listener_deinit = NULL;
    batt_prop_is_charging = NULL;
    ALOGW(":: %s: ---- Feature BATTERY_LISTENER is disabled ----", __func__);
}

void AudioExtn::battery_properties_listener_init(battery_status_change_fn_t fn)
{
    if(batt_listener_init)
        batt_listener_init(fn);
}
void AudioExtn::battery_properties_listener_deinit()
{
    if(batt_listener_deinit)
        batt_listener_deinit();
}
bool AudioExtn::battery_properties_is_charging()
{
    return (batt_prop_is_charging)? batt_prop_is_charging(): false;
}
// END: BATTERY_LISTENER ================================================================