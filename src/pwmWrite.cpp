/*******************************************************************
   pwmWrite Library for ESP32 Arduino core, Version 4.1.0
   by dlloydev https://github.com/Dlloydev/ESP32-ESP32S2-AnalogWrite
   This Library is licensed under the MIT License
 *******************************************************************/

#include <Arduino.h>
#include "pwmWrite.h"
#include <stdlib.h>

Pwm::Pwm() {}

float Pwm::write(uint8_t pin, uint32_t duty) {
  uint8_t ch = getPinStatus(pin);
  if (ch == pinIs::free) ch = attachPin(pin);
  if (ch < chMax) { // write PWM
    uint32_t dutyFix = maxDutyFix(duty, config[ch].resolution);
    if (config[ch].duty != duty) {
      configChannel(ch);
      ledcSetup(ch, config[ch].frequency, config[ch].resolution);
      if (_sync) timerPause(ch);
      ledcWrite(ch, dutyFix);
      config[ch].duty = dutyFix;
    }
  }
  return config[ch].frequency;
}

float Pwm::write(uint8_t pin, uint32_t duty, uint32_t frequency) {
  uint8_t ch = getPinStatus(pin);
  if (ch == pinIs::free) ch = attachPin(pin);
  if (ch < chMax) { // write PWM
    uint32_t dutyFix = maxDutyFix(duty, config[ch].resolution);
    if (config[ch].frequency != frequency) {
      configChannel(ch);
      ledcSetup(ch, frequency, config[ch].resolution);
      if (_sync) timerPause(ch);
      ledcWrite(ch, dutyFix);
      writerFreqResPair(ch, frequency, config[ch].resolution);
    }
    if (config[ch].duty != dutyFix) {
      ledcWrite(ch, dutyFix);
      config[ch].duty = dutyFix;
    }
  }
  return config[ch].frequency;
}

float Pwm::write(uint8_t pin, uint32_t duty, uint32_t frequency, uint8_t resolution) {
  uint8_t ch = getPinStatus(pin);
  if (ch == pinIs::free) ch = attachPin(pin);
  if (ch < chMax) { // write PWM
    uint32_t dutyFix = maxDutyFix(duty, resolution);
    if ((config[ch].frequency != frequency) || (config[ch].resolution != resolution)) {
      configChannel(ch);
      ledcSetup(ch, frequency, resolution);
      if (_sync) timerPause(ch);
      ledcWrite(ch, dutyFix);
      writerFreqResPair(ch, frequency, resolution);
    }
    if (config[ch].duty != duty) {
      ledcWrite(ch, duty);
      config[ch].duty = duty;
    }
  }
  return config[ch].frequency;
}

float Pwm::write(uint8_t pin, uint32_t duty, uint32_t frequency, uint8_t resolution, uint32_t phase) {
  uint8_t ch = getPinStatus(pin);
  if (ch == pinIs::free) ch = attachPin(pin);
  if (ch < chMax) { // write PWM
    uint32_t dutyFix = maxDutyFix(duty, resolution);
    if ((config[ch].frequency != frequency) || (config[ch].resolution != resolution)) {
      ledcSetup(ch, frequency, resolution);
      if (_sync) timerPause(ch);
      ledcWrite(ch, dutyFix);
      writerFreqResPair(ch, frequency, resolution);
    }
    configChannel(ch);
    uint32_t ch_config = ch;
    if (ch > 7) ch_config = ch - 8;
    ledc_set_duty_with_hpoint((ledc_mode_t)config[ch].mode, (ledc_channel_t)ch_config, duty, phase);
    config[ch].phase = phase;
    if (config[ch].duty != duty) {
      ledcWrite(ch, duty);
      config[ch].duty = duty;
    }
  }
  return config[ch].frequency;
}

