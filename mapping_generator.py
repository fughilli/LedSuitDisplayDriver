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

from absl import app
from absl import flags
from dataclasses import dataclass
from google.protobuf import text_format
from led_driver.led_mapping_pb2 import Mapping
from typing import Optional, Tuple, List
import importlib.util
import numpy
import os
import pickle
import pygame
import watchdog.events
import watchdog.observers

flags.DEFINE_string("generate_file", None,
                    "Script to run to generate the LED mapping")
flags.DEFINE_string("state_file", None, "Where to cache the editor state")
flags.mark_flag_as_required("generate_file")
flags.DEFINE_string("export_file", None, "File to export the mapping to")
flags.mark_flag_as_required("export_file")
flags.DEFINE_bool("export_only", False,
                  "Only export the mapping, and then exit")
flags.DEFINE_integer("border_size", 20,
                     "Pixels to add around the sampling array")
flags.DEFINE_bool(
    "preserve_aspect_ratio", False,
    "Whether or not to preserve the aspect ratio of the LED array")

FLAGS = flags.FLAGS


class GenerateModifiedHandler(watchdog.events.FileSystemEventHandler):
    def __init__(self, filename, handler):
        self.filename = filename
        self.handler = handler

        print("GenerateModifiedHandler initialized for", self.filename)

        super(GenerateModifiedHandler, self).__init__()

    def on_modified(self, event):
        print(event)
        if (event.event_type == "modified" and self.filename == event.src_path
                and event.is_directory == False):
            self.handler()


def ContainsByReference(value, array) -> bool:
    for i in range(len(array)):
        if value is array[i]:
            return True
    return False


@dataclass
class Sample:
    coordinate: numpy.array = numpy.array((0, 0))
    predecessor: Optional["Sample"] = None
    successor: Optional["Sample"] = None
    index: int = -1
    display_index: int = -1

    def FindExtremeSuccessor(self) -> "Sample":
        visited = [self]

        while True:
            next_sample = visited[-1].successor
            if next_sample is None:
                return visited[-1]
            if ContainsByReference(next_sample, visited):
                return visited[-1]
            visited.append(visited[-1].successor)

    def GetSuccessors(self) -> List["Sample"]:
        visited = [self]

        while True:
            next_sample = visited[-1].successor
            if next_sample is None:
                return visited
            if ContainsByReference(next_sample, visited):
                return visited
            visited.append(visited[-1].successor)

    def UpdateDisplayIndex(self) -> bool:
        return self.FindExtremeSuccessor().UpdateDisplayIndexHelper()

    def UpdateDisplayIndexHelper(self) -> bool:
        self.display_index = -1
        if self.predecessor:
            if self.predecessor.UpdateDisplayIndexHelper():
                self.display_index = self.predecessor.display_index + 1
                return True
            return False
        if self.index != -1:
            self.display_index = self.index
            return True
        return False

    def SetIndex(self, index: int):
        self.index = index
        self.predecessor = None


