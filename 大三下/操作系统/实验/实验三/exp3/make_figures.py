from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent
FIG = ROOT / "figures"
FIG.mkdir(exist_ok=True)

MONO = "/System/Library/Fonts/Menlo.ttc"
CJK = "/System/Library/Fonts/STHeiti Medium.ttc"


def font(path, size):
    return ImageFont.truetype(path, size)


def wrap_line(draw, text, fnt, max_px):
    if draw.textlength(text, font=fnt) <= max_px:
        return [text]
    chunks = []
    cur = ""
    for ch in text:
        trial = cur + ch
        if draw.textlength(trial, font=fnt) <= max_px:
            cur = trial
        else:
            if cur:
                chunks.append(cur)
            cur = ch
    if cur:
        chunks.append(cur)
    return chunks


def render_text(text, out, width=1500, font_path=MONO, font_size=24, line_height=1.35,
                bg=(255, 255, 255), fg=(28, 32, 36), pad=34, title=None):
    fnt = font(font_path, font_size)
    title_fnt = font(CJK, font_size + 6)
    dummy = Image.new("RGB", (10, 10), bg)
    draw = ImageDraw.Draw(dummy)
    lines = []
    for raw in text.rstrip("\n").splitlines():
        wrapped = wrap_line(draw, raw, fnt, width - 2 * pad)
        lines.extend(wrapped or [""])
    line_px = int(font_size * line_height)
    title_h = line_px + 16 if title else 0
    height = pad * 2 + title_h + max(1, len(lines)) * line_px
    im = Image.new("RGB", (width, height), bg)
    draw = ImageDraw.Draw(im)
    y = pad
    if title:
        draw.text((pad, y), title, font=title_fnt, fill=(18, 32, 48))
        y += title_h
    for line in lines:
        draw.text((pad, y), line, font=fnt, fill=fg)
        y += line_px
    im.save(FIG / out)


def numbered_excerpt(path, ranges):
    src = (ROOT / path).read_text().splitlines()
    out = []
    for start, end in ranges:
        for no in range(start, end + 1):
            if 1 <= no <= len(src):
                out.append(f"{no:>3}  {src[no - 1]}")
        out.append("")
    return "\n".join(out).rstrip()


def maps_summary():
    files = [
        "13_after_alloc_6.maps",
        "19_after_free_5.maps",
        "21_after_alloc_1024.maps",
        "23_after_alloc_64.maps",
    ]
    labels = [
        "after_alloc_6: 六个128MB块连续合并",
        "after_free_5: 释放2、3、5号后留下空洞",
        "after_alloc_1024: 1024MB向低地址扩展并与6号块合并",
        "after_alloc_64: 64MB复用2/3号合并空洞的高地址端",
    ]
    lines = []
    for label, name in zip(labels, files):
        lines.append(label)
        for line in (ROOT / "snapshots" / name).read_text().splitlines():
            parts = line.split()
            if len(parts) >= 5 and parts[1] == "rw-p" and parts[2] == "00000000" and parts[3] == "00:00" and parts[4] == "0" and "[" not in line:
                a, b = parts[0].split("-")
                size = (int(b, 16) - int(a, 16)) / (1024 * 1024)
                if size > 1:
                    lines.append(f"  {parts[0]}    {size:7.1f} MB")
        lines.append("")
    return "\n".join(lines).rstrip()


def compact_touch_log():
    read = (ROOT / "logs" / "touch_read_3072.log").read_text().splitlines()
    write = (ROOT / "logs" / "touch_write_3072.log").read_text().splitlines()

    def pick(lines):
        keep = []
        active = False
        for line in lines:
            if line.startswith("pid=") or "touch finished" in line or "page faults" in line:
                keep.append(line)
            if line.startswith("===== after mmap") or line.startswith("===== after touch"):
                active = True
                keep.append(line)
                continue
            if active and (line.startswith("VmSize:") or line.startswith("VmRSS:") or line.startswith("RssAnon:") or line.startswith("VmSwap:")):
                keep.append(line)
            if active and line.startswith("VmSwap:"):
                active = False
        return keep

    return "READ PASS\n" + "\n".join(pick(read)) + "\n\nWRITE PASS\n" + "\n".join(pick(write))


