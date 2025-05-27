//
// Created by user on 3/14/25.
//
#include "webrtc_apm_wrapper.h"

#include <modules/audio_processing/include/audio_processing.h>
#include <modules/audio_processing/aec_dump/aec_dump_factory.h>
#include <rtc_base/logging.h>
#include <memory>

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
};

void* webrtc_apm_create()
{
    auto *handle = new WebRTCApm();
    handle->apm = webrtc::AudioProcessingBuilder().Create();

    // 初始化扩展配置为默认值
    handle->voice_prob_config.high_confidence_threshold = 0.8f;
    handle->voice_prob_config.low_confidence_threshold = 0.2f;
    handle->voice_prob_config.use_advanced_estimation = false;

    handle->saturation_config.low_level_threshold = 0;
    handle->saturation_config.rms_threshold_dbfs = -3.0f;
    handle->saturation_config.enable_multi_criteria_detection = true;

    handle->noise_config.default_noise_level_dbfs = -60.0f;
    handle->noise_config.estimation_window_ms = 100;
    handle->noise_config.enable_adaptive_estimation = true;

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

    // Gain controller 2 (modern AGC) - 使用配置参数而非写死
    cfg.gain_controller2.enabled = config->gain_controller2.enabled;
    cfg.gain_controller2.adaptive_digital.enabled = config->gain_controller2.adaptive_digital.enabled;
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

    // VAD相关 - 使用配置参数
    if (webrtc_stats.voice_detected) {
        stats.voice_detected = *webrtc_stats.voice_detected;
        stats.voice_probability = stats.voice_detected ?
                                  handle->voice_prob_config.high_confidence_threshold :
                                  handle->voice_prob_config.low_confidence_threshold;
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

bool webrtc_apm_is_saturated(void *apm) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle) {
        return false;
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
    config.echo_canceller.enabled = false;
    config.echo_canceller.mobile_mode = false;
    config.echo_canceller.export_linear_aec_output = false;
    config.echo_canceller.enforce_high_pass_filtering = true;

    // Noise Suppression defaults
    config.noise_suppression.enabled = false;
    config.noise_suppression.level = kNsModerate;

    // High Pass Filter defaults
    config.high_pass_filter.enabled = false;

    // Gain Controller 1 defaults
    config.gain_controller.enabled = false;
    config.gain_controller.mode = kAgcAdaptiveAnalog;
    config.gain_controller.target_level_dbfs = 3;
    config.gain_controller.compression_gain_db = 9;
    config.gain_controller.enable_limiter = true;

    // Gain Controller 2 defaults
    config.gain_controller2.enabled = false;
    config.gain_controller2.adaptive_digital.enabled = true;
    config.gain_controller2.adaptive_digital.initial_saturation_margin_db = 20.0f;
    config.gain_controller2.adaptive_digital.extra_saturation_margin_db = 2;
    config.gain_controller2.adaptive_digital.gain_applier_adjacent_speech_frames_threshold = 1.0f;
    config.gain_controller2.adaptive_digital.max_gain_change_db_per_second = 3.0f;
    config.gain_controller2.adaptive_digital.max_output_noise_level_dbfs = -50.0f;
    config.gain_controller2.fixed_digital.gain_db = 0.0f;

    // Voice Detection defaults
    config.voice_detection.enabled = false;

    // Transient Suppression defaults
    config.transient_suppression.enabled = false;

    // Residual Echo Detector defaults
    config.residual_echo_detector.enabled = true;

    // Level Estimation defaults
    config.level_estimation.enabled = false;

    // Pre Amplifier defaults
    config.pre_amplifier.enabled = false;
    config.pre_amplifier.fixed_gain_factor = 1.0f;

    // Voice Probability defaults
    config.voice_probability.high_confidence_threshold = 0.8f;
    config.voice_probability.low_confidence_threshold = 0.2f;
    config.voice_probability.use_advanced_estimation = false;

    // Saturation Detection defaults
    config.saturation_detection.low_level_threshold = 0;
    config.saturation_detection.rms_threshold_dbfs = -3.0f;
    config.saturation_detection.enable_multi_criteria_detection = true;

    // Noise Estimation defaults
    config.noise_estimation.default_noise_level_dbfs = -60.0f;
    config.noise_estimation.estimation_window_ms = 100;
    config.noise_estimation.enable_adaptive_estimation = true;

    return config;
}

bool webrtc_apm_validate_config(const APMConfig* config) {
    if (!config) {
        return false;
    }

    // 验证 AGC1 参数范围
    if (config->gain_controller.target_level_dbfs < 0 || config->gain_controller.target_level_dbfs > 31) {
        return false;
    }
    if (config->gain_controller.compression_gain_db < 0 || config->gain_controller.compression_gain_db > 90) {
        return false;
    }

    // 验证 AGC2 参数
    if (config->gain_controller2.adaptive_digital.initial_saturation_margin_db < 0 ||
        config->gain_controller2.adaptive_digital.initial_saturation_margin_db > 50) {
        return false;
    }

    // 验证前置放大器增益
    if (config->pre_amplifier.fixed_gain_factor < 0.1f || config->pre_amplifier.fixed_gain_factor > 10.0f) {
        return false;
    }

    // 验证语音概率阈值
    if (config->voice_probability.high_confidence_threshold <= config->voice_probability.low_confidence_threshold) {
        return false;
    }
    if (config->voice_probability.high_confidence_threshold < 0.0f || config->voice_probability.high_confidence_threshold > 1.0f) {
        return false;
    }
    if (config->voice_probability.low_confidence_threshold < 0.0f || config->voice_probability.low_confidence_threshold > 1.0f) {
        return false;
    }

    return true;
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
            config.echo_canceller.enabled = true;
            config.noise_suppression.enabled = true;
            config.noise_suppression.level = kNsHigh;
            config.gain_controller.enabled = true;
            config.gain_controller.target_level_dbfs = 6;
            config.high_pass_filter.enabled = true;
            config.voice_detection.enabled = true;
            config.transient_suppression.enabled = true;
            break;

        case APM_PRESET_MUSIC:
            // 音乐模式：保持音频质量
            config.echo_canceller.enabled = false;
            config.noise_suppression.enabled = true;
            config.noise_suppression.level = kNsLow;
            config.gain_controller.enabled = false;
            config.gain_controller2.enabled = true;
            config.high_pass_filter.enabled = false;
            config.transient_suppression.enabled = false;
            break;

        case APM_PRESET_SPEECH:
            // 语音通话模式：平衡处理
            config.echo_canceller.enabled = true;
            config.noise_suppression.enabled = true;
            config.noise_suppression.level = kNsModerate;
            config.gain_controller.enabled = true;
            config.gain_controller.target_level_dbfs = 3;
            config.high_pass_filter.enabled = true;
            config.voice_detection.enabled = true;
            break;

        case APM_PRESET_LOW_LATENCY:
            // 低延迟模式：最小化处理
            config.echo_canceller.enabled = true;
            config.echo_canceller.mobile_mode = true; // 移动模式延迟更低
            config.noise_suppression.enabled = true;
            config.noise_suppression.level = kNsLow;
            config.gain_controller.enabled = false;
            config.gain_controller2.enabled = true; // AGC2 延迟更低
            config.high_pass_filter.enabled = false;
            config.transient_suppression.enabled = false;
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
        bool voice_detected = false;
        float base_probability = config->low_confidence_threshold;

        if (stats.voice_detected) {
            voice_detected = *stats.voice_detected;
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

bool webrtc_apm_is_saturated_ex(void *apm, const APMConfigSaturationDetection* config) {
    auto* handle = static_cast<WebRTCApm*>(apm);
    if (!handle || !config) {
        return false;
    }

    bool saturated = false;

    // 检查推荐的模拟电平
    int recommended_level = handle->apm->recommended_stream_analog_level();
    if (recommended_level <= config->low_level_threshold) {
        saturated = true;
    }

    if (config->enable_multi_criteria_detection) {
        // 多重标准检测
        auto stats = handle->apm->GetStatistics();

        // 检查RMS电平
        if (stats.output_rms_dbfs) {
            float rms = *stats.output_rms_dbfs;
            if (rms > config->rms_threshold_dbfs) {
                saturated = true;
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
        bool voice_detected = false;
        if (stats.voice_detected) {
            voice_detected = *stats.voice_detected;
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