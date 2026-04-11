#!/usr/bin/env python3

import io
import re
import urllib.request
from pathlib import Path

import packTab

BASE_URL = "https://www.unicode.org/Public/UCD/latest/ucd"

DOWNLOADS = [
    f"{BASE_URL}/emoji/emoji-data.txt",
    f"{BASE_URL}/auxiliary/GraphemeBreakProperty.txt",
    f"{BASE_URL}/DerivedCoreProperties.txt",
    f"{BASE_URL}/auxiliary/GraphemeBreakTest.txt",
]

SRCDIR = Path(__file__).resolve().parent.parent
OUTFILE = SRCDIR / "src" / "grapheme-data.h"
TESTFILE = SRCDIR / "tests" / "GraphemeBreakTest.txt"

GRAPHEME_BREAK_VALUES = {
    "RAQM_GRAPHEME_OTHER": "Other",
    "RAQM_GRAPHEME_CR": "CR",
    "RAQM_GRAPHEME_LF": "LF",
    "RAQM_GRAPHEME_CONTROL": "Control",
    "RAQM_GRAPHEME_EXTEND": "Extend",
    "RAQM_GRAPHEME_ZWJ": "ZWJ",
    "RAQM_GRAPHEME_REGIONAL_INDICATOR": "Regional_Indicator",
    "RAQM_GRAPHEME_PREPEND": "Prepend",
    "RAQM_GRAPHEME_SPACING_MARK": "SpacingMark",
    "RAQM_GRAPHEME_L": "L",
    "RAQM_GRAPHEME_V": "V",
    "RAQM_GRAPHEME_T": "T",
    "RAQM_GRAPHEME_LV": "LV",
    "RAQM_GRAPHEME_LVT": "LVT",
    "RAQM_GRAPHEME_EXTENDED_PICTOGRAPHIC": "Extended_Pictographic",
}

INCB_VALUES = {
    "RAQM_INCB_NONE": "None",
    "RAQM_INCB_CONSONANT": "Consonant",
    "RAQM_INCB_LINKER": "Linker",
    "RAQM_INCB_EXTEND": "Extend",
}


def download(url):
    print(f"Downloading {url}")
    with urllib.request.urlopen(url) as resp:
        return resp.read().decode("utf-8")


def parse_ranges(text, prop_name):
    ranges = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if prop_name not in line:
            continue
        m = re.match(r"([0-9A-Fa-f]+)(?:\.\.([0-9A-Fa-f]+))?\s*;\s*(\w+)", line)
        if not m:
            continue
        if m.group(3) != prop_name:
            continue
        start = int(m.group(1), 16)
        end = int(m.group(2), 16) if m.group(2) else start
        ranges.append((start, end))
    return ranges


def parse_incb(text):
    result = {"Consonant": [], "Linker": [], "Extend": []}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        m = re.match(
            r"([0-9A-Fa-f]+)(?:\.\.([0-9A-Fa-f]+))?\s*;\s*InCB\s*;\s*(\S+)", line
        )
        if not m:
            continue
        value = m.group(3)
        if value not in result:
            continue
        start = int(m.group(1), 16)
        end = int(m.group(2), 16) if m.group(2) else start
        result[value].append((start, end))
    return result


def get_unicode_version(text):
    for line in text.splitlines()[:5]:
        m = re.search(r"(\d+\.\d+\.\d+)", line)
        if m:
            return m.group(1)
    return "unknown"


def ranges_to_dict(ranges, value):
    d = {}
    for start, end in ranges:
        for cp in range(start, end + 1):
            d[cp] = value
    return d


def parse_all_property_values(text):
    values = set()
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("@"):
            continue
        m = re.match(r"[0-9A-Fa-f]+(?:\.\.[0-9A-Fa-f]+)?\s*;\s*(\w+)", line)
        if m:
            values.add(m.group(1))
    return values


