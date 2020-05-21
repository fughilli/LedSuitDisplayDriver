
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

import pygame
import numpy
import watchdog.events
import watchdog.observers
import os
import importlib.util
from led_driver.led_mapping_pb2 import Mapping
from absl import app
from absl import flags

flags.DEFINE_string("generate_file", None,
                    "Script to run to generate the LED mapping")
flags.DEFINE_string("export_file", None, "File to export the mapping to")
flags.DEFINE_bool("export_only", False,
                  "Only export the mapping, and then exit")

FLAGS = flags.FLAGS


class GenerateModifiedHandler(watchdog.events.FileSystemEventHandler):
    def __init__(self, filename, handler):
        self.filename = filename
        self.handler = handler

        print("GenerateModifiedHandler initialized for", self.filename)

        super(GenerateModifiedHandler, self).__init__()

    def on_modified(self, event):
        print(event)
        if (event.event_type == "modified" and
            self.filename == event.src_path and
                event.is_directory == False):
            self.handler()


class MappingGenerator(object):
    def __init__(self, screen, generate_file, export_file):
        self.generate_file = generate_file
        self.export_file = export_file
        self.ReloadHandler()
        self.modified_handler = GenerateModifiedHandler(
            self.generate_file, self.ReloadHandler)

        self.observer = watchdog.observers.Observer()
        self.observer.schedule(self.modified_handler,
                               os.path.dirname(self.generate_file), recursive=True)
        self.observer.start()

        font_size = 20
        font_path = pygame.font.match_font("CourierNew")
        self.font = pygame.font.Font(font_path, font_size)

        self.screen = screen

    def __del__(self):
        self.observer.join()

    def Tick(self):
        self.DrawSamples(self.generate_module.GenerateSampling())

    def ReloadHandler(self):
        self.generate_module = self.ReloadGenerateScript()
        self.ExportMapping()

    def ReloadGenerateScript(self):
        print("Reloading script from {}".format(
            self.generate_file))
        generate_spec = importlib.util.spec_from_file_location(
            "generate", self.generate_file)
        generate_module = importlib.util.module_from_spec(generate_spec)
        generate_spec.loader.exec_module(generate_module)
        return generate_module

    def DrawSample(self, index, coordinate):
        x, y = coordinate.astype(numpy.int32)

        pygame.draw.circle(self.screen, (255,)*3, (x, y), 16, 1)
        text_raster = self.font.render(str(index), True, (255,)*3)

        self.screen.blit(text_raster, (numpy.array((x, y)) -
                                       (numpy.array(text_raster.get_size())/2)
                                       ).astype(numpy.int32))

    def DrawSamples(self, samples):
        self.screen.fill((0,)*3)
        for i, sample in enumerate(samples):
            self.DrawSample(i, sample)

    def ExportMapping(self):
        samples = numpy.array(list(self.generate_module.GenerateSampling()))
        min_sample = numpy.amin(samples, axis=0)
        max_sample = numpy.amax(samples, axis=0)
        range_sample = max_sample - min_sample

        normalized_samples = numpy.divide(samples - min_sample, range_sample)

        mapping = Mapping()
        for normalized_sample in normalized_samples:
            coordinate = mapping.samples.add()
            coordinate.x, coordinate.y = normalized_sample

        with open(self.export_file, 'wb') as export_file:
            export_file.write(mapping.SerializeToString())


def main(argv):
    pygame.init()
    screen = pygame.display.set_mode((1000, 1000))
    clock = pygame.time.Clock()

    mapping_generator = MappingGenerator(
        screen, FLAGS.generate_file, FLAGS.export_file)

    if FLAGS.export_only:
        print("Exported mapping file, exiting")
        exit(0)

    should_exit = False
    while not should_exit:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                should_exit = True

        try:
            mapping_generator.Tick()
        except Exception as e:
            print(e)

        pygame.display.flip()

        clock.tick(30)


if __name__ == "__main__":
    app.run(main)