uint32_t Pwm::writeServo(uint8_t pin, float value) {
  uint8_t ch = getPinStatus(pin);
  float countPerUs;
  uint32_t duty = config[ch].servoDefUs;
  if (ch == pinIs::free) ch = attachPin(pin);
  if (ch < chMax) { // write PWM
    if (config[ch].frequency < 40 || config[ch].frequency > 900) config[ch].frequency = 50;
    if (config[ch].resolution > widthMax) config[ch].resolution = widthMax;
    else if (config[ch].resolution < 14) config[ch].resolution = 14;
    countPerUs = ((1 << config[ch].resolution) - 1) / (1000000.0 / config[ch].frequency);
    if (value < config[ch].servoMinUs) {  // degrees
      if (value < 0) value = 0;
      else if (value > 180 && value < 500) value = 180;
      duty = (((value / 180.0) * (config[ch].servoMaxUs - config[ch].servoMinUs)) + config[ch].servoMinUs) * countPerUs;
    } else {  // microseconds
      if (value < config[ch].servoMinUs) value = config[ch].servoMinUs;
      else if (value > config[ch].servoMaxUs) value = config[ch].servoMaxUs;
      duty = value * countPerUs;
    }
    if (config[ch].duty != duty) {
      configChannel(ch);
      ledcSetup(ch, config[ch].frequency, config[ch].resolution);
      ledcWrite(ch, duty);
      writerFreqResPair(ch, config[ch].frequency, config[ch].resolution);
      config[ch].duty = duty;
    }
  }
  return duty;
}
void Pwm::setServo(uint8_t ch, uint16_t minUs, uint16_t defUs, uint16_t maxUs) {
  if (ch < chMax) {
    if (minUs < 500) config[ch].servoMinUs = 500;
    else if (minUs > 2500) config[ch].servoMinUs = 2500;
    else config[ch].servoMinUs = minUs;
    if (defUs < 500) config[ch].servoDefUs = 500;
    else if (defUs > 2500) config[ch].servoDefUs = 2500;
    else config[ch].servoDefUs = defUs;
    if (maxUs < 500) config[ch].servoMaxUs = 500;
    else if (maxUs > 2500) config[ch].servoMaxUs = 2500;
    else config[ch].servoMaxUs = maxUs;
  }
}

uint8_t Pwm::attachPin(uint8_t pin) {
  if (getPinStatus(pin) == pinIs::free) {
    for (uint8_t ch = 0; ch < chMax; ch++) {
      if (config[ch].pin == pinIs::free) { // ch is free
        config[ch].pin = pin;
        ledcSetup(ch, config[ch].frequency, config[ch].resolution);
        if (_sync) timerPause(ch);
        ledcAttachPin(pin, ch);
        return ch;
      }
    }
  }
  return pinIs::denied;
}

uint8_t Pwm::attachPin(uint8_t pin, uint8_t ch) {
  if (getPinStatus(pin) == pinIs::free && ch < chMax) {
    config[ch].pin = pin;
    ledcSetup(ch, config[ch].frequency, config[ch].resolution);
    if (_sync) timerPause(ch);
    ledcAttachPin(pin, ch);
    return ch;
  }
  else return pinIs::denied;
}

void Pwm::detachPin(uint8_t pin) {
  uint8_t ch = getPinStatus(pin);
  if (ch < chMax) {
    config[ch].pin = 255;
    config[ch].duty = 0;
    config[ch].frequency = 1000;
    config[ch].resolution = 8;
    config[ch].phase = 0;
    ledcWrite(ch, 0);
    ledcSetup(ch, 0, 0);
    ledcDetachPin(config[ch].pin);
    REG_SET_FIELD(GPIO_PIN_MUX_REG[pin], MCU_SEL, GPIO_MODE_DEF_DISABLE);
  }
}

uint8_t Pwm::getPinStatus(uint8_t pin) {
  if (!((pinMask >> pin) & 1)) {
    return pinIs::notPwm;
  } else {  // check if pin is attached
    for (uint8_t ch = 0; ch < chMax; ch++) {
      if (config[ch].pin == pin) return ch;
    }
    if ((REG_GET_FIELD(GPIO_PIN_MUX_REG[pin], MCU_SEL)) == 0) return pinIs::free;
    else return pinIs::denied;
  }
}

uint8_t Pwm::getPinOnChannel(uint8_t ch) {
  return config[ch].pin;
}

void Pwm::pause() {
  _sync = true;
}

void Pwm::resume() {
  for (uint8_t ch = 0; ch < chMax; ch++) {
    if (config[ch].pin < 48) timerResume(ch);
  }
  _sync = false;
}

float Pwm::setFrequency(uint8_t pin, uint32_t frequency) {
  uint8_t ch = getPinStatus(pin);
  if (ch == pinIs::free) ch = attachPin(pin);
  if (ch < chMax) {
    if (config[ch].frequency != frequency) {
      configChannel(ch);
      ledcSetup(ch, frequency, config[ch].resolution);
      if (_sync) timerPause(ch);
      ledcWrite(ch, config[ch].duty);
      writerFreqResPair(ch, frequency, config[ch].resolution);
    }
  }
  return ledcReadFreq(ch);
}

