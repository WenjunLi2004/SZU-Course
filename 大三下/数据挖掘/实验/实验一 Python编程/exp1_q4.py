class ListNode(object):
    def __init__(self, val=0, next=None):
        self.val = val
        self.next = next


def addTwoNumbers(l1, l2):
    dummy = ListNode(0)
    current = dummy
    carry = 0

    while l1 is not None or l2 is not None or carry:
        x = l1.val if l1 is not None else 0
        y = l2.val if l2 is not None else 0

        total = x + y + carry
        carry = total // 10
        current.next = ListNode(total % 10)
        current = current.next

        if l1 is not None:
            l1 = l1.next
        if l2 is not None:
            l2 = l2.next

    return dummy.next


def build_list(values):
    dummy = ListNode(0)
    current = dummy
    for value in values:
        current.next = ListNode(value)
        current = current.next
    return dummy.next


def to_list(head):
    values = []
    while head is not None:
        values.append(head.val)
        head = head.next
    return values


if __name__ == "__main__":
    l1 = build_list([2, 4, 3])
    l2 = build_list([5, 6, 4])

    result = addTwoNumbers(l1, l2)
    print(to_list(result))  # [7, 0, 8]
