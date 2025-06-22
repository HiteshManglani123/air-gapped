from PIL import Image

# Create the cheeseburger image with 6x4 pixels
def create_cheeseburger_image():
    width, height = 4, 6
    img = Image.new("RGB", (width, height))

    # Define cheeseburger colors
    pixels = [
        (205, 133, 63),  # Top bun (light brown)
        (255, 0, 0),     # Tomato (red)
        (0, 255, 0),     # Lettuce (green)
        (255, 255, 0),   # Cheese (yellow)
        (139, 69, 19),   # Patty (brown)
        (205, 133, 63)   # Bottom bun (beige)
    ]

    # Fill the image with these colors
    for y in range(height):
        color = pixels[y % len(pixels)]
        for x in range(width):
            img.putpixel((x, y), color)

    img.save("cheeseburger.png")
    print("Cheeseburger image created.")

# Convert the message to binary bits
def message_to_bits(message):
    return ''.join(format(ord(c), '08b') for c in message) + '00000000'  # Add null terminator

# Embed message into the image using LSB
def hide_message(image_path, output_path, message):
    img = Image.open(image_path)
    img = img.convert('RGB')
    pixels = img.load()
    width, height = img.size

    bits = message_to_bits(message)
    bit_index = 0

    for y in range(height):
        for x in range(width):
            if bit_index >= len(bits):
                break

            r, g, b = pixels[x, y]
            # Embed bits into R, G, B
            if bit_index < len(bits):
                r = (r & ~1) | int(bits[bit_index])
                bit_index += 1
            if bit_index < len(bits):
                g = (g & ~1) | int(bits[bit_index])
                bit_index += 1
            if bit_index < len(bits):
                b = (b & ~1) | int(bits[bit_index])
                bit_index += 1

            pixels[x, y] = (r, g, b)

    img.save(output_path)
    print(f"Message hidden in {output_path}")

# Extract the hidden message from the image
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

    # Convert bits to characters
    chars = []
    for i in range(0, len(bits), 8):
        byte = bits[i:i+8]
        if byte == '00000000':  # Null terminator
            break
        chars.append(chr(int(byte, 2)))
    return ''.join(chars)

# --- Main Execution ---
if __name__ == "__main__":
    # Create the cheeseburger image
    create_cheeseburger_image()

    # Hide the message "404 sauce" in the image
    hide_message("cheeseburger.png", "cheeseburger_stego.png", "404 sauce")

    # Extract and display the hidden message
    extracted_message = extract_message("cheeseburger_stego.png")
    print("Extracted message:", extracted_message)
