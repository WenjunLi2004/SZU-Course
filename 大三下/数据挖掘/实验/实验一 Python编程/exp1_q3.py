def displayInventory(inventory):
    print("Inventory:")
    total_items = 0

    for item_name, item_count in inventory.items():
        print(f"{item_count} {item_name}")
        total_items += item_count

    print(f"Total number of items: {total_items}")


if __name__ == "__main__":
    stuff = {
        "arrow": 12,
        "gold coin": 42,
        "rope": 1,
        "torch": 6,
        "dagger": 1,
    }
    displayInventory(stuff)
