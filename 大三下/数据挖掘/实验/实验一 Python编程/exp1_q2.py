def build_names():
    return ["Lihua", "Rain", "Jack", "Xiuxiu", "Peiqi", "Black"]


def operation_1():
    names = build_names()
    black_index = names.index("Black")
    names.insert(black_index, "Blue")
    names.insert(black_index + 2, "White")
    print("Operation 1:", names)


def operation_2():
    names = build_names()
    xiuxiu_index = names.index("Xiuxiu")
    names[xiuxiu_index] = "\u79c0\u79c0"
    print("Operation 2:", names)


def operation_3():
    names = build_names()
    extra_list = [1, 2, 3, 4, 2, 5, 6, 2]
    names.extend(extra_list)
    print("Operation 3 (extended):", names)

    picked = names[2:11:2]  # index 2 to 10, step = 2
    print("Operation 3 (index 2-10, step=2):", picked)


if __name__ == "__main__":
    operation_1()
    operation_2()
    operation_3()
