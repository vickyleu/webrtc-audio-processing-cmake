//
// Created by user on 3/14/25.
//
#include "webrtc_apm_wrapper.h"

#include <modules/audio_processing/include/audio_processing.h>
#include <modules/audio_processing/aec_dump/aec_dump_factory.h>
#include <rtc_base/logging.h>

class WebRTCApm {
public:
    ~WebRTCApm() {
        delete apm;
    }
    webrtc::AudioProcessing* apm{nullptr};
    webrtc::StreamConfig input_stream_config;
    webrtc::StreamConfig output_stream_config;
    std::unique_ptr<webrtc::AecDump> aec_dump;  // 调试录音
};

void* webrtc_apm_create()
{
    auto *handle = new WebRTCApm();
    handle->apm = webrtc::AudioProcessingBuilder().Create();
    return handle;
}

void webrtc_apm_destroy(void* apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (handle) {
        delete handle;
    }
}

void webrtc_apm_prepare(void* apm, int sample_rate, int channels) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }

    handle->input_stream_config = webrtc::StreamConfig(sample_rate, channels);
    handle->output_stream_config = webrtc::StreamConfig(sample_rate, channels);
}

void webrtc_apm_apply_config(void *apm, const APMConfig *config) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !config) {
        return;
    }

    webrtc::AudioProcessing::Config cfg;

    // Echo canceller
    cfg.echo_canceller.enabled = config->echo_canceller.enabled;
    cfg.echo_canceller.mobile_mode = config->echo_canceller.mobile_mode;
    cfg.echo_canceller.export_linear_aec_output =
            config->echo_canceller.export_linear_aec_output;
    cfg.echo_canceller.enforce_high_pass_filtering =
            config->echo_canceller.enforce_high_pass_filtering;

    // Noise suppression
    cfg.noise_suppression.enabled = config->noise_suppression.enabled;
    cfg.noise_suppression.level = static_cast<webrtc::AudioProcessing::Config::NoiseSuppression::Level>(
            config->noise_suppression.level);

    // High-pass filter
    cfg.high_pass_filter.enabled = config->high_pass_filter.enabled;

    // Gain controller 1 (classic AGC)
    cfg.gain_controller1.enabled = config->gain_controller.enabled;
    cfg.gain_controller1.mode = static_cast<webrtc::AudioProcessing::Config::GainController1::Mode>(
            config->gain_controller.mode);
    cfg.gain_controller1.target_level_dbfs = config->gain_controller.target_level_dbfs;
    cfg.gain_controller1.compression_gain_db = config->gain_controller.compression_gain_db;
    cfg.gain_controller1.enable_limiter = config->gain_controller.enable_limiter;

    // Gain controller 2 (modern AGC) - 新增
    cfg.gain_controller2.enabled = config->gain_controller2.enabled;
    cfg.gain_controller2.adaptive_digital.enabled = config->gain_controller2.adaptive_digital.enabled;
    cfg.gain_controller2.adaptive_digital.max_gain_db = config->gain_controller2.adaptive_digital.max_gain_db;
    cfg.gain_controller2.adaptive_digital.initial_saturation_margin_db =
            config->gain_controller2.adaptive_digital.initial_saturation_margin_db;
    cfg.gain_controller2.adaptive_digital.extra_saturation_margin_db =
            config->gain_controller2.adaptive_digital.extra_saturation_margin_db;
    cfg.gain_controller2.adaptive_digital.gain_applier_adjacent_speech_frames_threshold =
            config->gain_controller2.adaptive_digital.gain_applier_adjacent_speech_frames_threshold;
    cfg.gain_controller2.adaptive_digital.max_gain_change_db_per_second =
            config->gain_controller2.adaptive_digital.max_gain_change_db_per_second;
    cfg.gain_controller2.adaptive_digital.max_output_noise_level_dbfs =
            config->gain_controller2.adaptive_digital.max_output_noise_level_dbfs;

    cfg.gain_controller2.fixed_digital.enabled = config->gain_controller2.fixed_digital.enabled;
    cfg.gain_controller2.fixed_digital.gain_db = config->gain_controller2.fixed_digital.gain_db;

    // Voice detection
    cfg.voice_detection.enabled = config->voice_detection.enabled;

    // Transient suppression
    cfg.transient_suppression.enabled = config->transient_suppression.enabled;

    // Residual echo detector
    cfg.residual_echo_detector.enabled = config->residual_echo_detector.enabled;

    // Level estimation
    cfg.level_estimation.enabled = config->level_estimation.enabled;

    // Pre-amplifier
    cfg.pre_amplifier.enabled = config->pre_amplifier.enabled;
    cfg.pre_amplifier.fixed_gain_factor = config->pre_amplifier.fixed_gain_factor;

    // Capture post processor - 新增
    cfg.capture_post_processor.enabled = config->capture_post_processor.enabled;

    // Render pre processor - 新增
    cfg.render_pre_processor.enabled = config->render_pre_processor.enabled;

    handle->apm->ApplyConfig(cfg);
}

