import time, Image, struct

rgb_buf = open("rgb_buf", "rb")

im = Image.fromstring("RGB", (320, 240), rgb_buf.read())
im.save("shot.png")
