//
// Created by user on 3/14/25.
//

#ifndef WEBRTC_APM_WRAPPER_H
#define WEBRTC_APM_WRAPPER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 基础配置结构体
typedef struct APMConfigEchoCanceller {
    bool enabled;
    bool mobile_mode;
    bool export_linear_aec_output;
    bool enforce_high_pass_filtering;
} APMConfigEchoCanceller;

// 噪声抑制（NS）配置
typedef enum APMNsLevel {
    kNsLow = 0,
    kNsModerate,
    kNsHigh,
    kNsVeryHigh
} APMNsLevel;

typedef struct APMConfigNoiseSuppression {
    bool enabled;
    APMNsLevel level;
} APMConfigNoiseSuppression;

// 高通滤波（HPF）配置
typedef struct APMConfigHighPassFilter {
    bool enabled;
} APMConfigHighPassFilter;

// 自动增益控制（AGC1）配置
typedef enum APMAgcMode {
    kAgcAdaptiveAnalog = 0,
    kAgcAdaptiveDigital,
    kAgcFixedDigital
} APMAgcMode;

typedef struct APMConfigGainController {
    bool enabled;
    APMAgcMode mode;
    int target_level_dbfs;        // [0,31]
    int compression_gain_db;      // [0,90]
    bool enable_limiter;
} APMConfigGainController;

// AGC2 自适应数字增益控制详细配置
typedef struct APMConfigAdaptiveDigital {
    bool enabled;
    float initial_saturation_margin_db;    // 可配置，默认20.0
    int extra_saturation_margin_db;        // 可配置，默认2
    float gain_applier_adjacent_speech_frames_threshold; // 可配置
    float max_gain_change_db_per_second;   // 可配置，默认3.0
    float max_output_noise_level_dbfs;     // 可配置，默认-50.0
} APMConfigAdaptiveDigital;

// AGC2配置（完整版本）
typedef struct APMConfigGainController2 {
    bool enabled;
    APMConfigAdaptiveDigital adaptive_digital;

    // 固定数字增益控制
    struct {
        float gain_db;
    } fixed_digital;
} APMConfigGainController2;

// 语音检测（VAD）配置
typedef struct APMConfigVoiceDetection {
    bool enabled;
} APMConfigVoiceDetection;

// 短暂噪声抑制（Transient Suppression）配置
typedef struct APMConfigTransientSuppression {
    bool enabled;
} APMConfigTransientSuppression;

// 残余回声检测（RED）配置
typedef struct APMConfigResidualEchoDetector {
    bool enabled;
} APMConfigResidualEchoDetector;

// 电平估计（Level Estimation）配置
typedef struct APMConfigLevelEstimation {
    bool enabled;
} APMConfigLevelEstimation;

// 前置放大器（PreAmplifier）配置
typedef struct APMConfigPreAmplifier {
    bool enabled;
    float fixed_gain_factor;  // 倍率，1.0 为 0 dB
} APMConfigPreAmplifier;

// 语音概率估算配置
typedef struct APMConfigVoiceProbability {
    float high_confidence_threshold;    // 高置信度阈值
    float low_confidence_threshold;     // 低置信度阈值
    bool use_advanced_estimation;       // 是否使用高级估算算法
} APMConfigVoiceProbability;

// 饱和检测配置
typedef struct APMConfigSaturationDetection {
    int low_level_threshold;               // 低电平阈值，小于此值认为可能饱和
    float rms_threshold_dbfs;              // RMS阈值，超过此值认为可能饱和
    bool enable_multi_criteria_detection; // 启用多重标准检测
} APMConfigSaturationDetection;

// 噪声电平估算配置
typedef struct APMConfigNoiseEstimation {
    float default_noise_level_dbfs;   // 默认噪声电平
    int estimation_window_ms;         // 估算窗口大小(毫秒)
    bool enable_adaptive_estimation;  // 启用自适应估算
} APMConfigNoiseEstimation;

