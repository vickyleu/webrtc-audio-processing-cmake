//
// Created by user on 3/14/25.
//

#ifndef WEBRTC_APM_WRAPPER_H
#define WEBRTC_APM_WRAPPER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct APMConfigEchoCanceller {
    bool enabled = false;
    bool mobile_mode = false;
    bool export_linear_aec_output = false;
    // Enforce the highpass filter to be on (has no effect for the mobile
    // mode).
    bool enforce_high_pass_filtering = true;
}APMConfigEchoCanceller;

// 新增：噪声抑制（NS）配置
typedef enum APMNsLevel {
    kNsLow = 0,
    kNsModerate,
    kNsHigh,
    kNsVeryHigh
} APMNsLevel;

typedef struct APMConfigNoiseSuppression {
    bool enabled = false;
    APMNsLevel level = kNsModerate;
} APMConfigNoiseSuppression;

// 新增：高通滤波（HPF）配置
typedef struct APMConfigHighPassFilter {
    bool enabled = false;
} APMConfigHighPassFilter;

// 新增：自动增益控制（AGC1）配置
typedef enum APMAgcMode {
    kAgcAdaptiveAnalog = 0,
    kAgcAdaptiveDigital,
    kAgcFixedDigital
} APMAgcMode;

typedef struct APMConfigGainController {
    bool enabled = false;
    APMAgcMode mode = kAgcAdaptiveAnalog;
    int target_level_dbfs = 3;        // [0,31]
    int compression_gain_db = 9;      // [0,90]
    bool enable_limiter = true;
} APMConfigGainController;

// 新增：AGC2配置（更先进的增益控制）
typedef struct APMConfigGainController2 {
    bool enabled = false;
    // 自适应数字增益控制
    struct {
        bool enabled = true;
        float max_gain_db = 50.0f;
        float initial_saturation_margin_db = 20.0f;
        int extra_saturation_margin_db = 2;
        float gain_applier_adjacent_speech_frames_threshold = 1.0f;
        float max_gain_change_db_per_second = 3.0f;
        float max_output_noise_level_dbfs = -50.0f;
    } adaptive_digital;

    // 固定数字增益控制
    struct {
        bool enabled = false;
        float gain_db = 0.0f;
    } fixed_digital;
} APMConfigGainController2;

// 新增：语音检测（VAD）配置
typedef struct APMConfigVoiceDetection {
    bool enabled = false;
} APMConfigVoiceDetection;

// 新增：短暂噪声抑制（Transient Suppression）配置
typedef struct APMConfigTransientSuppression {
    bool enabled = false;
} APMConfigTransientSuppression;

// 新增：残余回声检测（RED）配置
typedef struct APMConfigResidualEchoDetector {
    bool enabled = true;
} APMConfigResidualEchoDetector;

// 新增：电平估计（Level Estimation）配置
typedef struct APMConfigLevelEstimation {
    bool enabled = false;
} APMConfigLevelEstimation;

// 新增：前置放大器（PreAmplifier）配置
typedef struct APMConfigPreAmplifier {
    bool enabled = false;
    float fixed_gain_factor = 1.0f;  // 倍率，1.0 为 0 dB
} APMConfigPreAmplifier;

// 新增：捕获后增益控制
typedef struct APMConfigCapturePostProcessor {
    bool enabled = false;
} APMConfigCapturePostProcessor;

// 新增：播放前增益控制
typedef struct APMConfigRenderPreProcessor {
    bool enabled = false;
} APMConfigRenderPreProcessor;

typedef struct APMConfig {
    APMConfigEchoCanceller echo_canceller;
    APMConfigNoiseSuppression noise_suppression;
    APMConfigHighPassFilter high_pass_filter;
    APMConfigGainController gain_controller;
    APMConfigGainController2 gain_controller2;  // 新增
    APMConfigVoiceDetection voice_detection;
    APMConfigTransientSuppression transient_suppression;
    APMConfigResidualEchoDetector residual_echo_detector;
    APMConfigLevelEstimation level_estimation;
    APMConfigPreAmplifier pre_amplifier;
    APMConfigCapturePostProcessor capture_post_processor;  // 新增
    APMConfigRenderPreProcessor render_pre_processor;    // 新增
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
float webrtc_apm_get_voice_probability(void *apm);  // 新增：获取语音概率

// 延迟补偿：调用前先估计近端采集帧与远端播放帧之间的延迟（毫秒）
void webrtc_apm_set_stream_delay_ms(void *apm, int delay_ms);
int  webrtc_apm_get_stream_delay_ms(void *apm);

// 快捷开关接口
void webrtc_apm_enable_aec(void *apm, bool enable);
void webrtc_apm_enable_ns(void *apm, bool enable);   // 新增：噪声抑制开关
void webrtc_apm_enable_agc(void *apm, bool enable);  // 新增：增益控制开关
void webrtc_apm_enable_vad(void *apm, bool enable);  // 新增：VAD开关

// 动态参数调节接口
void webrtc_apm_set_ns_level(void *apm, APMNsLevel level);        // 新增：动态调节降噪强度
void webrtc_apm_set_agc_target_level(void *apm, int target_dbfs); // 新增：动态调节AGC目标电平
void webrtc_apm_set_pre_amplifier_gain(void *apm, float gain_factor); // 新增：动态调节前置放大

// 统计信息接口
APMStatistics webrtc_apm_get_statistics(void *apm);  // 新增：获取详细统计信息
void webrtc_apm_reset_statistics(void *apm);         // 新增：重置统计信息

// 高级接口
int webrtc_apm_process_stream_with_result(void *apm, const float *const *src, float *const *dest); // 新增：带返回值的处理
bool webrtc_apm_is_saturated(void *apm);            // 新增：检查是否饱和
float webrtc_apm_get_speech_level_dbfs(void *apm);  // 新增：获取语音电平
float webrtc_apm_get_noise_level_dbfs(void *apm);   // 新增：获取噪声电平

// 调试和监控接口
void webrtc_apm_enable_debug_recording(void *apm, const char* file_path); // 新增：启用调试录音
void webrtc_apm_disable_debug_recording(void *apm);  // 新增：禁用调试录音

#ifdef __cplusplus
}
#endif

#endif // WEBRTC_APM_WRAPPER_H