uint8_t Pwm::setResolution(uint8_t pin, uint8_t resolution) {
  uint8_t ch = getPinStatus(pin);
  if (ch == pinIs::free) ch = attachPin(pin);
  if (ch < chMax) {
    if ((config[ch].pin) > 47) return 255;
    if (config[ch].resolution != resolution) {
      ledcDetachPin(pin);
      configChannel(ch);
      ledcSetup(ch, config[ch].frequency, resolution);
      if (_sync) timerPause(ch);
      ledcAttachPin(pin, ch);
      ledcWrite(ch, config[ch].duty);
      config[ch].resolution = resolution;
      writerFreqResPair(ch, config[ch].frequency, resolution);
    }
  }
  return config[ch].resolution;
}

void Pwm::setConfigDefaults(uint32_t duty, uint32_t frequency, uint8_t resolution, uint32_t phase) {
  for (uint8_t ch = 0; ch < chMax; ch++) {
    config[ch].duty = duty;
    config[ch].frequency = frequency;
    config[ch].resolution = resolution;
    config[ch].phase = phase;
  }
}

void Pwm::printConfig() {
  Serial.print(F("PWM pins: "));
  for (uint8_t i = 0; i < 48; i++) {
    if ((pinMask >> i) & 1) {
      Serial.print(i); Serial.print(F(", "));
    }
  }
  Serial.println();
  Serial.println();
  for (uint8_t ch = 0; ch < chMax; ch++) {
    Serial.print(F("ch: "));
    if (ch < chMax) {
      if (config[ch].channel < 10) Serial.print(F(" "));
      Serial.print(ch);
      Serial.print(F("  "));
      Serial.print(F("Pin: "));
      if (config[ch].pin < 100) Serial.print(F(" "));
      if (config[ch].pin < 10) Serial.print(F(" "));
      Serial.print(config[ch].pin); Serial.print(F("  "));
      Serial.print(F("Hz: "));
      if (config[ch].frequency < 10000) Serial.print(F(" "));
      if (config[ch].frequency < 1000) Serial.print(F(" "));
      if (config[ch].frequency < 100) Serial.print(F(" "));
      if (config[ch].frequency < 10) Serial.print(F(" "));
      Serial.print(config[ch].frequency); Serial.print(F("  "));
      Serial.print(F("Bits: "));
      if (config[ch].resolution < 10) Serial.print(F(" "));
      Serial.print(config[ch].resolution); Serial.print(F("  "));
      Serial.print(F("Duty: "));
      if (config[ch].duty < 10000) Serial.print(F(" "));
      if (config[ch].duty < 1000) Serial.print(F(" "));
      if (config[ch].duty < 100) Serial.print(F(" "));
      if (config[ch].duty < 10) Serial.print(F(" "));
      Serial.print(config[ch].duty); Serial.print(F("  "));
      Serial.print(F("Ø: "));
      if (config[ch].phase < 1000) Serial.print(F(" "));
      if (config[ch].phase < 100) Serial.print(F(" "));
      if (config[ch].phase < 10) Serial.print(F(" "));
      Serial.print(config[ch].phase);
      Serial.println();
    }
  }
}

/***************************** private helper functions ********************************/

void Pwm::timerPause(uint8_t ch) {
  ledc_timer_pause((ledc_mode_t)config[ch].mode, (ledc_timer_t)config[ch].timer);
}

void Pwm::timerResume(uint8_t ch) {
  ledc_timer_resume((ledc_mode_t)config[ch].mode, (ledc_timer_t)config[ch].timer);
}

void  Pwm::configChannel(uint8_t ch) {
  uint32_t ch_config = ch;
  if (ch > 7) ch_config = ch - 8;
  ledc_channel_config_t ledc_channel = {
    .gpio_num       = (config[ch].pin),
    .speed_mode     = (ledc_mode_t) config[ch].mode,
    .channel        = (ledc_channel_t) ch_config,
    .intr_type      = (ledc_intr_type_t) LEDC_INTR_DISABLE,
    .timer_sel      = (ledc_timer_t) config[ch].timer,
    .duty           = 0,
    .hpoint         = 0,
    .flags = {
      .output_invert = 0
    }
  };
  ledc_channel_config(&ledc_channel);
}

void Pwm::writerFreqResPair(uint8_t ch, uint32_t frequency, uint8_t bits) {
  config[ch].frequency = frequency;
  config[ch].resolution = bits;
  if (ch % 2 == 0) { // even ch
    config[ch + 1].frequency = frequency;
    config[ch + 1].resolution = bits;
  } else { // odd ch
    config[ch - 1].frequency = frequency;
    config[ch - 1].resolution = bits;
  }
}

uint32_t Pwm::maxDutyFix(uint32_t duty, uint8_t resolution) {
  if (duty > ((1 << resolution) - 1)) duty = (1 << resolution); //constrain
  if ((resolution > 7) && (duty == ((1 << resolution) - 1))) duty = (1 << resolution); //keep PWM high
  return duty;
}
