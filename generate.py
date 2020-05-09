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
                           GenerateEye(numpy.array((650, 250)), True, 2.5, 2))
