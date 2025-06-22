from PIL import Image

# Function to extract the hidden message from the image
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

    # Convert bits to characters and keep track of bytes
    chars = []
    bytes_list = []
    for i in range(0, len(bits), 8):
        byte = bits[i:i+8]
        if byte == '00000000':  # Null terminator
            break
        bytes_list.append(byte)
        chars.append(chr(int(byte, 2)))

    # Print the bytes in binary form
    print("Extracted bytes (binary):")
    for b in bytes_list:
        print(b)
        
    return ''.join(chars)

# --- Main Execution ---
if __name__ == "__main__":
    image_path = input("Enter the image filename: ")
    extracted_message = extract_message(image_path)
    print("Extracted message:", extracted_message)

