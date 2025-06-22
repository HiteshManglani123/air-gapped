from PIL import Image

# Color definitions for terminal (ANSI codes)
GREEN = "\033[92m"
GRAY = "\033[90m"
RESET = "\033[0m"

# Function to colorize a byte used in decoding
def green_byte(byte_str):
    return GREEN + byte_str + RESET

def gray_byte(byte_str):
    return GRAY + byte_str + RESET

def extract_message(image_path):
    img = Image.open(image_path)
    img = img.convert('RGB')
    pixels = img.load()
    width, height = img.size

    bits = ""
    for y in range(height):
        for x in range(width):
            r, g, b = pixels[x, y]
            bits += str(r & 1)
            bits += str(g & 1)
            bits += str(b & 1)

    chars = []
    byte_chunks = []
    null_found = False

    print("Bitstream (color-coded by character):")
    for i in range(0, len(bits), 8):
        byte = bits[i:i+8]
        if len(byte) < 8:
            break
        if not null_found:
            if byte == '00000000':
                null_found = True
                byte_chunks.append(green_byte(byte) + "  <-- Null terminator")
                break
            char = chr(int(byte, 2))
            byte_chunks.append(green_byte(byte) + f"  # '{char}' (ASCII {ord(char)})")
            chars.append(char)
        else:
            byte_chunks.append(gray_byte(byte))

    # Print the color-coded bytes
    for chunk in byte_chunks:
        print(chunk)

    return ''.join(chars)

# --- Main Execution ---
if __name__ == "__main__":
    image_path = input("Enter the image filename: ")
    extracted_message = extract_message(image_path)
    print("\nExtracted message:", extracted_message)