void webrtc_apm_process_reverse_stream(void* apm, const float* const* src, float* const* dest) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }

    handle->apm->ProcessReverseStream(src, handle->input_stream_config, handle->output_stream_config, dest);
}

void webrtc_apm_process_stream(void* apm, const float* const* src, float* const* dest) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }

    handle->apm->ProcessStream(src, handle->input_stream_config, handle->output_stream_config, dest);
}

// --------------- 基础运行期接口 ----------------
void webrtc_apm_set_stream_analog_level(void *apm, int level) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    handle->apm->set_stream_analog_level(level);
}

int webrtc_apm_get_stream_analog_level(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return 0;
    }
    return handle->apm->recommended_stream_analog_level();
}

bool webrtc_apm_voice_detected(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return false;
    }
    auto stats = handle->apm->GetStatistics();
    if (stats.voice_detected) {
        return *stats.voice_detected;
    }
    return false;
}

// 新增：获取语音概率
float webrtc_apm_get_voice_probability(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return 0.0f;
    }
    auto stats = handle->apm->GetStatistics();
    // WebRTC内部没有直接的概率值，通过其他指标估算
    if (stats.voice_detected && *stats.voice_detected) {
        return 0.8f; // 检测到语音时返回高概率
    }
    return 0.2f; // 未检测到语音时返回低概率
}

// 延迟补偿接口实现
void webrtc_apm_set_stream_delay_ms(void *apm, int delay_ms) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    handle->apm->set_stream_delay_ms(delay_ms);
}

int webrtc_apm_get_stream_delay_ms(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return 0;
    }
    return handle->apm->stream_delay_ms();
}

// 键盘/按键提示接口实现
void webrtc_apm_set_key_pressed(void *apm, bool key_pressed) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    handle->apm->set_stream_key_pressed(key_pressed);
}

// --------------- 快捷开关接口 ----------------
void webrtc_apm_enable_aec(void *apm, bool enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.echo_canceller.enabled = enable;
    handle->apm->ApplyConfig(cfg);
}

void webrtc_apm_enable_ns(void *apm, bool enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.noise_suppression.enabled = enable;
    handle->apm->ApplyConfig(cfg);
}

void webrtc_apm_enable_agc(void *apm, bool enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.gain_controller1.enabled = enable;
    handle->apm->ApplyConfig(cfg);
}

void webrtc_apm_enable_vad(void *apm, bool enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.voice_detection.enabled = enable;
    handle->apm->ApplyConfig(cfg);
}

// --------------- 动态参数调节接口 ----------------
void webrtc_apm_set_ns_level(void *apm, APMNsLevel level) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.noise_suppression.level = static_cast<webrtc::AudioProcessing::Config::NoiseSuppression::Level>(level);
    handle->apm->ApplyConfig(cfg);
}

