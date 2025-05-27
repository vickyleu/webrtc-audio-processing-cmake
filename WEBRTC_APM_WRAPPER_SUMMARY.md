# WebRTC APM Wrapper 完整功能总结

## 概述

这个WebRTC APM Wrapper为语音助手应用提供了完整的音频处理功能，支持Kotlin Native（所有bool类型已转换为int类型）。该wrapper基于Google WebRTC的音频处理模块，提供了回声消除、噪声抑制、自动增益控制等核心功能，并针对语音助手场景进行了优化。

## 🎯 核心功能模块

### 1. 回声消除 (Echo Cancellation)
- **AEC3**: 最新的回声消除算法，支持复杂声学环境
- **移动模式**: 针对移动设备优化的低延迟模式
- **线性AEC输出**: 专为唤醒词检测优化的线性输出
- **自适应滤波**: 动态适应不同的声学环境
- **延迟容忍**: 处理可变的系统延迟

### 2. 噪声抑制 (Noise Suppression)
- **多级别抑制**: 低、中、高、极高四个级别
- **频谱分析**: 基于频域的智能噪声检测
- **语音保护**: 在抑制噪声的同时保护语音质量
- **自适应算法**: 动态调整抑制强度

### 3. 自动增益控制 (AGC)
- **AGC1**: 经典的模拟/数字自适应增益控制
- **AGC2**: 现代化的数字增益控制，更低延迟
- **饱和保护**: 防止音频削波和失真
- **动态范围优化**: 保持良好的音频动态范围

### 4. 语音活动检测 (VAD)
- **传统VAD**: 基于能量和频谱特征的语音检测
- **RNN-VAD**: 基于深度学习的高精度语音检测
- **双讲检测**: 检测同时说话的情况
- **语音概率估算**: 提供连续的语音概率评分

### 5. 高级音频处理
- **高通滤波**: 去除低频噪声和直流分量
- **瞬态抑制**: 抑制键盘、鼠标点击等瞬时噪声
- **前置放大**: 输入信号预放大
- **残余回声检测**: 监测回声消除效果

## 🚀 语音助手专用功能

### 1. 唤醒词环境检测
```c
int webrtc_apm_detect_wake_word_environment(void *apm);
```
- 检测当前环境是否适合唤醒词检测
- 综合考虑噪声水平、回声强度、信号饱和等因素

### 2. 远场优化
```c
void webrtc_apm_optimize_for_far_field(void *apm, int enable);
```
- 专为远距离拾音优化
- 增强噪声抑制和增益控制
- 适合智能音箱等远场应用

### 3. 语音清晰度评分
```c
float webrtc_apm_get_speech_clarity_score(void *apm);
```
- 实时评估语音清晰度
- 帮助优化处理参数

### 4. 线性AEC输出
```c
int webrtc_apm_get_linear_aec_output(void *apm, float **output, int *length);
```
- 提供未经后处理的AEC输出
- 专为唤醒词检测算法优化

## 🎛️ 配置预设模式

### APM_PRESET_VOICE_ASSISTANT (新增)
专为语音助手优化的预设配置：
- 强噪声抑制 (High Level)
- 启用回声消除和线性输出导出
- 保守的增益控制设置
- 启用VAD和瞬态抑制
- 高通滤波去除低频噪声
- 适当的前置增益 (1.5x)

### 其他预设模式
- **DEFAULT**: 基础配置
- **CONFERENCE**: 会议通话优化
- **MUSIC**: 音乐处理优化
- **SPEECH**: 语音通话优化
- **LOW_LATENCY**: 低延迟优化

## 📊 音频质量评估

### 实时质量监控
```c
typedef struct APMAudioQuality {
    float signal_to_noise_ratio_db;     // 信噪比
    float speech_intelligibility_score; // 语音清晰度 [0,1]
    float distortion_level;             // 失真程度 [0,1]
    float background_noise_level_dbfs;  // 背景噪声电平
    int audio_dropouts_detected;        // 音频丢失检测
    float dynamic_range_db;             // 动态范围
} APMAudioQuality;
```

### 扩展统计信息
- 回声消除详细统计 (ERLE, ERL, 滤波器状态)
- 噪声抑制统计 (信噪比, 抑制增益)
- 处理延迟统计 (缓冲延迟, 总延迟, 实时性)

## 🔧 高级功能

### 1. 多通道处理
```c
int webrtc_apm_process_multi_channel_stream(void *apm, 
    const float *const *src_channels, int num_input_channels,
    float *const *dest_channels, int num_output_channels);
```
- 支持多通道音频处理
- 可选的通道混合和空间处理
- 波束形成支持

### 2. 实时参数调整
```c
void webrtc_apm_set_runtime_setting(void *apm, const APMRuntimeSetting *setting);
```
支持的实时设置：
- 前置增益调整
- 压缩增益调整
- 后置固定增益
- 播放音量变化通知
- 音频设备变化通知

### 3. 音频流分析
```c
typedef struct APMStreamAnalysis {
    int contains_speech;    // 包含语音
    int contains_music;     // 包含音乐
    int contains_noise;     // 包含噪声
    float dominant_frequency_hz;    // 主导频率
    float spectral_centroid_hz;     // 频谱重心
    float zero_crossing_rate;       // 过零率
    int clipping_detected;          // 削波检测
} APMStreamAnalysis;
```

### 4. 自适应处理
- 自适应阈值调整
- 动态参数优化
- 环境自适应

## 🔍 诊断和调试功能