def build_grapheme_break_data(emoji_data, grapheme_break_data):
    prop_to_enum = {v: k for k, v in GRAPHEME_BREAK_VALUES.items() if v != "Other"}

    found = parse_all_property_values(grapheme_break_data)
    found |= {"Extended_Pictographic"} & parse_all_property_values(emoji_data)
    unknown = found - set(prop_to_enum) - {"Other"}
    if unknown:
        raise ValueError(
            f"Unknown Grapheme_Cluster_Break values in Unicode data: {unknown}. "
            "Add them to GRAPHEME_BREAK_VALUES and handle in raqm.c."
        )

    data = {}

    for prop, enum_val in prop_to_enum.items():
        if prop == "Extended_Pictographic":
            continue
        for start, end in parse_ranges(grapheme_break_data, prop):
            for cp in range(start, end + 1):
                data[cp] = enum_val

    for start, end in parse_ranges(emoji_data, "Extended_Pictographic"):
        for cp in range(start, end + 1):
            if cp not in data:
                data[cp] = prop_to_enum["Extended_Pictographic"]

    return data


def build_incb_data(derived_core_properties_data):
    prop_to_enum = {v: k for k, v in INCB_VALUES.items() if v != "None"}
    incb = parse_incb(derived_core_properties_data)
    data = {}
    for prop, enum_val in prop_to_enum.items():
        data.update(ranges_to_dict(incb[prop], enum_val))
    return data


def gen_enum(name, values):
    lines = ["typedef enum\n{"]
    for i, member in enumerate(values):
        comma = "," if i < len(values) - 1 else ""
        lines.append(f"  {member}{comma}")
    lines.append(f"}} {name};\n")
    return "\n".join(lines)


def main():
    emoji_data = download(DOWNLOADS[0])
    grapheme_break_data = download(DOWNLOADS[1])
    derived_core_properties_data = download(DOWNLOADS[2])
    grapheme_break_test_data = download(DOWNLOADS[3])

    unicode_version = get_unicode_version(grapheme_break_data)
    print(f"Unicode version: {unicode_version}")

    grapheme_break = build_grapheme_break_data(emoji_data, grapheme_break_data)
    incb = build_incb_data(derived_core_properties_data)

    grapheme_break_mapping = {v: i for i, v in enumerate(GRAPHEME_BREAK_VALUES)}
    incb_mapping = {v: i for i, v in enumerate(INCB_VALUES)}

    grapheme_break_sol = packTab.pack_table(
        grapheme_break,
        "RAQM_GRAPHEME_OTHER",
        mapping=grapheme_break_mapping,
        compression=9,
    )
    incb_sol = packTab.pack_table(
        incb, "RAQM_INCB_NONE", mapping=incb_mapping, compression=9
    )

    out = io.StringIO()
    out.write(
        f"/* Generated by scripts/gen-grapheme-data.py from Unicode {unicode_version} */\n"
        "/* DO NOT EDIT MANUALLY */\n"
        "\n"
        "#ifndef _RAQM_GRAPHEME_DATA_H_\n"
        "#define _RAQM_GRAPHEME_DATA_H_\n"
        "\n"
    )

    out.write(gen_enum("_raqm_grapheme_t", GRAPHEME_BREAK_VALUES))
    out.write("\n")
    out.write(gen_enum("_raqm_incb_t", INCB_VALUES))
    out.write("\n")

    code = packTab.Code("_raqm")
    grapheme_break_sol.genCode(code, "get_grapheme_break", language="c")
    incb_sol.genCode(code, "get_incb", language="c")
    code.print_code(file=out, language="c")

    out.write("\n#endif /* _RAQM_GRAPHEME_DATA_H_ */\n")

    OUTFILE.write_text(out.getvalue())
    print(f"Wrote {OUTFILE}")

    TESTFILE.write_text(grapheme_break_test_data)
    print(f"Wrote {TESTFILE}")


if __name__ == "__main__":
    main()
