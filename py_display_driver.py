from PIL import Image, ImageDraw, ImageFont
import code
import led_driver.pywrap_display_driver
import numpy as np
import os
import time


display_driver = led_driver.pywrap_display_driver.DisplayDriver('/dev/i2c-1')
display_driver.Initialize()


def GetIpAddress():
    return os.popen("ip addr | awk '/wlan0/{f=1}/inet/ && f{print "
                    "substr($2, 1, length($2)-3); exit}'").read().strip()


def DrawImage(image, offset, threshold):
    for x in range(image.size[0]):
        draw_x = x + offset[0]
        if draw_x < 0 or draw_x >= 128:
            continue
        for y in range(image.size[1]):
            draw_y = y + offset[1]
            if draw_y < 0 or draw_y >= 32:
                continue
            display_driver.DrawPixel(draw_x, draw_y,
                                     0 if image.getpixel((x, y)) > threshold
                                     else 1)


def DrawText(text, size=10):
    font = ImageFont.truetype(
        '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', size)
    image = Image.new('L', (128, 32), 0)
    draw = ImageDraw.Draw(image)
    draw.text((0, 0), text, fill=255, font=font)

    DrawImage(image, (0, 0), 128)
    display_driver.Update()


def ClearDisplay():
    for x in range(128):
        for y in range(32):
            display_driver.DrawPixel(
                x, y, 1)

    display_driver.Update()


def DrawRotating(image, seconds, framerate, radius):
    ts = np.arange(0, seconds, 1 / framerate)
    vecs = radius * np.e ** (1j * np.pi * ts)
    vecs_xy = vecs.view('(2,)float')
    vecs_xy = vecs_xy - ((np.array(image.size) - np.array((128, 32))) / 2)

    frame_times = []
    for x, y in vecs_xy:
        frame_start_time = time.clock_gettime(0)
        DrawImage(image, (int(x), int(y)), 0)
        display_driver.Update()
        frame_time = (time.clock_gettime(0) - frame_start_time)

        frame_times.append(frame_time)

        if (frame_time > (1 / framerate)):
            continue

        time.sleep((1 / framerate) - frame_time)

    print("Average FPS:", 1 / np.average(frame_times))


code.interact(local=locals())
