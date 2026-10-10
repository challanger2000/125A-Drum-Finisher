#!/usr/bin/env python3
"""Static CTextButton/host mapping regression; real Studio One clicks remain user gate."""
from pathlib import Path
import xml.etree.ElementTree as ET

root=Path(__file__).resolve().parent.parent
doc=ET.parse(root/"resource"/"DrumFinisher.uidesc").getroot()
code=(root/"src"/"vst"/"DrumPlugin.cpp").read_text(encoding="utf-8")
expected={"Punch":100,"Mass":101,"Tight":102,"Finish":103,
          "Glue":104,"Output":105,"Character":106,"Bypass":107,"Zoom":9000,
          "CharacterTight":9101,"CharacterPunch":9102,"CharacterDense":9103,
          "ActiveButton":9104,"BypassButton":9105}
tags={}
for t in doc.findall("./control-tags/control-tag"):
    name,tag=t.attrib["name"],int(t.attrib["tag"])
    assert name not in tags and tag not in tags.values()
    tags[name]=tag
assert tags==expected,(tags,expected)
for v in doc.findall(".//view"):
    tag=v.get("control-tag")
    if tag is not None:assert tag in tags,tag

buttons=doc.findall(".//view[@class='CTextButton']")
assert len(buttons)==5,"Expected five separate real CTextButton controls"
names={"CharacterTight":"TIGHT","CharacterPunch":"PUNCH",
       "CharacterDense":"DENSE","ActiveButton":"ON",
       "BypassButton":"BYPASS"}
assert {v.get("control-tag") for v in buttons}==set(names)
for v in buttons:
    tag=v.get("control-tag")
    assert v.get("title")==names[tag],tag
    assert v.get("mouse-enabled")=="true",tag
    assert v.get("kick-style")=="false",tag
    assert v.get("default-value")=="0",tag
    assert v.get("min-value")=="0" and v.get("max-value")=="1",tag
    assert v.get("gradient-highlighted")=="SwitchSelected",tag

segments=doc.findall(".//view[@class='CSegmentButton']")
assert len(segments)==1 and segments[0].get("control-tag")=="Zoom"
for name in ("PUNCH","MASS","TIGHT","FINISH","GLUE"):
    assert any(v.get("title")==name+" (%)" for v in doc.findall(".//view"))
assert any(v.get("title")=="OUTPUT (dB)" for v in doc.findall(".//view"))
assert "std::array<VSTGUI::CTextButton*,5> buttons_" in code
assert "dynamic_cast<VSTGUI::CTextButton*>(control)" in code
assert "button->setListener(this)" in code
assert "const ParamID id=tag<=9103?kCharacter:kBypass" in code
for token in ("beginEdit(id);","setParamNormalized(id,value);",
              "performEdit(id,value);","endEdit(id);","refreshButtons();",
              "buttons_.fill(nullptr)"):
    assert token in code,token
assert "tresult PLUGIN_API setParamNormalized(ParamID tag, ParamValue value) override" in code
assert "kCharacter=106, kBypass=107" in code
assert "static constexpr int32 stateVersion=1" in code
print("PASS: five independent VSTGUI CTextButtons, 106/107 host gestures, %/dB labels, UI lifecycle mapping")
