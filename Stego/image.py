from PIL import Image

# Create a 6x4 pixel image
width, height = 4,6
img = Image.new("RGB", (width, height))

# Define colors (for example, cheeseburger-like colors)
pixels = [
    (205, 133, 63),  # Top bun (light brown)
    (255, 0, 0),     # Tomato (red)
    (0, 255, 0),     # Lettuce (green)
    (255, 255, 0),   # Cheese (yellow)
    (139, 69, 19),   # Patty (brown)
    (205, 133, 63)  # Bottom bun (beige)
]

# Fill each row
for y in range(height):
    color = pixels[y % len(pixels)]
    for x in range(width):
        img.putpixel((x, y), color)

# Save it
img.save("cheeseburger.png")
