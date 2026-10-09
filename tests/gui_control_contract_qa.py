#!/usr/bin/env python3
"""Offline binding regression for VSTGUI controls. Not a real mouse event test."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parent.parent
doc = ET.parse(root / "resource" / "DrumFinisher.uidesc").getroot()
source = (root / "src" / "vst" / "DrumPlugin.cpp").read_text(encoding="utf-8")
expected = {
    "Punch": 100, "Mass": 101, "Tight": 102, "Finish": 103,
    "Glue": 104, "Output": 105, "Character": 106,
    "Bypass": 107, "Zoom": 9000
}
tags = {}
for tag in doc.findall("./control-tags/control-tag"):
    name, value = tag.attrib["name"], int(tag.attrib["tag"])
    assert name not in tags, f"duplicate control tag name: {name}"
    assert value not in tags.values(), f"duplicate parameter ID: {value}"
    tags[name] = value
assert tags == expected, f"unexpected tag mapping: {tags!r}"
usage = {}
for view in doc.findall(".//view"):
    name = view.get("control-tag")
    if name is None:
        continue
    assert name in tags, f"unresolved VSTGUI control tag: {name!r}"
    usage[name] = usage.get(name, 0) + 1
assert all(usage.get(key, 0) > 0 for key in expected), usage

buttons = doc.findall(".//view[@class='CSegmentButton']")
assert len(buttons) == 3, "Expect native zoom/character/bypass segment controls"
for view in buttons:
    name = view.get("control-tag")
    expected_segments = {"Zoom": ["100%", "150%"],
                         "Character": ["TIGHT", "PUNCH", "DENSE"],
                         "Bypass": ["ON", "BYPASS"]}
    assert name in expected_segments, f"unknown segment control: {name}"
    assert view.get("segment-names", "").split(",") == expected_segments[name]
    assert view.get("mouse-enabled", "true") != "false"

# Verify native VSTGUI parameter binding, not the old manual controller
# callback that cannot receive VST3Editor ParameterChangeListener updates.
assert "control->setListener(editor);" in source
assert "tag==kCharacter||tag==kBypass" in source
assert "control->setListener(this);" in source  # Zoom only
# The branch deliberately uses "else if(...)" inside verifyView: it must
# attach the VST3Editor listener, not manually call the host edit methods.
assert source.count("}else if(tag==kCharacter||tag==kBypass){") == 1
assert "control->setListener(editor);" in source
assert "beginEdit(id);" not in source
assert "beginEdit(id);" not in source
assert "ParameterInfo::kIsBypass" in source
assert "bypassParameter->appendString(STR16(\"ON\"));" in source
assert "bypassParameter->appendString(STR16(\"BYPASS\"));" in source
assert 'enum Param : ParamID { kPunch=100, kBody=101, kTight=102, kFinish=103,' in source
assert 'kGlue=104, kOutput=105, kCharacter=106, kBypass=107' in source
assert "static constexpr int32 stateVersion=1" in source
print("PASS: nine resolved GUI tags; all three clickable segment controls; native VST3 parameter binding; state IDs preserved")
