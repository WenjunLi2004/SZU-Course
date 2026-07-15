from __future__ import annotations

import base64
import re
import textwrap
import urllib.request
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
FIG = ROOT / "figures"
FIG.mkdir(exist_ok=True)

MONO_FONT = "/System/Library/Fonts/Menlo.ttc"
SANS_FONT = "/System/Library/Fonts/STHeiti Medium.ttc"


def font(path: str, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(path, size)


def line_height(draw: ImageDraw.ImageDraw, fnt: ImageFont.FreeTypeFont) -> int:
    box = draw.textbbox((0, 0), "Ag", font=fnt)
    return box[3] - box[1] + 10


def text_width(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.FreeTypeFont) -> int:
    return draw.textbbox((0, 0), text, font=fnt)[2]


def wrap_line(line: str, width: int) -> list[str]:
    if len(line) <= width:
        return [line]
    return textwrap.wrap(
        line,
        width=width,
        replace_whitespace=False,
        drop_whitespace=False,
        break_long_words=True,
        break_on_hyphens=False,
    )


def render_mermaid(mmd_path: Path, out_path: Path) -> str:
    code = mmd_path.read_text(encoding="utf-8")
    enc = base64.urlsafe_b64encode(code.encode("utf-8")).decode("ascii").rstrip("=")
    url = f"https://mermaid.ink/img/{enc}?type=png&bgColor=FFFFFF"
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    with urllib.request.urlopen(req, timeout=30) as resp:
        data = resp.read()
    if not data.startswith(b"\x89PNG"):
        raise RuntimeError(f"Mermaid endpoint did not return PNG for {mmd_path.name}")
    out_path.write_bytes(data)
    (out_path.with_suffix(out_path.suffix + ".url.txt")).write_text(url + "\n", encoding="utf-8")
    return url


def render_code_card(title: str, code: str, out_path: Path, max_cols: int = 92) -> None:
    del title
    mono = font(MONO_FONT, 28)
    tmp = Image.new("RGB", (10, 10))
    draw = ImageDraw.Draw(tmp)
    lines: list[str] = []
    for raw in code.rstrip().splitlines():
        lines.extend(wrap_line(raw.expandtabs(4), max_cols) or [""])
    lh = line_height(draw, mono) + 2
    line_no_w = text_width(draw, str(len(lines)) + "  ", mono)
    content_w = max(text_width(draw, line, mono) for line in lines) + line_no_w + 118
    width = max(1120, min(2050, content_w + 80))
    height = 76 + lh * len(lines) + 62
    img = Image.new("RGB", (width, height), "#ffffff")
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle((4, 4, width - 5, height - 5), radius=26,
                           fill="#ffffff", outline="#cfd4dc", width=3)

    keywords = {
        "typedef", "struct", "static", "uint32_t", "uint8_t", "int", "void",
        "return", "if", "for", "while", "const", "char", "size_t", "pthread_t",
        "SuperBlock", "SfsInode", "ThreadJob",
    }
    y = 46
    for idx, line in enumerate(lines, 1):
        draw.text((60, y), f"{idx:>3}", font=mono, fill="#a6adbd")
        x = 60 + line_no_w + 28
        comment_at = line.find("//")
        segments = [(line, False)] if comment_at < 0 else [
            (line[:comment_at], False), (line[comment_at:], True)
        ]
        for segment, is_comment in segments:
            if is_comment:
                draw.text((x, y), segment, font=mono, fill="#6b7280")
                x += text_width(draw, segment, mono)
                continue
            for match in re.finditer(r"\"(?:\\.|[^\"])*\"|\d+u?|\w+|\W+", segment):
                token = match.group(0)
                color = "#24272d"
                if token.strip() in keywords:
                    color = "#2f9de6"
                elif re.fullmatch(r"\d+u?", token.strip()):
                    color = "#f59e0b"
                elif token.startswith("\""):
                    color = "#15803d"
                elif token in {"->", ".", "*", "&", "=", "+", "-", "/", "%", "!", "<", ">"}:
                    color = "#64748b"
                draw.text((x, y), token, font=mono, fill=color)
                x += text_width(draw, token, mono)
        y += lh
    img.save(out_path)


def render_terminal(title: str, text: str, out_path: Path, max_cols: int = 88) -> None:
    del title
    mono = font(MONO_FONT, 28)
    tmp = Image.new("RGB", (10, 10))
    draw = ImageDraw.Draw(tmp)

    lines: list[str] = []
    for raw in text.strip("\n").splitlines():
        if raw.startswith("Connection to ") or raw == "exit":
            continue
        lines.extend(wrap_line(raw.rstrip(), max_cols) or [""])

    lh = line_height(draw, mono) + 3
    width = 1720
    height = 58 + lh * len(lines) + 46
    img = Image.new("RGB", (width, height), "#edf2f7")
    draw = ImageDraw.Draw(img)

    y = 36
    prompt_re = re.compile(r"^(wenjun@2023150001)(:[^$]*\$ )(.*)$")
    for line in lines:
        x = 34
        m = prompt_re.match(line)
        if m:
            userhost, path, cmd = m.groups()
            draw.text((x, y), userhost, font=mono, fill="#15803d")
            x += text_width(draw, userhost, mono)
            draw.text((x, y), path, font=mono, fill="#0969da")
            x += text_width(draw, path, mono)
            draw.text((x, y), cmd, font=mono, fill="#475569")
        elif line.startswith("gcc ") or line.startswith("rm -f "):
            draw.text((x, y), line, font=mono, fill="#475569")
        elif line.startswith("[format]"):
            draw.text((x, y), line, font=mono, fill="#1d4ed8")
        elif line.startswith("[write]") or line.startswith("[open]"):
            draw.text((x, y), line, font=mono, fill="#047857")
        elif line.startswith("[read]") or line.startswith("[ls]"):
            draw.text((x, y), line, font=mono, fill="#7c2d12")
        else:
            draw.text((x, y), line, font=mono, fill="#334155")
        y += lh
    img.save(out_path)


def segment_between(lines: list[str], start_prefix: str, end_prefix: str | None) -> str:
    start = next(i for i, line in enumerate(lines) if line.startswith(start_prefix))
    if end_prefix is None:
        end = len(lines)
    else:
        end = next(i for i, line in enumerate(lines[start + 1 :], start + 1)
                   if line.startswith(end_prefix))
    return "\n".join(lines[start:end])


def extract_code(start_marker: str, end_marker: str) -> str:
    src = (ROOT / "sfs.c").read_text(encoding="utf-8")
    start = src.index(start_marker)
    end = src.index(end_marker, start)
    return src[start:end].strip()


def main() -> None:
    render_mermaid(FIG / "fig01_fs_layout.mmd", FIG / "fig01_fs_layout.png")
    render_mermaid(FIG / "fig02_run_flow.mmd", FIG / "fig02_run_flow.png")

    src = (ROOT / "sfs.c").read_text(encoding="utf-8")
    ds = src[src.index("typedef struct {\n    uint32_t magic;"):src.index("static uint8_t *g_disk")]
    render_code_card("核心数据结构：SuperBlock 与 SfsInode", ds, FIG / "fig03_structs.png")
    render_code_card(
        "格式化：把 20M 空间划分为元数据区与数据区",
        extract_code("void sfs_format", "int sfs_open"),
        FIG / "fig04_format.png",
        max_cols=88,
    )
    render_code_card(
        "资源分配：inode 位图与数据块位图",
        extract_code("static int alloc_inode_unlocked", "void sfs_format"),
        FIG / "fig05_bitmap_alloc.png",
        max_cols=88,
    )
    render_code_card(
        "覆盖写入：释放旧块、分配新块、按 1KB 写入",
        extract_code("int sfs_write", "int sfs_read"),
        FIG / "fig06_write.png",
        max_cols=88,
    )
    render_code_card(
        "读取与删除接口：按块索引读回并回收资源",
        extract_code("int sfs_read", "void sfs_list"),
        FIG / "fig07_read_rm.png",
        max_cols=88,
    )
    render_code_card(
        "两个 pthread 子线程分别创建、写入、读取两个文件",
        extract_code("static void *worker", "static void run_basic_interface_demo"),
        FIG / "fig08_threads.png",
        max_cols=88,
    )

    lines = (ROOT / "remote_screens_session.txt").read_text(encoding="utf-8").splitlines()
    render_terminal(
        "服务器编译验证",
        segment_between(lines, "wenjun@2023150001:~/exp2_sfs$ make clean && make",
                        "wenjun@2023150001:~/exp2_sfs$ ./sfs | sed -n '1,18p'"),
        FIG / "fig09_compile_server.png",
    )
    render_terminal(
        "初始化与基础接口验证",
        segment_between(lines, "wenjun@2023150001:~/exp2_sfs$ ./sfs | sed -n '1,18p'",
                        "wenjun@2023150001:~/exp2_sfs$ ./sfs | sed -n '19,80p'"),
        FIG / "fig10_basic_server.png",
    )
    render_terminal(
        "双线程写读与最终文件表",
        segment_between(lines, "wenjun@2023150001:~/exp2_sfs$ ./sfs | sed -n '19,80p'",
                        "wenjun@2023150001:~/exp2_sfs$ ./run_demo.sh | tail -24"),
        FIG / "fig11_threads_server.png",
    )
    render_terminal(
        "run_demo.sh 复现验证",
        segment_between(lines, "wenjun@2023150001:~/exp2_sfs$ ./run_demo.sh | tail -24",
                        "wenjun@2023150001:~/exp2_sfs$ exit"),
        FIG / "fig12_rundemo_server.png",
    )


if __name__ == "__main__":
    main()
