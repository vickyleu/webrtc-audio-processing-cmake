//
// Created by user on 3/14/25.
//
#include "webrtc_apm_wrapper.h"

#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_processing/aec_dump/aec_dump_factory.h"
#include "rtc_base/logging.h"
#include <memory>
#include <algorithm>
#include <vector>

class WebRTCApm {
public:
    ~WebRTCApm() {
        delete apm;
    }
    webrtc::AudioProcessing* apm{nullptr};
    webrtc::StreamConfig input_stream_config;
    webrtc::StreamConfig output_stream_config;
    std::unique_ptr<webrtc::AecDump> aec_dump;  // 调试录音

    // 存储扩展配置用于后续使用
    APMConfigVoiceProbability voice_prob_config;
    APMConfigSaturationDetection saturation_config;
    APMConfigNoiseEstimation noise_config;
    
    // 新增成员变量
    APMConfigMultiChannel multi_channel_config;
    APMConfigPerformance performance_config;
    APMErrorCode last_error;
    int quality_monitoring_enabled;
    int voice_assistant_mode;
    int far_field_optimization;
    float adaptation_speed;
    
    // 回调函数
    APMAudioEventCallback audio_event_callback;
    void* audio_event_user_data;
    APMProcessingCallback processing_callback;
    void* processing_user_data;
    
    // 线性AEC输出缓冲
    std::vector<float> linear_aec_output_buffer;
    int linear_aec_output_available;
    
    // 音频质量评估数据
    APMAudioQuality audio_quality;
    
    // 音频流分析数据
    APMStreamAnalysis stream_analysis;
    
    // 预处理链配置
    APMPreprocessingChain preprocessing_chain;
};

