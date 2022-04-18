#include "scene/elements/bitmapelement.h"

BitmapElement::BitmapElement(
        std::string name,
        uint16_t width,
        uint16_t height,
        uint32_t draw_x,
        uint32_t draw_y,
        std::initializer_list<Element*> children
    )
    : Element(name, children) 
    , Adafruit_GFX(width, height)
    , draw_x(draw_x)
    , draw_y(draw_y)
    {
        framebuffer_size = width * height;
        framebuffer = new uint16_t[width * height];
    }

void BitmapElement::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Framebuffer is column major order
    framebuffer[x * HEIGHT + y] = color;
}

uint16_t BitmapElement::getPixel(int16_t x, int16_t y) {
    return framebuffer[x * HEIGHT + y];
}

void BitmapElement::accept(ElementVisitor *visitor) {
        visitor->visit(this);
    }