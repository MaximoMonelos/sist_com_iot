import re
import sys

path = 'lib/UniversalTelegramBot/src/UniversalTelegramBot.cpp'
try:
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
except FileNotFoundError:
    print(f"File not found: {path}")
    sys.exit(1)

# Replace DynamicJsonDocument
content = re.sub(r'DynamicJsonDocument\s+(\w+)\s*\([^)]+\)\s*;', r'JsonDocument \1;', content)

# Replace containsKey
content = re.sub(r'(\w+)\.containsKey\s*\(\s*("[^"]+")\s*\)', r'!\1[\2].isNull()', content)

# Replace createNestedObject
content = re.sub(r'(\w+)\.createNestedObject\s*\(\s*("[^"]+")\s*\)', r'\1[\2].to<JsonObject>()', content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Patch applied successfully.")