void* webrtc_apm_create()
{
    auto *handle = new WebRTCApm();
    handle->apm = webrtc::AudioProcessingBuilder().Create();

    // 初始化扩展配置为默认值
    handle->voice_prob_config.high_confidence_threshold = 0.8f;
    handle->voice_prob_config.low_confidence_threshold = 0.2f;
    handle->voice_prob_config.use_advanced_estimation = 0;  // false

    handle->saturation_config.low_level_threshold = 0;
    handle->saturation_config.rms_threshold_dbfs = -3.0f;
    handle->saturation_config.enable_multi_criteria_detection = 1;  // true

    handle->noise_config.default_noise_level_dbfs = -60.0f;
    handle->noise_config.estimation_window_ms = 100;
    handle->noise_config.enable_adaptive_estimation = 1;  // true
    
    // 初始化新增成员变量
    handle->last_error = APM_ERROR_NONE;
    handle->quality_monitoring_enabled = 0;  // false
    handle->voice_assistant_mode = 0;        // false
    handle->far_field_optimization = 0;      // false
    handle->adaptation_speed = 0.5f;
    handle->audio_event_callback = nullptr;
    handle->audio_event_user_data = nullptr;
    handle->processing_callback = nullptr;
    handle->processing_user_data = nullptr;
    handle->linear_aec_output_available = 0;  // false
    
    // 初始化多通道配置
    handle->multi_channel_config.enable_multi_channel_processing = 0;  // false
    handle->multi_channel_config.num_channels = 1;
    handle->multi_channel_config.enable_channel_mixing = 0;            // false
    handle->multi_channel_config.enable_spatial_processing = 0;        // false
    handle->multi_channel_config.spatial_audio.enabled = 0;            // false
    handle->multi_channel_config.spatial_audio.reference_channel_weight = 1.0f;
    handle->multi_channel_config.spatial_audio.enable_beamforming = 0; // false
    handle->multi_channel_config.spatial_audio.beam_width_degrees = 60.0f;
    
    // 初始化性能配置
    handle->performance_config.enable_low_latency_mode = 0;      // false
    handle->performance_config.enable_background_processing = 0; // false
    handle->performance_config.processing_priority = 5;
    handle->performance_config.enable_simd_optimizations = 1;    // true
    handle->performance_config.max_processing_delay_ms = 50;
    
    // 初始化音频质量数据
    handle->audio_quality.signal_to_noise_ratio_db = 0.0f;
    handle->audio_quality.speech_intelligibility_score = 0.0f;
    handle->audio_quality.distortion_level = 0.0f;
    handle->audio_quality.background_noise_level_dbfs = -60.0f;
    handle->audio_quality.audio_dropouts_detected = 0;  // false
    handle->audio_quality.dynamic_range_db = 0.0f;
    
    // 初始化音频流分析数据
    handle->stream_analysis.contains_speech = 0;  // false
    handle->stream_analysis.contains_music = 0;   // false
    handle->stream_analysis.contains_noise = 0;   // false
    handle->stream_analysis.dominant_frequency_hz = 0.0f;
    handle->stream_analysis.spectral_centroid_hz = 0.0f;
    handle->stream_analysis.zero_crossing_rate = 0.0f;
    handle->stream_analysis.clipping_detected = 0;  // false
    
    // 初始化预处理链配置
    handle->preprocessing_chain.enable_dc_removal = 0;                      // false
    handle->preprocessing_chain.enable_wind_noise_reduction = 0;            // false
    handle->preprocessing_chain.enable_click_removal = 0;                   // false
    handle->preprocessing_chain.enable_automatic_gain_normalization = 0;    // false
    handle->preprocessing_chain.custom_high_pass.enabled = 0;               // false
    handle->preprocessing_chain.custom_high_pass.cutoff_frequency_hz = 80.0f;
    handle->preprocessing_chain.custom_high_pass.order = 2;

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

    // Echo canceller - 使用advanced配置的basic字段
    cfg.echo_canceller.enabled = config->echo_canceller_advanced.basic.enabled ? true : false;
    cfg.echo_canceller.mobile_mode = config->echo_canceller_advanced.basic.mobile_mode ? true : false;
    cfg.echo_canceller.export_linear_aec_output =
            config->echo_canceller_advanced.basic.export_linear_aec_output ? true : false;
    cfg.echo_canceller.enforce_high_pass_filtering =
            config->echo_canceller_advanced.basic.enforce_high_pass_filtering ? true : false;

    // Noise suppression
    cfg.noise_suppression.enabled = config->noise_suppression.enabled ? true : false;
    cfg.noise_suppression.level = static_cast<webrtc::AudioProcessing::Config::NoiseSuppression::Level>(
            config->noise_suppression.level);

    // High-pass filter
    cfg.high_pass_filter.enabled = config->high_pass_filter.enabled ? true : false;

    // Gain controller 1 (classic AGC)
    cfg.gain_controller1.enabled = config->gain_controller.enabled ? true : false;
    cfg.gain_controller1.mode = static_cast<webrtc::AudioProcessing::Config::GainController1::Mode>(
            config->gain_controller.mode);
    cfg.gain_controller1.target_level_dbfs = config->gain_controller.target_level_dbfs;
    cfg.gain_controller1.compression_gain_db = config->gain_controller.compression_gain_db;
    cfg.gain_controller1.enable_limiter = config->gain_controller.enable_limiter ? true : false;

    // Gain controller 2 (modern AGC) - 使用配置参数而非写死
    cfg.gain_controller2.enabled = config->gain_controller2.enabled ? true : false;
    cfg.gain_controller2.adaptive_digital.enabled = config->gain_controller2.adaptive_digital.enabled ? true : false;
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

    cfg.gain_controller2.fixed_digital.gain_db = config->gain_controller2.fixed_digital.gain_db;

    // Voice detection - 使用advanced配置的basic字段
    cfg.voice_detection.enabled = config->voice_detection_advanced.basic.enabled ? true : false;

    // Transient suppression
    cfg.transient_suppression.enabled = config->transient_suppression.enabled ? true : false;

    // Residual echo detector
    cfg.residual_echo_detector.enabled = config->residual_echo_detector.enabled ? true : false;

    // Level estimation
    cfg.level_estimation.enabled = config->level_estimation.enabled ? true : false;

    // Pre-amplifier
    cfg.pre_amplifier.enabled = config->pre_amplifier.enabled ? true : false;
    cfg.pre_amplifier.fixed_gain_factor = config->pre_amplifier.fixed_gain_factor;

    // 存储扩展配置
    handle->voice_prob_config = config->voice_probability;
    handle->saturation_config = config->saturation_detection;
    handle->noise_config = config->noise_estimation;

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

int webrtc_apm_voice_detected(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return 0;  // false
    }
    auto stats = handle->apm->GetStatistics();
    if (stats.voice_detected) {
        return *stats.voice_detected ? 1 : 0;
    }
    return 0;  // false
}

// 改进的语音概率获取 - 使用配置参数
float webrtc_apm_get_voice_probability(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return 0.0f;
    }
    return webrtc_apm_get_voice_probability_ex(apm, &handle->voice_prob_config);
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
void webrtc_apm_set_key_pressed(void *apm, int key_pressed) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    handle->apm->set_stream_key_pressed(key_pressed ? true : false);
}

// --------------- 快捷开关接口 ----------------
void webrtc_apm_enable_aec(void *apm, int enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.echo_canceller.enabled = enable ? true : false;
    handle->apm->ApplyConfig(cfg);
}

void webrtc_apm_enable_ns(void *apm, int enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.noise_suppression.enabled = enable ? true : false;
    handle->apm->ApplyConfig(cfg);
}

void webrtc_apm_enable_agc(void *apm, int enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.gain_controller1.enabled = enable ? true : false;
    handle->apm->ApplyConfig(cfg);
}

