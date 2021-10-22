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
        yield (center + (r * numpy.array(
            (math.cos(theta + start_theta), math.sin(theta + start_theta)))))


def GenerateEye(center, reverse, offset, offset_step):
    return itertools.chain(*([
        CirclePoints(center, 24 * 2, offset, 8),
        CirclePoints(center, 42 * 2, offset + offset_step, 12),
        CirclePoints(center, 60 * 2, offset + 2 * offset_step, 16)
    ][::(-1 if reverse else 1)]))


def GeneratePadding(center, number):
    return center + numpy.zeros((number, 2))


def GenerateCenteredLine(center, pitch, width):
    flipped = False
    if width < 0:
        flipped = True
        width = -width
    spaces = width - 1
    for i in range(width):
        if flipped:
            yield center + (spaces / 2 - i) * pitch * numpy.array((1, 0))
        else:
            yield center - (spaces / 2 - i) * pitch * numpy.array((1, 0))


def GenerateCenteredStack(bottom_center, pitch, widths):
    return itertools.chain(*(
        GenerateCenteredLine(bottom_center -
                             n * pitch * numpy.array((0, 1)), pitch, widths[n])
        for n in range(len(widths))))


left_eye_center = numpy.array((200, 600))
right_eye_center = numpy.array((475, 600))


def GenerateSampling():
    return itertools.chain(
        # Eyes on glasses
        GenerateEye(left_eye_center, False, 1.5, 0),
        GenerateEye(right_eye_center, True, 2.5, 2),
        # Throwaway indices to get to next driver port starting at index 300
        GeneratePadding(numpy.array((337.5, 450)), 300 - 72),
        # Squid hat
        GenerateCenteredStack(numpy.array((337.5, 400)), 40,
                              [15, -14, 13, -12, 10, -9, 7, -5, 4]))


def GeneratePointsOfInterest():
    return [
        left_eye_center,
        right_eye_center,
    ]