// 主配置结构体
typedef struct APMConfig {
    APMConfigEchoCanceller echo_canceller;
    APMConfigNoiseSuppression noise_suppression;
    APMConfigHighPassFilter high_pass_filter;
    APMConfigGainController gain_controller;
    APMConfigGainController2 gain_controller2;
    APMConfigVoiceDetection voice_detection;
    APMConfigTransientSuppression transient_suppression;
    APMConfigResidualEchoDetector residual_echo_detector;
    APMConfigLevelEstimation level_estimation;
    APMConfigPreAmplifier pre_amplifier;

    // 高级配置
    APMConfigVoiceProbability voice_probability;
    APMConfigSaturationDetection saturation_detection;
    APMConfigNoiseEstimation noise_estimation;
} APMConfig;

// 音频统计信息结构体
typedef struct APMStatistics {
    // VAD相关
    bool voice_detected;
    float voice_probability;

    // 回声相关
    bool residual_echo_detected;
    float residual_echo_likelihood;
    float echo_return_loss;
    float echo_return_loss_enhancement;

    // 延迟相关
    int delay_median_ms;
    int delay_standard_deviation_ms;

    // 噪声相关
    float noise_suppression_level;

    // 增益相关
    bool saturation_detected;
    int recommended_analog_level;
} APMStatistics;

// 配置预设模式
typedef enum APMPresetMode {
    APM_PRESET_DEFAULT = 0,      // 默认模式
    APM_PRESET_CONFERENCE,       // 会议模式
    APM_PRESET_MUSIC,           // 音乐模式
    APM_PRESET_SPEECH,          // 语音通话模式
    APM_PRESET_LOW_LATENCY      // 低延迟模式
} APMPresetMode;

// 基础接口
void *webrtc_apm_create();
void webrtc_apm_destroy(void *apm);

void webrtc_apm_apply_config(void *apm, const APMConfig *config);
void webrtc_apm_prepare(void *apm, int sample_rate, int channels);
void webrtc_apm_process_reverse_stream(void *apm, const float *const *src,
                                       float *const *dest);
void webrtc_apm_process_stream(void *apm, const float *const *src,
                               float *const *dest);

// 运行期接口
void webrtc_apm_set_stream_analog_level(void *apm, int level);
int  webrtc_apm_get_stream_analog_level(void *apm);
void webrtc_apm_set_key_pressed(void *apm, bool key_pressed);

// VAD 结果输出
bool webrtc_apm_voice_detected(void *apm);
float webrtc_apm_get_voice_probability(void *apm);

// 延迟补偿
void webrtc_apm_set_stream_delay_ms(void *apm, int delay_ms);
int  webrtc_apm_get_stream_delay_ms(void *apm);

// 快捷开关接口
void webrtc_apm_enable_aec(void *apm, bool enable);
void webrtc_apm_enable_ns(void *apm, bool enable);
void webrtc_apm_enable_agc(void *apm, bool enable);
void webrtc_apm_enable_vad(void *apm, bool enable);

// 动态参数调节接口
void webrtc_apm_set_ns_level(void *apm, APMNsLevel level);
void webrtc_apm_set_agc_target_level(void *apm, int target_dbfs);
void webrtc_apm_set_pre_amplifier_gain(void *apm, float gain_factor);

// 统计信息接口
APMStatistics webrtc_apm_get_statistics(void *apm);
void webrtc_apm_reset_statistics(void *apm);

// 高级接口
int webrtc_apm_process_stream_with_result(void *apm, const float *const *src, float *const *dest);
bool webrtc_apm_is_saturated(void *apm);
float webrtc_apm_get_speech_level_dbfs(void *apm);
float webrtc_apm_get_noise_level_dbfs(void *apm);

// 调试和监控接口
void webrtc_apm_enable_debug_recording(void *apm, const char* file_path);
void webrtc_apm_disable_debug_recording(void *apm);

// 配置管理接口
APMConfig webrtc_apm_get_default_config();
bool webrtc_apm_validate_config(const APMConfig* config);
void webrtc_apm_apply_preset(void *apm, APMPresetMode mode);

// 扩展接口
float webrtc_apm_get_voice_probability_ex(void *apm, const APMConfigVoiceProbability* config);
bool webrtc_apm_is_saturated_ex(void *apm, const APMConfigSaturationDetection* config);
float webrtc_apm_get_noise_level_dbfs_ex(void *apm, const APMConfigNoiseEstimation* config);

#ifdef __cplusplus
}
#endif

#endif // WEBRTC_APM_WRAPPER_H