def compact_vm_layout_log():
    lines = (ROOT / "logs" / "vm_layout.log").read_text().splitlines()
    keep = []
    wanted_snapshots = {
        "===== snapshot 13: after_alloc_6 =====",
        "===== snapshot 19: after_free_5 =====",
        "===== snapshot 21: after_alloc_1024 =====",
        "===== snapshot 23: after_alloc_64 =====",
    }
    capture_status = False
    for line in lines:
        if line.startswith("pid=") or line.startswith("block[") or line.startswith("freed block") or line.startswith("big_block") or line.startswith("small_block") or line.startswith("Prediction:"):
            keep.append(line)
        if line in wanted_snapshots:
            keep.append("")
            keep.append(line)
            capture_status = True
            continue
        if capture_status and (line.startswith("VmSize:") or line.startswith("VmRSS:") or line.startswith("VmData:")):
            keep.append(line)
        if capture_status and line.startswith("VmData:"):
            capture_status = False
    return "\n".join(keep).strip()


def flowchart():
    labels = [
        ("创建exp3工作目录", "编译四个实验程序"),
        ("虚存布局实验", "连续分配6个128MB并释放2、3、5号"),
        ("大块再分配", "申请1024MB并预测64MB落点"),
        ("缺页机制实验", "3GB映射分别执行读触页与写触页"),
        ("物理页竞争", "A保持1792MB，B写入4096MB"),
        ("局部性对比", "全空间循环与16页分组访问计时"),
        ("整理报告", "按Title1、Title2、Description、Image写入Word"),
    ]
    w, h = 1500, 1220
    im = Image.new("RGB", (w, h), (255, 255, 255))
    draw = ImageDraw.Draw(im)
    title_f = font(CJK, 38)
    box_f = font(CJK, 28)
    sub_f = font(CJK, 22)
    draw.text((60, 45), "graph TD 竖向流程图", font=title_f, fill=(28, 47, 69))
    y = 130
    x = 190
    bw, bh = 1120, 115
    colors = [
        (226, 241, 255), (235, 248, 239), (255, 244, 224),
        (239, 235, 255), (255, 235, 238), (232, 247, 247), (245, 245, 245),
    ]
    for i, (main, sub) in enumerate(labels):
        fill = colors[i % len(colors)]
        outline = (72, 99, 128)
        draw.rounded_rectangle((x, y, x + bw, y + bh), radius=18, fill=fill, outline=outline, width=3)
        draw.text((x + 36, y + 24), main, font=box_f, fill=(20, 39, 58))
        draw.text((x + 36, y + 68), sub, font=sub_f, fill=(68, 82, 97))
        if i < len(labels) - 1:
            cx = x + bw // 2
            draw.line((cx, y + bh + 8, cx, y + bh + 52), fill=(72, 99, 128), width=4)
            draw.polygon([(cx, y + bh + 64), (cx - 11, y + bh + 45), (cx + 11, y + bh + 45)], fill=(72, 99, 128))
        y += 150
    im.save(FIG / "flowchart.png")


def main():
    flowchart()
    render_text(
        numbered_excerpt("vm_layout.c", [(44, 69), (78, 130)]),
        "code_vm_layout.png",
        title="vm_layout.c核心代码",
    )
    render_text(
        numbered_excerpt("max_vmem.c", [(19, 25), (36, 63)]),
        "code_max_vmem.png",
        title="max_vmem.c核心代码",
    )
    render_text(
        numbered_excerpt("touch_pages.c", [(18, 42), (72, 101)]),
        "code_touch_pages.png",
        title="touch_pages.c核心代码",
    )
    render_text(
        numbered_excerpt("locality_compare.c", [(31, 49), (65, 86)]),
        "code_locality.png",
        title="locality_compare.c核心代码",
    )
    render_text(maps_summary(), "maps_summary.png", title="/proc/pid/maps关键匿名映射片段", font_path=CJK)
    render_text(compact_vm_layout_log(), "vm_layout_log.png", title="vm_layout运行关键日志", width=1600, font_size=23)
    render_text((ROOT / "logs" / "max_vmem.log").read_text(), "max_vmem_log.png", title="max_vmem运行日志")
    render_text(compact_touch_log(), "touch_compare.png", title="读触页与写触页状态对比", width=1600, font_size=23)
    render_text((ROOT / "logs" / "competition_summary.log").read_text() + "\n\n" + (ROOT / "logs" / "competition_B.log").read_text(),
                "competition.png", title="双进程物理内存竞争结果", width=1600, font_size=23)
    render_text((ROOT / "logs" / "locality_global_4096x3.log").read_text() + "\n\n" + (ROOT / "logs" / "locality_group16_4096x3.log").read_text(),
                "locality_compare.png", title="访问局部性时间对比", width=1500, font_size=24)


if __name__ == "__main__":
    main()