void webrtc_apm_enable_vad(void *apm, int enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    auto cfg = handle->apm->GetConfig();
    cfg.voice_detection.enabled = enable ? true : false;
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

    // VAD相关 - 使用配置参数
    if (webrtc_stats.voice_detected) {
        stats.voice_detected = *webrtc_stats.voice_detected ? 1 : 0;
        stats.voice_probability = stats.voice_detected ?
                                  handle->voice_prob_config.high_confidence_threshold :
                                  handle->voice_prob_config.low_confidence_threshold;
    }

    // 回声相关
    if (webrtc_stats.residual_echo_likelihood) {
        stats.residual_echo_likelihood = *webrtc_stats.residual_echo_likelihood;
        stats.residual_echo_detected = stats.residual_echo_likelihood > 0.5f ? 1 : 0;
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
    stats.saturation_detected = webrtc_apm_is_saturated_ex(apm, &handle->saturation_config);

    return stats;
}

void webrtc_apm_reset_statistics(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
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

int webrtc_apm_is_saturated(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return 0;  // false
    }
    return webrtc_apm_is_saturated_ex(apm, &handle->saturation_config);
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
    return webrtc_apm_get_noise_level_dbfs_ex(apm, &handle->noise_config);
}

// --------------- 调试和监控接口 ----------------
void webrtc_apm_enable_debug_recording(void *apm, const char* file_path) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !file_path) {
        return;
    }

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

// --------------- 配置管理接口 ----------------
APMConfig webrtc_apm_get_default_config() {
    APMConfig config = {};

    // Echo Canceller defaults
    config.echo_canceller_advanced.basic.enabled = 0;              // false
    config.echo_canceller_advanced.basic.mobile_mode = 0;          // false
    config.echo_canceller_advanced.basic.export_linear_aec_output = 0;  // false
    config.echo_canceller_advanced.basic.enforce_high_pass_filtering = 1;  // true

    // Noise Suppression defaults
    config.noise_suppression.enabled = 0;  // false
    config.noise_suppression.level = kNsModerate;

    // High Pass Filter defaults
    config.high_pass_filter.enabled = 0;  // false

    // Gain Controller 1 defaults
    config.gain_controller.enabled = 0;  // false
    config.gain_controller.mode = kAgcAdaptiveAnalog;
    config.gain_controller.target_level_dbfs = 3;
    config.gain_controller.compression_gain_db = 9;
    config.gain_controller.enable_limiter = 1;  // true

    // Gain Controller 2 defaults
    config.gain_controller2.enabled = 0;  // false
    config.gain_controller2.adaptive_digital.enabled = 1;  // true
    config.gain_controller2.adaptive_digital.initial_saturation_margin_db = 20.0f;
    config.gain_controller2.adaptive_digital.extra_saturation_margin_db = 2;
    config.gain_controller2.adaptive_digital.gain_applier_adjacent_speech_frames_threshold = 1.0f;
    config.gain_controller2.adaptive_digital.max_gain_change_db_per_second = 3.0f;
    config.gain_controller2.adaptive_digital.max_output_noise_level_dbfs = -50.0f;
    config.gain_controller2.fixed_digital.gain_db = 0.0f;

    // Voice Detection defaults
    config.voice_detection_advanced.basic.enabled = 0;  // false

    // Transient Suppression defaults
    config.transient_suppression.enabled = 0;  // false

    // Residual Echo Detector defaults
    config.residual_echo_detector.enabled = 1;  // true

    // Level Estimation defaults
    config.level_estimation.enabled = 0;  // false

    // Pre Amplifier defaults
    config.pre_amplifier.enabled = 0;  // false
    config.pre_amplifier.fixed_gain_factor = 1.0f;

    // Voice Probability defaults
    config.voice_probability.high_confidence_threshold = 0.8f;
    config.voice_probability.low_confidence_threshold = 0.2f;
    config.voice_probability.use_advanced_estimation = 0;  // false

    // Saturation Detection defaults
    config.saturation_detection.low_level_threshold = 0;
    config.saturation_detection.rms_threshold_dbfs = -3.0f;
    config.saturation_detection.enable_multi_criteria_detection = 1;  // true

    // Noise Estimation defaults
    config.noise_estimation.default_noise_level_dbfs = -60.0f;
    config.noise_estimation.estimation_window_ms = 100;
    config.noise_estimation.enable_adaptive_estimation = 1;  // true

    return config;
}

int webrtc_apm_validate_config(const APMConfig* config) {
    if (!config) {
        return 0;  // false
    }

    // 验证 AGC1 参数范围
    if (config->gain_controller.target_level_dbfs < 0 || config->gain_controller.target_level_dbfs > 31) {
        return 0;  // false
    }
    if (config->gain_controller.compression_gain_db < 0 || config->gain_controller.compression_gain_db > 90) {
        return 0;  // false
    }

    // 验证 AGC2 参数
    if (config->gain_controller2.adaptive_digital.initial_saturation_margin_db < 0 ||
        config->gain_controller2.adaptive_digital.initial_saturation_margin_db > 50) {
        return 0;  // false
    }

    // 验证前置放大器增益
    if (config->pre_amplifier.fixed_gain_factor < 0.1f || config->pre_amplifier.fixed_gain_factor > 10.0f) {
        return 0;  // false
    }

    // 验证语音概率阈值
    if (config->voice_probability.high_confidence_threshold <= config->voice_probability.low_confidence_threshold) {
        return 0;  // false
    }
    if (config->voice_probability.high_confidence_threshold < 0.0f || config->voice_probability.high_confidence_threshold > 1.0f) {
        return 0;  // false
    }
    if (config->voice_probability.low_confidence_threshold < 0.0f || config->voice_probability.low_confidence_threshold > 1.0f) {
        return 0;  // false
    }

    return 1;  // true
}

