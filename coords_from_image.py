# LED Suit Driver - Embedded host driver software for Kevin's LED suit controller.
# Copyright (C) 2019-2020 Kevin Balke
#
# This file is part of LED Suit Driver.
#
# LED Suit Driver is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LED Suit Driver is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LED Suit Driver.  If not, see <http://www.gnu.org/licenses/>.

"""coords_from_image.py: Compute LED coordinates from an image.

Usage:
    bazelisk run :coords_from_image -- \
            --input=image.png

coords_from_image iterates over all pixels in the input image, looking for blue
ones (defined by the B channel having a larger value than the R and G channels
summed). Each blue pixel's X and Y coordinates are recorded. A list of these
coordinates, with the minimum X and Y value of the whole list subtracted, is
output to STDOUT.

An additional offset can be applied to the coordinates with `--offset_x` and
`--offset_y`.
"""

from PIL import Image
from absl import app
from absl import flags
from absl import logging
from typing import Sequence

flags.DEFINE_string("input", None, "Image file to load LED locations from")
flags.DEFINE_integer(
    "offset_x", 0, "Offset to apply to the X coordinate of each LED location")
flags.DEFINE_integer(
    "offset_y", 0, "Offset to apply to the Y coordinate of each LED location")
flags.mark_flag_as_required("input")

FLAGS = flags.FLAGS


def ProcessImage(image: Image.Image, offset_x: int, offset_y: int) -> None:
    coordinates = []
    min_x = None
    min_y = None

    pixels = image.load()

    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = pixels[x, y]
            if (b > (r + g)):
                coordinates.append((x, y))
                if not min_x:
                    min_x = x
                else:
                    min_x = min(min_x, x)
                if not min_y:
                    min_y = y
                else:
                    min_y = min(min_y, y)

    coordinates = [(x - min_x + offset_x, y - min_y + offset_y)
                   for x, y in coordinates]

    logging.info("Total number of LEDs: %d", len(coordinates))
    logging.info("Coordinates: %s", coordinates)


def main(argv: Sequence[str]) -> None:
    with Image.open(FLAGS.input) as image:
        ProcessImage(image, FLAGS.offset_x, FLAGS.offset_y)


if __name__ == "__main__":
    app.run(main)
