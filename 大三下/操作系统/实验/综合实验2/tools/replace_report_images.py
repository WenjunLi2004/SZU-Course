from __future__ import annotations

import re
import shutil
import tempfile
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

from lxml import etree
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
DOCX = ROOT / "综合实验2_2023150001_李文俊.docx"
FIG = ROOT / "figures"

NS = {
    "w": "http://schemas.openxmlformats.org/wordprocessingml/2006/main",
    "v": "urn:schemas-microsoft-com:vml",
    "r": "http://schemas.openxmlformats.org/officeDocument/2006/relationships",
}


def parse_style(style: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for item in style.split(";"):
        if ":" not in item:
            continue
        key, value = item.split(":", 1)
        values[key.strip()] = value.strip()
    return values


def style_from(values: dict[str, str]) -> str:
    order = ["width", "height", "visibility", "mso-wrap-style"]
    parts = []
    for key in order:
        if key in values:
            parts.append(f"{key}:{values[key]}")
    for key, value in values.items():
        if key not in order:
            parts.append(f"{key}:{value}")
    return ";".join(parts)


def width_pt(style: str) -> float:
    match = re.search(r"width:([0-9.]+)pt", style)
    if not match:
        raise ValueError(f"cannot find width in style: {style}")
    return float(match.group(1))


def main() -> None:
    mapping = {
        f"word/media/image{i}.png": FIG / f"fig{i:02d}_{name}.png"
        for i, name in [
            (1, "fs_layout"),
            (2, "run_flow"),
            (3, "structs"),
            (4, "format"),
            (5, "bitmap_alloc"),
            (6, "write"),
            (7, "read_rm"),
            (8, "threads"),
            (9, "compile_server"),
            (10, "basic_server"),
            (11, "threads_server"),
            (12, "rundemo_server"),
        ]
    }
    missing = [str(path) for path in mapping.values() if not path.exists()]
    if missing:
        raise FileNotFoundError("\n".join(missing))

    with ZipFile(DOCX) as zin:
        doc_xml = zin.read("word/document.xml")
        rels_xml = zin.read("word/_rels/document.xml.rels")
        doc = etree.fromstring(doc_xml)
        rels = etree.fromstring(rels_xml)
        rel_targets = {rel.get("Id"): rel.get("Target") for rel in rels}

        for pict in doc.xpath(".//w:pict", namespaces=NS):
            shape = pict.find(".//v:shape", namespaces=NS)
            imagedata = pict.find(".//v:imagedata", namespaces=NS)
            if shape is None or imagedata is None:
                continue
            rid = imagedata.get(f"{{{NS['r']}}}id")
            target = rel_targets.get(rid)
            zip_name = f"word/{target}" if target else ""
            fig = mapping.get(zip_name)
            if fig is None:
                continue
            style = shape.get("style", "")
            w_pt = width_pt(style)
            with Image.open(fig) as im:
                new_h_pt = w_pt * im.height / im.width
            values = parse_style(style)
            values["height"] = f"{new_h_pt:.2f}pt"
            shape.set("style", style_from(values))

        updated_doc_xml = etree.tostring(
            doc, xml_declaration=True, encoding="UTF-8", standalone="yes"
        )

        tmp_path = Path(tempfile.mkstemp(suffix=".docx", dir=str(ROOT))[1])
        try:
            with ZipFile(tmp_path, "w", compression=ZIP_DEFLATED) as zout:
                for item in zin.infolist():
                    if item.filename == "word/document.xml":
                        zout.writestr(item, updated_doc_xml)
                    elif item.filename in mapping:
                        zout.writestr(item, mapping[item.filename].read_bytes())
                    else:
                        zout.writestr(item, zin.read(item.filename))
            shutil.move(str(tmp_path), DOCX)
        finally:
            if tmp_path.exists():
                tmp_path.unlink()

    print(DOCX)


if __name__ == "__main__":
    main()
