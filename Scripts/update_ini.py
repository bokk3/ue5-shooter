with open('Config/DefaultGame.ini', 'r', encoding='utf-8') as f:
    text = f.read()

target = '+DirectoriesToAlwaysCook=(Path="/Game/Characters")'
replacement = '+DirectoriesToAlwaysCook=(Path="/Game/Characters")\n+DirectoriesToAlwaysCook=(Path="/Game/Environment")'

if target in text:
    text = text.replace(target, replacement, 1)
    with open('Config/DefaultGame.ini', 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(text)
    print("DefaultGame.ini updated successfully")
else:
    print("Target not found")