### 1. 错误处理
```c
typedef enum APMErrorCode {
    APM_ERROR_NONE = 0,
    APM_ERROR_INVALID_PARAMETER = -1,
    APM_ERROR_PROCESSING_FAILED = -2,
    APM_ERROR_INITIALIZATION_FAILED = -3,
    APM_ERROR_MEMORY_ALLOCATION = -4,
    APM_ERROR_UNSUPPORTED_OPERATION = -5
} APMErrorCode;
```

### 2. 调试录音
```c
void webrtc_apm_enable_debug_recording(void *apm, const char* file_path);
```
- 记录处理前后的音频数据
- 用于分析和优化处理效果

### 3. 频率响应分析
```c
void webrtc_apm_get_frequency_response_ex(void *apm, 
    float *magnitude, float *phase, int num_bins);
```

## 🎮 便捷的控制接口

### 快捷开关
```c
void webrtc_apm_enable_aec(void *apm, int enable);    // 回声消除开关
void webrtc_apm_enable_ns(void *apm, int enable);     // 噪声抑制开关
void webrtc_apm_enable_agc(void *apm, int enable);    // 自动增益开关
void webrtc_apm_enable_vad(void *apm, int enable);    // 语音检测开关
```

### 动态参数调节
```c
void webrtc_apm_set_ns_level(void *apm, APMNsLevel level);          // 噪声抑制级别
void webrtc_apm_set_agc_target_level(void *apm, int target_dbfs);   // AGC目标电平
void webrtc_apm_set_pre_amplifier_gain(void *apm, float gain);      // 前置增益
```

## 🔄 回调和事件系统

### 音频事件回调
```c
typedef void (*APMAudioEventCallback)(void *user_data, int event_type, const void *event_data);
void webrtc_apm_set_audio_event_callback(void *apm, APMAudioEventCallback callback, void *user_data);
```

### 处理回调
```c
typedef void (*APMProcessingCallback)(void *user_data, const float *const *audio, int length);
void webrtc_apm_set_processing_callback(void *apm, APMProcessingCallback callback, void *user_data);
```

## ⚡ 性能优化

### 平台优化
```c
void webrtc_apm_optimize_for_platform(void *apm, const char *platform_info);
```
- 针对不同平台的性能优化
- 移动设备、服务器等不同场景

### 性能模式
```c
void webrtc_apm_set_performance_mode(void *apm, int low_latency, int low_power);
```
- 低延迟模式
- 低功耗模式

## 🔧 预处理链

### 自定义预处理
```c
typedef struct APMPreprocessingChain {
    int enable_dc_removal;                  // DC分量移除
    int enable_wind_noise_reduction;        // 风噪抑制
    int enable_click_removal;               // 点击音移除
    int enable_automatic_gain_normalization; // 自动增益归一化
    struct {
        int enabled;
        float cutoff_frequency_hz;
        int order;
    } custom_high_pass;                     // 自定义高通滤波
} APMPreprocessingChain;
```

## 📝 使用示例

### 基础使用
```c
// 创建APM实例
void *apm = webrtc_apm_create();

// 应用语音助手预设
webrtc_apm_apply_preset(apm, APM_PRESET_VOICE_ASSISTANT);

// 准备音频流
webrtc_apm_prepare(apm, 16000, 1);  // 16kHz单声道

// 处理音频
webrtc_apm_process_stream(apm, input_audio, output_audio);

// 获取处理结果
int voice_detected = webrtc_apm_voice_detected(apm);
float voice_prob = webrtc_apm_get_voice_probability(apm);

// 获取线性AEC输出用于唤醒词检测
float *linear_output;
int length;
int has_output = webrtc_apm_get_linear_aec_output(apm, &linear_output, &length);

// 销毁实例
webrtc_apm_destroy(apm);
```

### 高级配置
```c
// 获取默认配置
APMConfig config = webrtc_apm_get_default_config();

// 自定义配置
config.noise_suppression.enabled = 1;
config.noise_suppression.level = kNsHigh;
config.echo_canceller_advanced.basic.enabled = 1;
config.echo_canceller_advanced.basic.export_linear_aec_output = 1;

// 应用配置
webrtc_apm_apply_config(apm, &config);

// 实时调整
APMRuntimeSetting setting;
setting.type = APM_RUNTIME_CAPTURE_PRE_GAIN;
setting.value.float_value = 2.0f;
webrtc_apm_set_runtime_setting(apm, &setting);
```

## 🎯 语音助手应用建议

### 1. 推荐配置
- 使用 `APM_PRESET_VOICE_ASSISTANT` 预设
- 启用质量监控: `webrtc_apm_enable_quality_monitoring(apm, 1)`
- 启用远场优化: `webrtc_apm_optimize_for_far_field(apm, 1)`

### 2. 性能考虑
- 对于实时应用，监控 `real_time_processing` 状态
- 根据平台选择合适的性能模式
- 定期检查 `webrtc_apm_get_last_error()` 获取错误信息

### 3. 质量保证
- 定期调用 `webrtc_apm_detect_wake_word_environment()` 检查环境适宜性
- 监控 `speech_intelligibility_score` 评估语音质量
- 使用音频流分析检测异常情况

## 🔍 技术特点

1. **完整性**: 涵盖了语音助手所需的所有音频处理功能
2. **灵活性**: 支持从简单开关到详细参数的多层次配置
3. **实时性**: 提供实时参数调整和状态监控
4. **兼容性**: 支持Kotlin Native (所有bool转换为int)
5. **可扩展性**: 预留了回调和扩展接口
6. **性能优化**: 针对不同平台和应用场景的优化
7. **质量保证**: 全面的音频质量评估和诊断功能

这个wrapper为语音助手应用提供了工业级的音频处理能力，能够在各种复杂的声学环境中提供高质量的音频处理效果。 