void webrtc_apm_apply_preset(void *apm, APMPresetMode mode) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }

    APMConfig config = webrtc_apm_get_default_config();

    switch (mode) {
        case APM_PRESET_DEFAULT:
            // 使用默认配置
            break;

        case APM_PRESET_CONFERENCE:
            // 会议模式：强调语音清晰度
            config.echo_canceller_advanced.basic.enabled = 1;       // true
            config.noise_suppression.enabled = 1;                   // true
            config.noise_suppression.level = kNsHigh;
            config.gain_controller.enabled = 1;                     // true
            config.gain_controller.target_level_dbfs = 6;
            config.high_pass_filter.enabled = 1;                    // true
            config.voice_detection_advanced.basic.enabled = 1;      // true
            config.transient_suppression.enabled = 1;               // true
            break;

        case APM_PRESET_MUSIC:
            // 音乐模式：保持音频质量
            config.echo_canceller_advanced.basic.enabled = 0;       // false
            config.noise_suppression.enabled = 1;                   // true
            config.noise_suppression.level = kNsLow;
            config.gain_controller.enabled = 0;                     // false
            config.gain_controller2.enabled = 1;                    // true
            config.high_pass_filter.enabled = 0;                    // false
            config.transient_suppression.enabled = 0;               // false
            break;

        case APM_PRESET_SPEECH:
            // 语音通话模式：平衡处理
            config.echo_canceller_advanced.basic.enabled = 1;       // true
            config.noise_suppression.enabled = 1;                   // true
            config.noise_suppression.level = kNsModerate;
            config.gain_controller.enabled = 1;                     // true
            config.gain_controller.target_level_dbfs = 3;
            config.high_pass_filter.enabled = 1;                    // true
            config.voice_detection_advanced.basic.enabled = 1;      // true
            break;

        case APM_PRESET_LOW_LATENCY:
            // 低延迟模式：最小化处理
            config.echo_canceller_advanced.basic.enabled = 1;       // true
            config.echo_canceller_advanced.basic.mobile_mode = 1;   // true，移动模式延迟更低
            config.noise_suppression.enabled = 1;                   // true
            config.noise_suppression.level = kNsLow;
            config.gain_controller.enabled = 0;                     // false
            config.gain_controller2.enabled = 1;                    // true，AGC2 延迟更低
            config.high_pass_filter.enabled = 0;                    // false
            config.transient_suppression.enabled = 0;               // false
            break;
            
        case APM_PRESET_VOICE_ASSISTANT:
            // 语音助手模式：优化远场语音处理
            config.echo_canceller_advanced.basic.enabled = 1;                    // true
            config.echo_canceller_advanced.basic.mobile_mode = 0;                // false
            config.echo_canceller_advanced.basic.export_linear_aec_output = 1;   // true，导出线性AEC输出用于唤醒词检测
            config.noise_suppression.enabled = 1;                                // true
            config.noise_suppression.level = kNsHigh;                            // 强噪声抑制
            config.gain_controller.enabled = 1;                                  // true
            config.gain_controller.target_level_dbfs = 6;                        // 更保守的增益目标
            config.gain_controller2.enabled = 1;                                 // true
            config.high_pass_filter.enabled = 1;                                 // true，去除低频噪声
            config.voice_detection_advanced.basic.enabled = 1;                   // true，启用VAD
            config.transient_suppression.enabled = 1;                            // true，抑制键盘点击等瞬时噪声
            config.residual_echo_detector.enabled = 1;                           // true
            config.level_estimation.enabled = 1;                                 // true，用于音频质量评估
            config.pre_amplifier.enabled = 1;                                    // true
            config.pre_amplifier.fixed_gain_factor = 1.5f;                       // 适当的前置增益
            
            // 设置语音助手特定配置
            handle->voice_assistant_mode = 1;        // true
            handle->far_field_optimization = 1;      // true
            handle->quality_monitoring_enabled = 1;  // true
            break;
    }

    webrtc_apm_apply_config(apm, &config);
}

// --------------- 扩展接口实现 ----------------
float webrtc_apm_get_voice_probability_ex(void *apm, const APMConfigVoiceProbability* config) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !config) {
        return 0.0f;
    }

    auto stats = handle->apm->GetStatistics();

    if (config->use_advanced_estimation) {
        // 高级估算：结合多个指标
        int voice_detected = 0;  // false，改为int
        float base_probability = config->low_confidence_threshold;

        if (stats.voice_detected) {
            voice_detected = *stats.voice_detected ? 1 : 0;
        }

        if (voice_detected) {
            base_probability = config->high_confidence_threshold;

            // 根据输出电平调整概率
            if (stats.output_rms_dbfs) {
                float rms = *stats.output_rms_dbfs;
                if (rms > -20.0f) {
                    base_probability = std::min(1.0f, base_probability + 0.1f);
                } else if (rms < -40.0f) {
                    base_probability = std::max(0.0f, base_probability - 0.2f);
                }
            }
        }

        return base_probability;
    } else {
        // 简单估算
        if (stats.voice_detected && *stats.voice_detected) {
            return config->high_confidence_threshold;
        }
        return config->low_confidence_threshold;
    }
}

