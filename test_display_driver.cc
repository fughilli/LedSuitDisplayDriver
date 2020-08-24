//
// LED Suit Driver - Embedded host driver software for Kevin's LED suit
// controller. Copyright (C) 2019-2020 Kevin Balke
//
// This file is part of LED Suit Driver.
//
// LED Suit Driver is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// LED Suit Driver is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with LED Suit Driver.  If not, see <http://www.gnu.org/licenses/>.
//

#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>

#include "HexapodController2/bus.h"
#include "display_driver.h"
#include "gfx.h"
#include "ugfx/boards/base/framebuffer_cpp_shim/ugfx_framebuffer.h"

namespace led_driver {

namespace {
constexpr int kVerticalPadding = 2;

GHandle list_handle;

void CustomWidgetRenderer(GWidgetObject *widget, void *parameters) {
  std::cout << "CustomWidgetRenderer invoked for widget at "
            << reinterpret_cast<intptr_t>(widget) << std::endl;
  const GColorSet *colors =
      gwinGetEnabled(reinterpret_cast<GHandle>(widget))
          ? ((widget->g.flags & GBUTTON_FLG_PRESSED) ? &widget->pstyle->pressed
                                                     : &widget->pstyle->enabled)
          : &widget->pstyle->disabled;
  auto display = widget->g.display;
  auto x = widget->g.x;
  auto y = widget->g.y;
  auto width = widget->g.width;
  auto height = widget->g.height;
  gdispGFillArea(display, x + 1, y + 1, width - 2, height - 2,
                 widget->pstyle->background);
  gdispGDrawBox(display, x, y, width, height, colors->edge);

  // Draws the list name
  // gdispGDrawStringBox(display, x, y, width, height, widget->text,
  //                    widget->g.font, colors->text, justifyCenter);

  auto *list_widget = reinterpret_cast<GListObject *>(widget);

  auto item_height =
      gdispGetFontMetric(widget->g.font, fontHeight) + kVerticalPadding;
  const gfxQueueASyncItem *queue_item = nullptr;
  int i = 0;
  for (queue_item = gfxQueueASyncPeek(&list_widget->list_head),
      i = item_height - 1;
       i < list_widget->top && queue_item != nullptr;
       queue_item = gfxQueueASyncNext(queue_item), i += item_height) {
  }

  int offset_y = 0;

  for (offset_y = 1 - (list_widget->top % item_height);
       offset_y < widget->g.height - 2 && queue_item != nullptr;
       queue_item = gfxQueueASyncNext(queue_item), offset_y += item_height) {
    auto *list_item = reinterpret_cast<const ListItem *>(queue_item);
    bool item_selected = list_item->flags & GLIST_FLG_SELECTED;
    gdispGFillArea(display, x + 1, y + offset_y, width - 2, item_height,
                   widget->pstyle->background);
    if (item_selected) {
      gdispGDrawBox(display, x + 2, y + offset_y + 1, width - 4,
                    item_height - 2, colors->edge);
    }
    std::cout << "Drawing item string in box " << x + 2 << ", "
              << y + offset_y + 1 << ", " << width - 4 << ", " << height - 2
              << std::endl;
    gdispGDrawStringBox(display, x + 2, y + offset_y + 1, width - 4,
                        item_height - 2, list_item->text, widget->g.font,
                        colors->text, justifyCenter);
  }

  if (offset_y < height - 1) {
    gdispGFillArea(display, x + 1, y + offset_y, width - 2,
                   height - 1 - offset_y, widget->pstyle->background);
  }
}

void CreateWidgets() {
  GWidgetInit widget_init;
  gwinWidgetClearInit(&widget_init);
  widget_init.customDraw = CustomWidgetRenderer;
  widget_init.customParam = 0;
  widget_init.customStyle = 0;
  widget_init.g.show = false;

  widget_init.g.width = 96;
  widget_init.g.height = 32;
  widget_init.g.y = 0;
  widget_init.g.x = 0;
  widget_init.text = "Options";

  list_handle = gwinListCreate(nullptr, &widget_init, false);
}

bool RenderFrame(std::shared_ptr<ugfx::UgfxFramebuffer> framebuffer,
                 std::shared_ptr<DisplayDriver> display_driver) {
  if (!framebuffer->CopyByPixels([&display_driver](int x, int y) {
        return display_driver->DrawPixel(x, y, DisplayDriver::Color::kWhite);
      })) {
    return false;
  }
  return display_driver->Update();
}
}  // namespace

extern "C" int main(int argc, char *argv[]) {
  auto bus = std::make_shared<i2c::Bus>("/dev/i2c-1");
  auto display_driver = std::make_shared<DisplayDriver>(bus);
  auto framebuffer = std::make_shared<ugfx::UgfxFramebuffer>(128, 32);
  ugfx::RegisterUgfxFramebuffer(framebuffer);

  std::cout << "Initializing display: " << display_driver->Initialize()
            << std::endl;

  gfxInit();
  font_t font = gdispOpenFont("DejaVuSans10");
  gwinSetDefaultFont(font);
  gwinSetDefaultStyle(&BlackWidgetStyle, false);
  gdispClear(Black);

  CreateWidgets();

  gwinListAddItem(list_handle, "Foo", true);
  gwinListAddItem(list_handle, "Bar", true);

  gwinSetVisible(list_handle, true);

  for (int i = 0; i < 100; ++i) {
    if (!RenderFrame(framebuffer, display_driver)) {
      std::cerr << "Failed to render frame " << i << std::endl;
      return 1;
    }
  }

  return 0;
}

}  // namespace led_driver
