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
          "BypassButton":9105}
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
assert len(buttons)==4,"Expected 3 character keys and 1 bypass toggle"
names={"CharacterTight":"TIGHT","CharacterPunch":"PUNCH",
       "CharacterDense":"DENSE","BypassButton":"BYPASS"}
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
# Headers are deliberately name-only. Units belong to *dynamic values*
# underneath the knobs, and must update with automation / host recall.
for name in ("PUNCH","MASS","TIGHT","FINISH","GLUE","OUTPUT"):
    assert any(v.get("title")==name for v in doc.findall(".//view"))
assert not any(v.get("title") in (
    "PUNCH (%)","MASS (%)","TIGHT (%)","FINISH (%)","GLUE (%)",
    "OUTPUT (dB)") for v in doc.findall(".//view"))
for name in ("Punch","Mass","Tight","Finish","Glue","Output"):
    displays=[v for v in doc.findall(".//view[@class='CParamDisplay']")
              if v.get("control-tag")==name]
    assert len(displays)==1, "value display missing or duplicated: "+name
assert "getParamStringByValue(" in code
assert "id>=kPunch && id<=kGlue" in code
assert '"%.0f %%"' in code
assert '"0.0 dB"' in code and '"%+.1f dB"' in code
assert "24.0*clampUnit(valueNormalized)-12.0" in code
assert "std::array<VSTGUI::CTextButton*,4> buttons_" in code
assert "dynamic_cast<VSTGUI::CTextButton*>(control)" in code
assert "button->setListener(this)" in code
assert "const ParamID id=tag<=9103?kCharacter:kBypass" in code
assert "double(control->getValueNormalized()>=0.5f)" in code, "one toggle must emit both states"
assert "tag==9105?3:tag-9101" in code
bypass=[v for v in buttons if v.get("control-tag")=="BypassButton"][0]
assert bypass.get("origin")=="1010,82" and bypass.get("size")=="154,30"
assert all(v.get("origin").endswith(",428") for v in buttons if v is not bypass)
for token in ("beginEdit(id);","setParamNormalized(id,value);",
              "performEdit(id,value);","endEdit(id);","refreshButtons();",
              "buttons_.fill(nullptr)"):
    assert token in code,token
assert "tresult PLUGIN_API setParamNormalized(ParamID tag, ParamValue value) override" in code
assert "kCharacter=106, kBypass=107" in code
assert "static constexpr int32 stateVersion=1" in code
print("PASS: three character CTextButtons plus one header BYPASS toggle, 106/107 host gestures, plain headers and %/dB dynamic value text, UI lifecycle mapping")
