# Drumee — прототип VST3 (Reflexed)

Базовый прототип драм-плагина: 5 слотов сэмплов, 16-шаговый секвенсор,
9 макро-энкодеров (Timing/Groove, Pitch/Sound, Ratchet/Chaos), пресеты,
GUI в тёмной стеклянной эстетике по заданной палитре, окно 16:9.

## Структура

```
CMakeLists.txt              сборка через JUCE (FetchContent, тег 7.0.9)
Source/
  Parameters.h               ID и лэйаут параметров APVTS
  DSP.h / DSP.cpp             сэмпл-плеер, слоты, движок секвенсора
  PresetManager.h / .cpp      сохранение/загрузка пресетов (.xml)
  PluginProcessor.h / .cpp    AudioProcessor, обработка блока, состояние
  GUI.h / .cpp                LookAndFeel, энкодеры, визуализатор шагов, слоты
  PluginEditor.h / .cpp       компоновка интерфейса
.github/workflows/build.yml  сборка VST3 на Windows/macOS + zip-архив
```

## Локальная сборка

```bash
cmake -B build
cmake --build build --config Release
```

Собранный `.vst3` появится в `build/Drumee_artefacts/Release/VST3`.

## Сборка в CI

Любой push в `main` или тег `vX.Y.Z` запускает `build.yml`: сборка на
Windows и macOS, архивация VST3 в zip и загрузка как artifact
(а на теге — публикация в Release).

## Известные ограничения прототипа

- Пресеты хранят пути к файлам сэмплов, а не сами аудиоданные — при
  переносе пресета на другой компьютер сэмплы нужно копировать отдельно.
- Синхронизация темпа берётся из хоста через `AudioPlayHead`; в
  Standalone-режиме используется 120 BPM по умолчанию.