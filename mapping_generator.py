import pygame
import generate
import numpy

pygame.init()

screen = pygame.display.set_mode((500, 500))
font_path = pygame.font.match_font("CourierNew")

if font_path is None:
    raise Exception()

font_size = 20
font = pygame.font.Font(font_path, font_size)


def DrawSample(index, coordinate):
    x, y = coordinate.astype(numpy.int32)

    pygame.draw.circle(screen, (255,)*3, (x, y), 16, 1)
    text_raster = font.render(str(index), True, (255,)*3)

    screen.blit(text_raster, (numpy.array((x, y)) -
                              (numpy.array(text_raster.get_size())/2)).astype(numpy.int32))


def DrawSamples(samples):
    screen.fill((0,)*3)
    for i, sample in enumerate(samples):
        DrawSample(i, sample)


should_exit = False
while not should_exit:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            should_exit = True

    DrawSamples(generate.GenerateSampling())

    pygame.display.flip()