int webrtc_apm_is_saturated_ex(void *apm, const APMConfigSaturationDetection* config) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !config) {
        return 0;  // false
    }

    int saturated = 0;  // false

    // 检查推荐的模拟电平
    int recommended_level = handle->apm->recommended_stream_analog_level();
    if (recommended_level <= config->low_level_threshold) {
        saturated = 1;  // true
    }

    if (config->enable_multi_criteria_detection) {
        // 多重标准检测
        auto stats = handle->apm->GetStatistics();

        // 检查RMS电平
        if (stats.output_rms_dbfs) {
            float rms = *stats.output_rms_dbfs;
            if (rms > config->rms_threshold_dbfs) {
                saturated = 1;  // true
            }
        }

        // 可以添加更多检测标准
        // 例如：检查削波、失真等
    }

    return saturated;
}

float webrtc_apm_get_noise_level_dbfs_ex(void *apm, const APMConfigNoiseEstimation* config) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !config) {
        return -100.0f;
    }

    auto stats = handle->apm->GetStatistics();

    if (config->enable_adaptive_estimation) {
        // 自适应噪声估算
        int voice_detected = 0;  // false，改为int
        if (stats.voice_detected) {
            voice_detected = *stats.voice_detected ? 1 : 0;
        }

        if (!voice_detected && stats.output_rms_dbfs) {
            // 在非语音段，输出RMS可以作为噪声电平的估算
            float current_level = *stats.output_rms_dbfs;

            // 简单的滑动平均滤波（这里简化处理）
            static float estimated_noise = config->default_noise_level_dbfs;
            const float alpha = 0.1f; // 滤波系数
            estimated_noise = alpha * current_level + (1.0f - alpha) * estimated_noise;

            return estimated_noise;
        }
    }

    return config->default_noise_level_dbfs;
}

// =============== 新增功能实现 ===============

// 实时设置
void webrtc_apm_set_runtime_setting(void *apm, const APMRuntimeSetting *setting) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !setting) {
        if (handle) handle->last_error = APM_ERROR_INVALID_PARAMETER;
        return;
    }

    webrtc::AudioProcessing::RuntimeSetting webrtc_setting;
    
    switch (setting->type) {
        case APM_RUNTIME_CAPTURE_PRE_GAIN:
            webrtc_setting = webrtc::AudioProcessing::RuntimeSetting::CreateCapturePreGain(setting->value.float_value);
            break;
        case APM_RUNTIME_COMPRESSION_GAIN:
            webrtc_setting = webrtc::AudioProcessing::RuntimeSetting::CreateCompressionGainDb(setting->value.int_value);
            break;
        case APM_RUNTIME_CAPTURE_FIXED_POST_GAIN:
            webrtc_setting = webrtc::AudioProcessing::RuntimeSetting::CreateCaptureFixedPostGain(setting->value.float_value);
            break;
        case APM_RUNTIME_PLAYOUT_VOLUME_CHANGE:
            webrtc_setting = webrtc::AudioProcessing::RuntimeSetting::CreatePlayoutVolumeChange(setting->value.int_value);
            break;
        case APM_RUNTIME_PLAYOUT_AUDIO_DEVICE_CHANGE: {
            webrtc::AudioProcessing::RuntimeSetting::PlayoutAudioDeviceInfo device_info;
            device_info.id = setting->value.audio_device.device_id;
            device_info.max_volume = setting->value.audio_device.max_volume;
            webrtc_setting = webrtc::AudioProcessing::RuntimeSetting::CreatePlayoutAudioDeviceChange(device_info);
            break;
        }
        default:
            handle->last_error = APM_ERROR_UNSUPPORTED_OPERATION;
            return;
    }
    
    handle->apm->SetRuntimeSetting(webrtc_setting);
    handle->last_error = APM_ERROR_NONE;
}

// 线性AEC输出
int webrtc_apm_get_linear_aec_output(void *apm, float **output, int *length) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !output || !length) {
        if (handle) handle->last_error = APM_ERROR_INVALID_PARAMETER;
        return 0;  // false
    }

    // WebRTC GetLinearAecOutput需要特定的数组视图
    std::array<float, 160> linear_output_frame;
    rtc::ArrayView<std::array<float, 160>> linear_output_view(&linear_output_frame, 1);
    
    bool has_output = handle->apm->GetLinearAecOutput(linear_output_view);  // 保持bool，因为这是WebRTC API返回值
    
    if (has_output) {
        handle->linear_aec_output_buffer.assign(linear_output_frame.begin(), linear_output_frame.end());
        *output = handle->linear_aec_output_buffer.data();
        *length = static_cast<int>(handle->linear_aec_output_buffer.size());
        handle->linear_aec_output_available = 1;  // true
    } else {
        *output = nullptr;
        *length = 0;
        handle->linear_aec_output_available = 0;  // false
    }
    
    handle->last_error = APM_ERROR_NONE;
    return has_output ? 1 : 0;  // 转换为int返回值
}

