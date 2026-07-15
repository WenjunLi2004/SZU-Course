def lengthOfLongestSubstring(s):
    left = 0
    max_len = 0
    last_pos = {}

    for right, ch in enumerate(s):
        if ch in last_pos and last_pos[ch] >= left:
            left = last_pos[ch] + 1

        last_pos[ch] = right
        max_len = max(max_len, right - left + 1)

    return max_len


if __name__ == "__main__":
    s1 = "abcabcbb"
    s2 = "bbbbb"

    print(f"s = {s1}, length = {lengthOfLongestSubstring(s1)}")
    print(f"s = {s2}, length = {lengthOfLongestSubstring(s2)}")