void webrtc_apm_set_agc_target_level(void *apm, int target_dbfs) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.gain_controller1.target_level_dbfs = target_dbfs;
    handle->apm->ApplyConfig(cfg);
}

void webrtc_apm_set_pre_amplifier_gain(void *apm, float gain_factor) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.pre_amplifier.fixed_gain_factor = gain_factor;
    handle->apm->ApplyConfig(cfg);
}

// --------------- 统计信息接口 ----------------
APMStatistics webrtc_apm_get_statistics(void *apm) {
    APMStatistics stats = {};
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return stats;
    }

    auto webrtc_stats = handle->apm->GetStatistics();

    // VAD相关
    if (webrtc_stats.voice_detected) {
        stats.voice_detected = *webrtc_stats.voice_detected;
        stats.voice_probability = stats.voice_detected ? 0.8f : 0.2f;
    }

    // 回声相关
    if (webrtc_stats.residual_echo_likelihood) {
        stats.residual_echo_likelihood = *webrtc_stats.residual_echo_likelihood;
        stats.residual_echo_detected = stats.residual_echo_likelihood > 0.5f;
    }

    if (webrtc_stats.echo_return_loss) {
        stats.echo_return_loss = *webrtc_stats.echo_return_loss;
    }

    if (webrtc_stats.echo_return_loss_enhancement) {
        stats.echo_return_loss_enhancement = *webrtc_stats.echo_return_loss_enhancement;
    }

    // 延迟相关
    if (webrtc_stats.delay_median_ms) {
        stats.delay_median_ms = *webrtc_stats.delay_median_ms;
    }

    if (webrtc_stats.delay_standard_deviation_ms) {
        stats.delay_standard_deviation_ms = *webrtc_stats.delay_standard_deviation_ms;
    }

    // 增益相关
    stats.recommended_analog_level = handle->apm->recommended_stream_analog_level();

    return stats;
}

void webrtc_apm_reset_statistics(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    // WebRTC没有直接的重置统计接口，通过重新应用配置来实现
    auto cfg = handle->apm->GetConfig();
    handle->apm->ApplyConfig(cfg);
}

// --------------- 高级接口 ----------------
int webrtc_apm_process_stream_with_result(void *apm, const float *const *src, float *const *dest) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return -1;
    }

    int result = handle->apm->ProcessStream(src, handle->input_stream_config, handle->output_stream_config, dest);
    return result;
}

bool webrtc_apm_is_saturated(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return false;
    }

    auto stats = handle->apm->GetStatistics();
    // 通过推荐的模拟电平判断是否饱和
    int recommended_level = handle->apm->recommended_stream_analog_level();
    return recommended_level <= 0; // 推荐电平很低时可能表示饱和
}

float webrtc_apm_get_speech_level_dbfs(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return -100.0f;
    }

    auto stats = handle->apm->GetStatistics();
    if (stats.output_rms_dbfs) {
        return *stats.output_rms_dbfs;
    }
    return -100.0f;
}

float webrtc_apm_get_noise_level_dbfs(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return -100.0f;
    }

    // WebRTC没有直接的噪声电平，这里返回一个估算值
    // 可以根据语音检测状态和输出电平来估算
    auto stats = handle->apm->GetStatistics();
    if (stats.output_rms_dbfs && stats.voice_detected && !*stats.voice_detected) {
        return *stats.output_rms_dbfs; // 非语音时段的输出可视为噪声
    }
    return -60.0f; // 默认噪声电平
}

// --------------- 调试和监控接口 ----------------
void webrtc_apm_enable_debug_recording(void *apm, const char* file_path) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !file_path) {
        return;
    }

    // 创建AEC dump用于调试
    handle->aec_dump = webrtc::AecDumpFactory::Create(file_path, -1, nullptr);
    if (handle->aec_dump) {
        handle->apm->AttachAecDump(std::move(handle->aec_dump));
    }
}

void webrtc_apm_disable_debug_recording(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }

    handle->apm->DetachAecDump();
    handle->aec_dump.reset();
}