// 多通道配置
void webrtc_apm_set_multi_channel_config(void *apm, const APMConfigMultiChannel *config) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !config) {
        if (handle) handle->last_error = APM_ERROR_INVALID_PARAMETER;
        return;
    }
    
    handle->multi_channel_config = *config;
    
    // 根据多通道配置调整WebRTC配置
    auto apm_config = handle->apm->GetConfig();
    apm_config.pipeline.multi_channel_capture = config->enable_multi_channel_processing;
    apm_config.pipeline.multi_channel_render = config->enable_multi_channel_processing;
    handle->apm->ApplyConfig(apm_config);
    
    handle->last_error = APM_ERROR_NONE;
}

// 多通道流处理
int webrtc_apm_process_multi_channel_stream(void *apm, const float *const *src_channels, 
                                           int num_input_channels, float *const *dest_channels, 
                                           int num_output_channels) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !src_channels || !dest_channels) {
        if (handle) handle->last_error = APM_ERROR_INVALID_PARAMETER;
        return -1;
    }
    
    if (!handle->multi_channel_config.enable_multi_channel_processing) {
        handle->last_error = APM_ERROR_UNSUPPORTED_OPERATION;
        return -1;
    }
    
    // 创建多通道流配置
    webrtc::StreamConfig input_config(handle->input_stream_config.sample_rate_hz(), num_input_channels);
    webrtc::StreamConfig output_config(handle->output_stream_config.sample_rate_hz(), num_output_channels);
    
    int result = handle->apm->ProcessStream(src_channels, input_config, output_config, dest_channels);
    
    handle->last_error = (result == 0) ? APM_ERROR_NONE : APM_ERROR_PROCESSING_FAILED;
    return result;
}

// 音频质量评估
APMAudioQuality webrtc_apm_assess_audio_quality(void *apm) {
    APMAudioQuality quality = {};
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return quality;
    }
    
    if (!handle->quality_monitoring_enabled) {
        handle->last_error = APM_ERROR_UNSUPPORTED_OPERATION;
        return quality;
    }
    
    auto stats = handle->apm->GetStatistics();
    
    // 计算信噪比
    if (stats.output_rms_dbfs && stats.residual_echo_likelihood) {
        float signal_level = *stats.output_rms_dbfs;
        float noise_estimate = handle->audio_quality.background_noise_level_dbfs;
        quality.signal_to_noise_ratio_db = signal_level - noise_estimate;
    }
    
    // 语音清晰度评分（基于多个指标）
    if (stats.voice_detected) {
        float voice_prob = *stats.voice_detected ? 1.0f : 0.0f;
        float echo_factor = stats.residual_echo_likelihood ? (1.0f - *stats.residual_echo_likelihood) : 1.0f;
        quality.speech_intelligibility_score = voice_prob * echo_factor * 0.8f; // 简化计算
    }
    
    // 失真程度（基于回声和增益控制）
    if (stats.residual_echo_likelihood) {
        quality.distortion_level = *stats.residual_echo_likelihood;
    }
    
    // 背景噪声电平
    quality.background_noise_level_dbfs = webrtc_apm_get_noise_level_dbfs_ex(apm, &handle->noise_config);
    
    // 动态范围（简化计算）
    if (stats.output_rms_dbfs) {
        quality.dynamic_range_db = std::max(0.0f, *stats.output_rms_dbfs - quality.background_noise_level_dbfs);
    }
    
    handle->audio_quality = quality;
    handle->last_error = APM_ERROR_NONE;
    return quality;
}

// 启用质量监控
void webrtc_apm_enable_quality_monitoring(void *apm, int enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    
    handle->quality_monitoring_enabled = enable;
    
    // 启用相关的统计功能
    auto config = handle->apm->GetConfig();
    config.level_estimation.enabled = enable ? true : false;
    config.residual_echo_detector.enabled = enable ? true : false;
    handle->apm->ApplyConfig(config);
    
    handle->last_error = APM_ERROR_NONE;
}

