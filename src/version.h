// version.h: версия rus1scan.
//
// Основная версия (MAJOR.MINOR) меняется вручную. Номер сборки (BUILD) подставляет CI
// через свойство MSBuild Rus1scanBuild (номер запуска GitHub Actions); при локальной
// сборке он равен 0. Релиз в CI получает тег vMAJOR.MINOR.BUILD.

#pragma once

#define RUS1SCAN_VERSION_MAJOR 4
#define RUS1SCAN_VERSION_MINOR 0

#ifndef RUS1SCAN_BUILD
#define RUS1SCAN_BUILD 0
#endif

#define RUS1SCAN_STRINGIZE2(x) #x
#define RUS1SCAN_STRINGIZE(x)  RUS1SCAN_STRINGIZE2(x)

// Строка версии вида "4.0.17" (одним литералом: компилятор ресурсов не склеивает строки)
#define RUS1SCAN_VERSION_STRING \
    RUS1SCAN_STRINGIZE(RUS1SCAN_VERSION_MAJOR.RUS1SCAN_VERSION_MINOR.RUS1SCAN_BUILD)
