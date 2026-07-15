def merge_and_sort(file_a, file_b, file_c):
    # utf-8-sig can safely read files with or without BOM
    with open(file_a, "r", encoding="utf-8-sig") as fa:
        a = fa.readline().strip()

    with open(file_b, "r", encoding="utf-8-sig") as fb:
        b = fb.readline().strip()

    merged = a + b
    result = "".join(sorted(merged))

    with open(file_c, "w", encoding="utf-8") as fc:
        fc.write(result)

    print("merge done ->", file_c)
    print("result:", result)


if __name__ == "__main__":
    merge_and_sort("test1.txt", "test2.txt", "test3.txt")