// 扩展统计信息
APMStatisticsExtended webrtc_apm_get_extended_statistics(void *apm) {
    APMStatisticsExtended ext_stats = {};
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return ext_stats;
    }
    
    // 获取基础统计信息
    ext_stats.basic = webrtc_apm_get_statistics(apm);
    
    auto stats = handle->apm->GetStatistics();
    
    // 回声消除详细统计
    if (stats.echo_return_loss_enhancement) {
        ext_stats.echo_cancellation.erle_db = *stats.echo_return_loss_enhancement;
    }
    if (stats.echo_return_loss) {
        ext_stats.echo_cancellation.erl_db = *stats.echo_return_loss;
    }
    if (stats.delay_ms) {
        ext_stats.echo_cancellation.filter_delay_ms = *stats.delay_ms;
    }
    if (stats.divergent_filter_fraction) {
        ext_stats.echo_cancellation.filter_diverged = *stats.divergent_filter_fraction > 0.1f;
    }
    
    // 音频质量评估
    if (handle->quality_monitoring_enabled) {
        ext_stats.audio_quality = webrtc_apm_assess_audio_quality(apm);
    }
    
    // 处理延迟统计（估算值）
    ext_stats.latency.processing_delay_ms = 10; // 假设每帧10ms
    ext_stats.latency.buffer_delay_ms = handle->apm->stream_delay_ms();
    ext_stats.latency.total_delay_ms = ext_stats.latency.processing_delay_ms + ext_stats.latency.buffer_delay_ms;
    ext_stats.latency.real_time_processing = (ext_stats.latency.total_delay_ms < handle->performance_config.max_processing_delay_ms);
    
    handle->last_error = APM_ERROR_NONE;
    return ext_stats;
}

// 错误处理
APMErrorCode webrtc_apm_get_last_error(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return APM_ERROR_INVALID_PARAMETER;
    }
    return handle->last_error;
}

const char* webrtc_apm_get_error_string(APMErrorCode error) {
    switch (error) {
        case APM_ERROR_NONE:
            return "No error";
        case APM_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case APM_ERROR_PROCESSING_FAILED:
            return "Processing failed";
        case APM_ERROR_INITIALIZATION_FAILED:
            return "Initialization failed";
        case APM_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation failed";
        case APM_ERROR_UNSUPPORTED_OPERATION:
            return "Unsupported operation";
        default:
            return "Unknown error";
    }
}

// 事件监听
void webrtc_apm_set_audio_event_callback(void *apm, APMAudioEventCallback callback, void *user_data) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    
    handle->audio_event_callback = callback;
    handle->audio_event_user_data = user_data;
    handle->last_error = APM_ERROR_NONE;
}

void webrtc_apm_set_processing_callback(void *apm, APMProcessingCallback callback, void *user_data) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    
    handle->processing_callback = callback;
    handle->processing_user_data = user_data;
    handle->last_error = APM_ERROR_NONE;
}

// 语音助手专用功能
void webrtc_apm_set_voice_assistant_mode(void *apm, int enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    
    handle->voice_assistant_mode = enable;
    
    if (enable) {
        // 自动应用语音助手预设
        webrtc_apm_apply_preset(apm, APM_PRESET_VOICE_ASSISTANT);
    }
    
    handle->last_error = APM_ERROR_NONE;
}

int webrtc_apm_detect_wake_word_environment(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return 0;  // false
    }
    
    auto stats = handle->apm->GetStatistics();
    
    // 检测是否适合唤醒词检测的环境
    int suitable = 1;  // true
    
    // 检查背景噪声水平
    float noise_level = webrtc_apm_get_noise_level_dbfs_ex(apm, &handle->noise_config);
    if (noise_level > -40.0f) { // 噪声太高
        suitable = 0;  // false
    }
    
    // 检查回声水平
    if (stats.residual_echo_likelihood && *stats.residual_echo_likelihood > 0.3f) {
        suitable = 0;  // false
    }
    
    // 检查饱和检测
    if (webrtc_apm_is_saturated_ex(apm, &handle->saturation_config)) {
        suitable = 0;  // false
    }
    
    handle->last_error = APM_ERROR_NONE;
    return suitable;
}

float webrtc_apm_get_speech_clarity_score(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return 0.0f;
    }
    
    if (!handle->quality_monitoring_enabled) {
        handle->last_error = APM_ERROR_UNSUPPORTED_OPERATION;
        return 0.0f;
    }
    
    APMAudioQuality quality = webrtc_apm_assess_audio_quality(apm);
    handle->last_error = APM_ERROR_NONE;
    return quality.speech_intelligibility_score;
}

void webrtc_apm_optimize_for_far_field(void *apm, int enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    
    handle->far_field_optimization = enable;
    
    auto config = handle->apm->GetConfig();
    
    if (enable) {
        // 远场优化配置
        config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kHigh;
        config.gain_controller.target_level_dbfs = 6; // 更保守的增益
        config.high_pass_filter.enabled = true;
        config.transient_suppression.enabled = true;
        config.pre_amplifier.enabled = true;
        config.pre_amplifier.fixed_gain_factor = 2.0f; // 更高的前置增益
    } else {
        // 恢复默认配置
        config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kModerate;
        config.gain_controller.target_level_dbfs = 3;
        config.pre_amplifier.fixed_gain_factor = 1.0f;
    }
    
    handle->apm->ApplyConfig(config);
    handle->last_error = APM_ERROR_NONE;
}

