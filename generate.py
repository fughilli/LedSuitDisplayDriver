
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

import numpy
import math
import itertools


def CirclePoints(center, r, start_offset_points, num_points):
    circle_radians = numpy.pi * 2
    delta_theta = circle_radians / num_points
    start_theta = delta_theta * start_offset_points
    for theta in numpy.arange(0, circle_radians, delta_theta):
        yield (center +
               (r * numpy.array((math.cos(theta + start_theta),
                                 math.sin(theta + start_theta)))))


def GenerateEye(center, reverse, offset, offset_step):
    return itertools.chain(*([CirclePoints(center, 24*2, offset, 8),
                              CirclePoints(
                                  center, 42*2, offset + offset_step, 12),
                              CirclePoints(center, 60*2, offset + 2 * offset_step, 16)][::(-1 if reverse else 1)]))


def GenerateSampling():
    return itertools.chain(GenerateEye(numpy.array((200, 250)), False, 1.5, 0),
                           GenerateEye(numpy.array((475, 250)), True, 2.5, 2))