class MappingGenerator(object):
    def __init__(self, screen, border_size, generate_file, export_file,
                 state_file, preserve_aspect_ratio):
        self.generate_file = generate_file
        self.export_file = export_file
        self.border_size = border_size
        self.ReloadHandler()
        self.modified_handler = GenerateModifiedHandler(
            self.generate_file, self.ReloadHandler)

        self.observer = watchdog.observers.Observer()
        self.observer.schedule(self.modified_handler,
                               os.path.dirname(self.generate_file),
                               recursive=True)
        self.observer.start()
        self.raw_samples = []
        self.samples = []

        self.mouse_pos = (0, 0)

        self.clicked = False
        self.new_click = False

        self.partial_sample: Optional[Sample] = None
        self.choosing_index = False

        self.loaded_mapping = False

        self.index_string = ""

        self.state_file = state_file

        self.preserve_aspect_ratio = preserve_aspect_ratio

        font_size = 20

        if screen:
            font_path = pygame.font.match_font("CourierNew")
            self.font = pygame.font.Font(font_path, font_size)

            self.screen = screen

    def __del__(self):
        self.observer.join()

    def MaybeUpdateSampling(self):
        if self.loaded_mapping:
            return

        new_raw_samples = list(self.generate_module.GenerateSampling())
        if (self.raw_samples != new_raw_samples):
            self.raw_samples = new_raw_samples
            self.samples = []
            for raw_sample in self.raw_samples:
                self.samples.append(Sample(coordinate=numpy.array(raw_sample)))

    def Tick(self):
        self.screen.fill((0, ) * 3)

        self.MaybeUpdateSampling()
        self.DrawSamples()
        self.MaybeDrawIndexDialog()
        self.DrawPointsOfInterest(
            self.generate_module.GeneratePointsOfInterest())

        self.mouse_pos = pygame.mouse.get_pos()
        self.clicked = False
        if self.new_click:
            self.clicked = True
            self.new_click = False

    def MaybeDrawIndexDialog(self):
        if not self.choosing_index:
            return

        rect = pygame.Rect(100, 100, 500, 100)
        inner_rect = rect.inflate(-3, -3)
        pygame.draw.rect(self.screen, (255, ) * 3, rect, border_radius=3)
        pygame.draw.rect(self.screen, (0, ) * 3, inner_rect, border_radius=3)

        text_raster = self.font.render("Enter index: " + self.index_string,
                                       True, (255, ) * 3)

        self.screen.blit(text_raster, [_ + 10 for _ in inner_rect.topleft])

    def Clicked(self):
        self.new_click = True

    def ReloadHandler(self):
        self.generate_module = self.ReloadGenerateScript()
        # self.ExportMapping()

    def ReloadGenerateScript(self):
        print("Reloading script from {}".format(self.generate_file))
        generate_spec = importlib.util.spec_from_file_location(
            "generate", self.generate_file)
        generate_module = importlib.util.module_from_spec(generate_spec)
        generate_spec.loader.exec_module(generate_module)
        return generate_module

    def ProcessClick(self, sample: Sample):
        if self.partial_sample is not None:
            if self.partial_sample is sample:
                sample.predecessor = None
                self.choosing_index = True
                return
            if ContainsByReference(sample,
                                   self.partial_sample.GetSuccessors()):
                print("Cycle detected, cancelling")
                self.partial_sample = None
                return
            if sample.successor is not None:
                sample.successor.predecessor = None
            sample.successor = self.partial_sample
            self.partial_sample.predecessor = sample
            self.partial_sample.UpdateDisplayIndex()
            self.partial_sample = None
            return

        self.partial_sample = sample

    def UpdateAllDisplayIndices(self):
        for sample in self.samples:
            sample.UpdateDisplayIndex()

    def ProcessKeystroke(self, key):
        if key == pygame.K_e:
            self.ExportMapping()
        if key == pygame.K_i:
            self.ImportMapping()

        if not self.choosing_index:
            return

        numeric_keys = [
            pygame.K_0,
            pygame.K_1,
            pygame.K_2,
            pygame.K_3,
            pygame.K_4,
            pygame.K_5,
            pygame.K_6,
            pygame.K_7,
            pygame.K_8,
            pygame.K_9,
        ]
        if key in numeric_keys:
            self.index_string += str(numeric_keys.index(key))

        if key == pygame.K_RETURN:
            self.choosing_index = False
            try:
                self.partial_sample.index = int(self.index_string)
            except:
                self.partial_sample = None
                return
            self.partial_sample.UpdateDisplayIndex()
            self.partial_sample = None
            #self.UpdateAllDisplayIndices()

    def DrawArrow(self, sample_from, sample_to):
        from_xy = sample_from.coordinate
        to_xy = sample_to.coordinate

        segment = to_xy - from_xy
        radius = 16

        distance = numpy.linalg.norm(segment)

        if distance < (radius * 2):
            return

        arrow_dir = segment / distance

        arrow_start = arrow_dir * radius + from_xy
        arrow_end = arrow_dir * (distance - radius) + from_xy

        arrow_head_base = arrow_dir * (distance - radius - 3) + from_xy

        arrow_head_center_segment = arrow_end - arrow_head_base

        r_a = numpy.array(
            (arrow_head_center_segment[1], -arrow_head_center_segment[0]))

        pygame.draw.polygon(
            self.screen, (255, ) * 3,
            (arrow_end, arrow_head_base + r_a, arrow_head_base - r_a))

        pygame.draw.line(self.screen, (255, ) * 3, arrow_start, arrow_end)

    def DrawSample(self, sample):
        x, y = sample.coordinate.astype(numpy.int32)

        hovering = False
        color = (255, ) * 3
        if numpy.linalg.norm(
                numpy.array((x, y)) - numpy.array(self.mouse_pos)) <= 16:
            hovering = True
            color = (0, 255, 0)

        if hovering and self.clicked:
            self.ProcessClick(sample)

        if sample is self.partial_sample:
            color = (0, 0, 255)

        pygame.draw.circle(self.screen, color, (x, y), 16, 1)
        text_raster = self.font.render(str(sample.display_index), True,
                                       (255, ) * 3)

        self.screen.blit(text_raster, (numpy.array(
            (x, y)) - (numpy.array(text_raster.get_size()) / 2)).astype(
                numpy.int32))

        if sample.predecessor:
            self.DrawArrow(sample, sample.predecessor)

        if sample.successor:
            self.DrawArrow(sample.successor, sample)

    def DrawPointOfInterest(self, index, coordinate):
        x, y = coordinate.astype(numpy.int32)

        pygame.draw.circle(self.screen, (255, 0, 0), (x, y), 16, 1)
        text_raster = self.font.render("I{:d}".format(index), True,
                                       (255, ) * 3)

        self.screen.blit(text_raster, (numpy.array(
            (x, y)) - (numpy.array(text_raster.get_size()) / 2)).astype(
                numpy.int32))

    def ComputeBounds(self, samples):
        border_coord = numpy.array((self.border_size, self.border_size))
        min_sample = numpy.amin(samples, axis=0) - border_coord
        max_sample = numpy.amax(samples, axis=0) + border_coord
        return min_sample, max_sample

    def DrawSamples(self):
        samples_flattened = numpy.array(list(self.raw_samples))
        min_sample, max_sample = self.ComputeBounds(samples_flattened)
        range_sample = max_sample - min_sample
        pygame.draw.rect(
            self.screen, (255, 255, 255),
            (min_sample[0], min_sample[1], range_sample[0], range_sample[1]),
            2)
        for sample in self.samples:
            self.DrawSample(sample)

    def DrawPointsOfInterest(self, samples):
        for i, sample in enumerate(samples):
            self.DrawPointOfInterest(i, sample)

    def ImportMapping(self):
        if self.state_file:
            with open(self.state_file, 'rb') as import_file:
                self.samples, self.raw_samples = pickle.loads(
                    import_file.read())
        self.loaded_mapping = True
        print("Loaded mapping from file, generate script will be ignored")

    def ExportMapping(self):
        if self.state_file:
            with open(self.state_file, 'wb') as export_file:
                export_file.write(
                    pickle.dumps([self.samples, self.raw_samples]))

        samples = numpy.array(list(self.raw_samples))
        min_sample, max_sample = self.ComputeBounds(samples)
        range_sample = max_sample - min_sample

        center_sample = min_sample + range_sample / 2

        if self.preserve_aspect_ratio:
            range_sample = numpy.amax(range_sample, axis=0) * numpy.array(
                (1, 1))

        normalized_samples = numpy.divide(
            samples - center_sample, range_sample) + numpy.array((0.5, 0.5))

        ordered_coordinates = [numpy.array(
            (0, 0))] * (max([sample.display_index
                             for sample in self.samples]) + 1)

        for sample, normalized_coords in zip(self.samples, normalized_samples):
            if sample.display_index != -1:
                if sample.display_index >= len(ordered_coordinates):
                    print("Sample", sample,
                          "has a display index out of bounds:",
                          sample.display_index, ">=", len(ordered_coordinates))
                    continue
                ordered_coordinates[sample.display_index] = normalized_coords

        mapping = Mapping()
        for normalized_sample in ordered_coordinates:
            coordinate = mapping.samples.add()
            coordinate.x, coordinate.y = normalized_sample

        with open(self.export_file, 'wb') as export_file:
            print("Writing to", self.export_file)
            export_file.write(mapping.SerializeToString())
        with open(self.export_file + ".textproto", 'w') as export_file:
            print("Writing to", self.export_file + ".textproto")
            export_file.write(text_format.MessageToString(mapping))

        points_of_interest = numpy.array(
            list(self.generate_module.GeneratePointsOfInterest()))

        if points_of_interest.size > 0:
            normalized_points_of_interest = numpy.divide(
                points_of_interest - min_sample, range_sample)

            for i, point_of_interest in enumerate(
                    normalized_points_of_interest):
                screen_coord = (point_of_interest - numpy.array(
                    (0.5, 0.5))) * 2
                screen_coord = screen_coord * numpy.array((1, -1))
                print("%Point of interest {:4d}: {:s}".format(
                    i, str(screen_coord)))


def main(argv):
    if not FLAGS.export_only:
        pygame.init()
        screen = pygame.display.set_mode((1800, 1000))
        clock = pygame.time.Clock()
    else:
        screen = None

    mapping_generator = MappingGenerator(screen, FLAGS.border_size,
                                         FLAGS.generate_file,
                                         FLAGS.export_file, FLAGS.state_file,
                                         FLAGS.preserve_aspect_ratio)

    if FLAGS.export_only:
        print("Exported mapping file, exiting")
        exit(0)

    should_exit = False
    while not should_exit:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                should_exit = True

            if event.type == pygame.MOUSEBUTTONUP:
                mapping_generator.Clicked()

            if event.type == pygame.KEYUP:
                mapping_generator.ProcessKeystroke(event.key)

        #try:
        mapping_generator.Tick()
        #except Exception as e:
        #    print(e)

        pygame.display.flip()

        clock.tick(30)


if __name__ == "__main__":
    app.run(main)