// 音频流分析（简化版本）
APMStreamAnalysis webrtc_apm_analyze_audio_stream(void *apm, const float *const *audio, int length) {
    APMStreamAnalysis analysis = {};
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !audio) {
        if (handle) handle->last_error = APM_ERROR_INVALID_PARAMETER;
        return analysis;
    }
    
    auto stats = handle->apm->GetStatistics();
    
    // 简化的音频流分析
    if (stats.voice_detected) {
        analysis.contains_speech = *stats.voice_detected;
    }
    
    // 检查削波
    const float* first_channel = audio[0];
    for (int i = 0; i < length; ++i) {
        if (std::abs(first_channel[i]) > 0.95f) {
            analysis.clipping_detected = true;
            break;
        }
    }
    
    // 计算过零率（简化）
    int zero_crossings = 0;
    for (int i = 1; i < length; ++i) {
        if ((first_channel[i-1] >= 0.0f) != (first_channel[i] >= 0.0f)) {
            zero_crossings++;
        }
    }
    analysis.zero_crossing_rate = static_cast<float>(zero_crossings) / length;
    
    // 基于过零率的简单分类
    if (analysis.zero_crossing_rate > 0.1f) {
        analysis.contains_noise = true;
    } else if (analysis.zero_crossing_rate < 0.02f && analysis.contains_speech) {
        analysis.contains_music = true;
    }
    
    handle->stream_analysis = analysis;
    handle->last_error = APM_ERROR_NONE;
    return analysis;
}

// 双讲检测
int webrtc_apm_detect_double_talk_ex(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return 0;  // false
    }
    
    auto stats = handle->apm->GetStatistics();
    
    // 简化的双讲检测：基于回声返回损失和语音检测
    int double_talk = 0;  // false
    
    if (stats.voice_detected && *stats.voice_detected && 
        stats.echo_return_loss && *stats.echo_return_loss < 6.0) {
        double_talk = 1;  // true
    }
    
    handle->last_error = APM_ERROR_NONE;
    return double_talk;
}

// 混响时间估计（简化）
float webrtc_apm_estimate_reverberation_time_ex(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return 0.0f;
    }
    
    auto stats = handle->apm->GetStatistics();
    
    // 基于延迟统计的简化混响时间估计
    float reverberation_time = 0.0f;
    
    if (stats.delay_standard_deviation_ms) {
        // 简化计算：延迟标准差可以反映房间混响特性
        reverberation_time = *stats.delay_standard_deviation_ms * 0.01f; // 转换为秒
    }
    
    handle->last_error = APM_ERROR_NONE;
    return reverberation_time;
}

// 平台优化（占位符实现）
void webrtc_apm_optimize_for_platform(void *apm, const char *platform_info) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return;
    }
    
    // 基于平台信息的优化（简化版本）
    if (platform_info && strstr(platform_info, "mobile")) {
        handle->performance_config.enable_low_latency_mode = true;
        handle->performance_config.processing_priority = 7;
    } else if (platform_info && strstr(platform_info, "server")) {
        handle->performance_config.enable_background_processing = true;
        handle->performance_config.processing_priority = 3;
    }
    
    handle->last_error = APM_ERROR_NONE;
}

// 其他功能的占位符实现...
void webrtc_apm_set_performance_mode(void *apm, int low_latency, int low_power) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) return;
    
    handle->performance_config.enable_low_latency_mode = low_latency;
    // 低功耗模式的实现需要更多WebRTC内部支持
    handle->last_error = APM_ERROR_NONE;
}

void webrtc_apm_enable_adaptive_processing(void *apm, int enable) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) return;
    
    // 自适应处理的实现
    handle->last_error = APM_ERROR_NONE;
}

void webrtc_apm_set_adaptation_speed(void *apm, float speed) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) return;
    
    handle->adaptation_speed = std::max(0.0f, std::min(1.0f, speed));
    handle->last_error = APM_ERROR_NONE;
}

// 更多占位符实现...
void webrtc_apm_update_config_runtime(void *apm, const char *config_json) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) return;
    // JSON配置解析实现
    handle->last_error = APM_ERROR_NONE;
}

char* webrtc_apm_export_config_json(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) return nullptr;
    // JSON配置导出实现
    handle->last_error = APM_ERROR_NONE;
    return nullptr;
}

void webrtc_apm_set_preprocessing_chain(void *apm, const APMPreprocessingChain *chain) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !chain) {
        if (handle) handle->last_error = APM_ERROR_INVALID_PARAMETER;
        return;
    }
    
    handle->preprocessing_chain = *chain;
    handle->last_error = APM_ERROR_NONE;
}

void webrtc_apm_get_frequency_response_ex(void *apm, float *magnitude, float *phase, int num_bins) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !magnitude || !phase) {
        if (handle) handle->last_error = APM_ERROR_INVALID_PARAMETER;
        return;
    }
    
    // 频率响应获取的实现需要更深入的WebRTC集成
    for (int i = 0; i < num_bins; ++i) {
        magnitude[i] = 1.0f; // 占位符
        phase[i] = 0.0f;     // 占位符
    }
    
    handle->last_error = APM_ERROR_NONE;
}