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
    offset = (int(offset[0]), int(offset[1]))
    cropped_image = image.crop(
        (offset[0], offset[1], offset[0] + 128, offset[1] + 32))
    for x in range(cropped_image.size[0]):
        for y in range(cropped_image.size[1]):
            display_driver.DrawPixel(x, y,
                                     0 if cropped_image.getpixel((x, y)) > threshold
                                     else 1)


def DrawText(text, offset, size=10, font=None):
    if font is None:
        font = ImageFont.truetype(
            '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', size)
    image = Image.new('L', (128, 32), 0)
    draw = ImageDraw.Draw(image)
    draw.text(offset, text, fill=255, font=font)

    DrawImage(image, (0, 0), 128)
    display_driver.Update()


def ScrollText(text, offset, seconds, framerate, size=10):
    font = ImageFont.truetype(
        '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', size)
    string_size = font.getsize(text)
    scroll_target_x = 128 - string_size[0]
    scroll_pixels_per_second = scroll_target_x / seconds
    scroll_pixels_per_frame = scroll_pixels_per_second / framerate
    frame_times = []
    for x in np.arange(0, scroll_target_x + scroll_pixels_per_frame, scroll_pixels_per_frame):
        frame_start_time = time.clock_gettime(0)
        DrawText(text, np.array(offset) + np.array((x, 0)), size, font)
        display_driver.Update()
        frame_time = (time.clock_gettime(0) - frame_start_time)

        frame_times.append(frame_time)

        if (frame_time > (1 / framerate)):
            continue

        time.sleep((1 / framerate) - frame_time)
    print("Average FPS:", 1 / np.average(frame_times))


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
    vecs_xy = vecs_xy + ((np.array(image.size) - np.array((128, 32))) / 2)

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
