#ifndef EDIT_GRAPHICS_HPP
#define EDIT_GRAPHICS_HPP

#include <ctty>
#include <string>

class GraphicsPanel {
private:
    char *colors;
    char *characters; 
    unsigned int width;
    unsigned int height;
    bool buffered = false;

    char& color_at(unsigned int x, unsigned int y) {
        return colors[y * width + x];
    }

    char& character_at(unsigned int x, unsigned int y) {
        return characters[y * width + x];
    }

public:
    GraphicsPanel(unsigned int width = 80, unsigned int height = 25, char default_color = 0x07, bool buffered = false) {
        this->width = width;
        this->height = height;
        this->buffered = buffered;

        colors = new char[width * height];
        characters = new char[width * height];
        
        for (unsigned int y = 0; y < height; y++) {
            for (unsigned int x = 0; x < width; x++) {
                color_at(x, y) = default_color;
                character_at(x, y) = ' ';
            }
        }
    }

    ~GraphicsPanel() {
        delete[] colors;
        delete[] characters;
    }

    void set_buffer(bool buffer) {
        buffered = buffer;
    }

    void blit() {
        for (unsigned int y = 0; y < height; y++) {
            for (unsigned int x = 0; x < width; x++) {
                terminal_putentryat(character_at(x, y), color_at(x, y), x, y);
            }
        }
    }

    void set_color(unsigned int x, unsigned int y, unsigned char color) {
        if (x < width && y < height) {
            color_at(x, y) = color;
            if (!buffered)
                terminal_putentryat(character_at(x, y), color, x, y);
        }
    }

    void draw_rect(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, unsigned char color, unsigned char border) {
        for (unsigned int y = y0; y < y1; y++) {
            for (unsigned int x = x0; x < x1; x++) {
                if (y - y0 < 1 || y1 - y < 2 || x - x0 < 2 || x1 - x < 3) {
                    if (!buffered)
                        terminal_putentryat(' ', border, x, y);
                    color_at(x, y) = border;
                    character_at(x, y) = ' ';
                } else {
                    if (!buffered)
                        terminal_putentryat(' ', color, x, y);
                    color_at(x, y) = color;
                    character_at(x, y) = ' ';
                }
            }
        }
    }

    void put_text(unsigned int x, unsigned int y, std::string text) {
        const char *cstr = text.c_str();
        while (*cstr) {
            character_at(x, y) = *cstr;
            if (!buffered)
                terminal_putentryat(*cstr, color_at(x, y), x, y);
            x++;
            cstr++;
        }
    }

    void put_text_centered(unsigned int x, unsigned int y, unsigned int width, std::string text) {
        if (text.length() >= width) {
            text = text.substr(0, width - 3);
            text.append("...");
            put_text(x, y, text);
        } else {
            unsigned int start_x = x + (width - text.length()) / 2;
            put_text(start_x, y, text);
        }
    }
};

#endif