"""Генерирует wia_names_table.inc - таблицы имён WIA для секции [wia] в rus1scan.toml.

Источники:
  - WIA_CONSTANTS.TXT.H из папки проекта (справочник, на который ссылается конфиг);
  - WiaDef.h из Windows SDK (полный набор; в WIA_CONSTANTS.TXT.H часть файла обрезана).

Свойство WIA - это макрос NAME с числовым значением, для которого есть пара NAME_STR
с отображаемым именем (так устроен WiaDef.h). Остальные числовые макросы считаются
константами значений (FEEDER, WIA_INTENT_IMAGE_TYPE_COLOR, WIA_PAGE_A4, ...).

Запуск из папки проекта:  python tools\\gen_wia_names.py [путь к WiaDef.h]
"""
import glob
import os
import re
import sys

PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFINE = re.compile(r'^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.+?)\s*$')
INT = re.compile(r'^\(?\s*(0[xX][0-9A-Fa-f]+|\d+)\s*[uUlL]*\s*\)?$')
WSTR = re.compile(r'^L"((?:[^"\\]|\\.)*)"$')


def find_sdk_wiadef():
    """Ищет WiaDef.h самой новой версии Windows SDK."""
    pattern = r'C:\Program Files (x86)\Windows Kits\10\Include\*\um\WiaDef.h'
    candidates = sorted(glob.glob(pattern))
    return candidates[-1] if candidates else None


def parse(path):
    """Возвращает словарь NAME -> сырое значение макроса (без комментариев)."""
    result = {}
    with open(path, encoding='cp1251', errors='replace') as f:
        for line in f:
            line = line.split('//')[0]
            m = DEFINE.match(line)
            if m and m.group(1) not in result:
                result[m.group(1)] = m.group(2).strip()
    return result


def resolve_int(name, defines, depth=0):
    """Вычисляет целое значение макроса, раскрывая псевдонимы (WIA_PAGE_ISO_A4 -> WIA_PAGE_A4)."""
    raw = defines.get(name)
    if raw is None or depth > 8:
        return None
    m = INT.match(raw)
    if m:
        return int(m.group(1), 0)
    if re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', raw):
        return resolve_int(raw, defines, depth + 1)
    return None


def main():
    sdk = sys.argv[1] if len(sys.argv) > 1 else find_sdk_wiadef()
    sources = [os.path.join(PROJECT, 'WIA_CONSTANTS.TXT.H')]
    if sdk and os.path.exists(sdk):
        sources.append(sdk)
    else:
        print('warning: WiaDef.h from Windows SDK not found, using WIA_CONSTANTS.TXT.H only')

    defines = {}
    for src in sources:
        for k, v in parse(src).items():
            defines.setdefault(k, v)

    properties = {}
    for name, raw in defines.items():
        if name.endswith('_STR') or name + '_STR' not in defines:
            continue
        value = resolve_int(name, defines)
        m = WSTR.match(defines[name + '_STR'])
        if value is not None and m:
            properties[name] = (value, m.group(1))

    values = {}
    for name in defines:
        if name in properties or name.endswith('_STR') or name.startswith('_'):
            continue
        value = resolve_int(name, defines)
        if value is not None and -2**31 <= value < 2**32:
            values[name] = value

    out = os.path.join(PROJECT, 'wia_names_table.inc')
    with open(out, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write('// wia_names_table.inc: таблицы имён WIA для секции [wia] в rus1scan.toml.\n')
        f.write('// СГЕНЕРИРОВАНО tools\\gen_wia_names.py из ' +
                ' и '.join(os.path.basename(s) for s in sources) + ' - не редактировать вручную.\n\n')
        f.write('static const WiaPropertyName kWiaPropertyNames[] =\n{\n')
        for name, (value, display) in sorted(properties.items(), key=lambda kv: (kv[1][0], kv[0])):
            f.write(f'    {{ L"{name}", {value}, L"{display}" }},\n')
        f.write('};\n\nstatic const WiaValueName kWiaValueNames[] =\n{\n')
        for name, value in sorted(values.items()):
            # Значения >= 2^31 (и отрицательные) записываем как 32-битное беззнаковое
            # с приведением: литерал -2147483648 в C++ недопустим.
            literal = str(value) if 0 <= value < 2**31 else f'static_cast<long>(0x{value & 0xFFFFFFFF:08X}UL)'
            f.write(f'    {{ L"{name}", {literal} }},\n')
        f.write('};\n')
    print(f'{out}: {len(properties)} properties, {len(values)} value constants from {len(sources)} file(s)')


if __name__ == '__main__':
    main()
